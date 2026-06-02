// SPDX-License-Identifier: LGPL-2.1-or-later
/*
  scenechange.c: Copyright (C) 2012  Oka Motofumi

  Author: Oka Motofumi (chikuzen.mo at gmail dot com)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the author; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <VapourSynth4.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define SCENECHANGE_PLUGIN_VERSION "0.3.0"
#define LOG_HEADER "# scd scene change detect log version 1\n"

typedef struct scenechange_handler scenechange_t;

struct scenechange_handler {
    VSNode *node;
    const VSVideoInfo *vi;
    int interval_h;
    int interval_v;
    int64_t threshold;
    FILE *log;
    void(VS_CC *function)(scenechange_t *, int, const uint8_t **, VSFrame *, int *, int *);
};

static void VS_CC proc_8bit(
    scenechange_t *sc, int stride, const uint8_t **srcp, VSFrame *dst, int *sc_prev, int *sc_next) {
    int w = sc->vi->width, h = sc->vi->height, interval_h = sc->interval_h,
        interval_v = sc->interval_v;
    int64_t diff_prev = 0, diff_next = 0;

    for (int y = 0; y < h; y += interval_v) {
        for (int x = 0; x < w; x += interval_h) {
            diff_prev += abs((int)srcp[1][x] - srcp[0][x]);
            diff_next += abs((int)srcp[1][x] - srcp[2][x]);
        }
        srcp[0] += stride;
        srcp[1] += stride;
        srcp[2] += stride;
    }

    *sc_prev = (diff_prev > sc->threshold);
    *sc_next = (diff_next > sc->threshold);
}

static void VS_CC proc_16bit(scenechange_t *sc,
                             int stride,
                             const uint8_t **srcp8,
                             VSFrame *dst,
                             int *sc_prev,
                             int *sc_next) {
    int w = sc->vi->width;
    int h = sc->vi->height;
    int interval_h = sc->interval_h;
    int interval_v = sc->interval_v;
    int64_t diff_prev = 0, diff_next = 0;
    const uint16_t *srcp[3] = {(uint16_t *)srcp8[0], (uint16_t *)srcp8[1], (uint16_t *)srcp8[2]};
    stride >>= 1;

    for (int y = 0; y < h; y += interval_v) {
        for (int x = 0; x < w; x += interval_h) {
            diff_prev += abs((int)srcp[1][x] - srcp[0][x]);
            diff_next += abs((int)srcp[1][x] - srcp[2][x]);
        }
        srcp[0] += stride;
        srcp[1] += stride;
        srcp[2] += stride;
    }

    *sc_prev = (diff_prev > sc->threshold);
    *sc_next = (diff_next > sc->threshold);
}

static const VSFrame *VS_CC get_frame(int n,
                                      int activation_reason,
                                      void *instance_data,
                                      void **frame_data,
                                      VSFrameContext *frame_ctx,
                                      VSCore *core,
                                      const VSAPI *vsapi) {
    scenechange_t *sc = (scenechange_t *)instance_data;
    int prev = n <= 0 ? 0 : n - 1;
    int next = n >= sc->vi->numFrames ? sc->vi->numFrames - 1 : n + 1;

    if (activation_reason == arInitial) {
        for (int i = prev; i <= next; i++) {
            vsapi->requestFrameFilter(i, sc->node, frame_ctx);
        }
        return NULL;
    }

    if (activation_reason != arAllFramesReady) {
        return NULL;
    }

    const VSFrame *src[3];
    for (int i = 0; i <= next - prev; i++) {
        src[i] = vsapi->getFrameFilter(i + prev, sc->node, frame_ctx);
    }

    VSFrame *dst = vsapi->copyFrame(src[n - prev], core);

    int sc_prev = !n;
    int sc_next = !(n - sc->vi->numFrames + 1);
    if (sc_prev || sc_next) {
        goto proc_finish;
    }

    const uint8_t *srcp[3];
    for (int i = 0; i <= next - prev; i++) {
        srcp[i] = vsapi->getReadPtr(src[i], 0);
    }
    int stride = vsapi->getStride(src[0], 0) * sc->interval_v;
    sc->function(sc, stride, srcp, dst, &sc_prev, &sc_next);

proc_finish:
    vsapi->mapSetInt(vsapi->getFramePropertiesRW(dst), "_SceneChangePrev", sc_prev, maReplace);
    vsapi->mapSetInt(vsapi->getFramePropertiesRW(dst), "_SceneChangeNext", sc_next, maReplace);

    if (sc->log && (sc_prev + sc_next != 0)) {
        fprintf(sc->log, "%d %d %d\n", n, sc_prev, sc_next);
    }

    for (int i = 0; i <= next - prev; i++) {
        vsapi->freeFrame(src[i]);
    }

    return dst;
}

static void VS_CC close_filter(void *instance_data, VSCore *core, const VSAPI *vsapi) {
    scenechange_t *sc = (scenechange_t *)instance_data;
    if (!sc) {
        return;
    }
    if (sc->node) {
        vsapi->freeNode(sc->node);
        sc->node = NULL;
    }
    if (sc->log) {
        fclose(sc->log);
        sc->log = NULL;
    }
    free(sc);
    sc = NULL;
}

static int VS_CC get_interval(int size) {
    int interval = 1;

    // The resolution required for scene change detection will be less than QVGA.
    while (size / interval > 320) {
        interval++;
    }

    return interval;
}

#define FAIL_IF_ERROR(cond, ...)                                                                   \
    {                                                                                              \
        if (cond) {                                                                                \
            snprintf(msg, 240, __VA_ARGS__);                                                       \
            goto fail;                                                                             \
        }                                                                                          \
    }

static void VS_CC
create_detect(const VSMap *in, VSMap *out, void *user_data, VSCore *core, const VSAPI *vsapi) {
    char msg_buff[256] = "Detect: ";
    char *msg = msg_buff + strlen(msg_buff);

    scenechange_t *sc = (scenechange_t *)calloc(sizeof(scenechange_t), 1);
    FAIL_IF_ERROR(!sc, "failed to allocate handler");

    sc->node = vsapi->mapGetNode(in, "clip", 0, 0);
    sc->vi = vsapi->getVideoInfo(sc->node);
    FAIL_IF_ERROR(!sc->vi->width || !sc->vi->height || sc->vi->format.colorFamily == cfUndefined,
                  "not constant format");
    FAIL_IF_ERROR(sc->vi->format.colorFamily != cfGray && sc->vi->format.colorFamily != cfYUV,
                  "not Gray/YUV format");
    FAIL_IF_ERROR(sc->vi->format.sampleType != stInteger, "float samples are unsupported");

    int err;
    int interval = (int)vsapi->mapGetInt(in, "interval_h", 0, &err);
    if (err || interval <= 0 || interval > sc->vi->width) {
        interval = get_interval(sc->vi->width);
    }
    sc->interval_h = interval;

    interval = (int)vsapi->mapGetInt(in, "interval_v", 0, &err);
    if (err || interval <= 0 || interval > sc->vi->height) {
        interval = get_interval(sc->vi->height);
        // Odd interval_v will cause failure if source is interlaced.
        interval += interval % 2;
    }
    sc->interval_v = interval;

    int resolution = (sc->vi->width / sc->interval_h) * (sc->vi->height / sc->interval_v);

    int shift = sc->vi->format.bitsPerSample - 8;
    sc->threshold = (int)vsapi->mapGetInt(in, "thresh", 0, &err);
    if (err || sc->threshold < 0 || sc->threshold > (254 << shift)) {
        sc->threshold = 15 << shift;
    }
    sc->threshold *= resolution;

    sc->function = shift == 0 ? proc_8bit : proc_16bit;

    const char *log_name = vsapi->mapGetData(in, "log", 0, &err);
    if (!err) {
        sc->log = fopen(log_name, "w");
        FAIL_IF_ERROR(!sc->log, "failed to create log file");
        fprintf(sc->log, LOG_HEADER "frames: %d\n", sc->vi->numFrames);
    }

    const VSFilterDependency deps[] = {{sc->node, rpGeneral}};
    vsapi->createVideoFilter(
        out, "Detect", sc->vi, get_frame, close_filter, fmParallel, deps, 1, sc, core);

    return;

fail:
    close_filter(sc, core, vsapi);
    vsapi->mapSetError(out, msg_buff);
}
#undef FAIL_IF_ERROR

typedef struct {
    VSNode *node;
    const VSVideoInfo *vi;
    uint8_t *is_scene_change;
} apply_log_t;

static const VSFrame *VS_CC get_frame_apply(int n,
                                            int activation_reason,
                                            void *instance_data,
                                            void **frame_data,
                                            VSFrameContext *frame_ctx,
                                            VSCore *core,
                                            const VSAPI *vsapi) {
    apply_log_t *al = (apply_log_t *)instance_data;
    if (n > al->vi->numFrames - 1) {
        n = al->vi->numFrames - 1;
    }

    if (activation_reason == arInitial) {
        vsapi->requestFrameFilter(n, al->node, frame_ctx);
        return NULL;
    }

    if (activation_reason != arAllFramesReady) {
        return NULL;
    }

    const VSFrame *src = vsapi->getFrameFilter(n, al->node, frame_ctx);
    VSFrame *dst = vsapi->copyFrame(src, core);
    vsapi->mapSetInt(vsapi->getFramePropertiesRW(dst),
                     "_SceneChangePrev",
                     (al->is_scene_change[n] & 2),
                     maReplace);
    vsapi->mapSetInt(vsapi->getFramePropertiesRW(dst),
                     "_SceneChangeNext",
                     (al->is_scene_change[n] & 1),
                     maReplace);
    vsapi->freeFrame(src);

    return dst;
}

static void VS_CC close_apply(void *instance_data, VSCore *core, const VSAPI *vsapi) {
    apply_log_t *al = (apply_log_t *)instance_data;
    if (!al) {
        return;
    }
    if (al->node) {
        vsapi->freeNode(al->node);
        al->node = NULL;
    }
    if (al->is_scene_change) {
        free(al->is_scene_change);
        al->is_scene_change = NULL;
    }
    free(al);
    al = NULL;
}

#define SCD_PARSE_LOG_BUFF_SIZE 1024

static const char *VS_CC parse_log(apply_log_t *al, FILE *log) {
    char read_buff[SCD_PARSE_LOG_BUFF_SIZE];
    if (!fgets(read_buff, SCD_PARSE_LOG_BUFF_SIZE, log) || strcmp(read_buff, LOG_HEADER) != 0) {
        return "unsupported log file";
    }

    int num = 0;
    int prev = 0;
    int next = 0;
    fgets(read_buff, SCD_PARSE_LOG_BUFF_SIZE, log);
    sscanf(read_buff, "frames: %d\n", &num);
    if (num != al->vi->numFrames) {
        return "number of frames does not match";
    }

    al->is_scene_change = (uint8_t *)calloc(1, num);
    if (!al->is_scene_change) {
        return "out of memory";
    }

    while (fgets(read_buff, SCD_PARSE_LOG_BUFF_SIZE, log)) {
        if (sscanf(read_buff, "%d %d %d\n", &num, &prev, &next) == EOF || num < 0 ||
            num >= al->vi->numFrames) {
            continue;
        };
        al->is_scene_change[num] = ((!!prev << 1) | !!next);
    }

    fclose(log);
    log = NULL;

    return NULL;
#undef BUFF_SIZE
}

#define FAIL_IF_ERROR(cond, ...)                                                                   \
    {                                                                                              \
        if (cond) {                                                                                \
            snprintf(msg, 240, __VA_ARGS__);                                                       \
            goto fail2;                                                                            \
        }                                                                                          \
    }

static void VS_CC
create_apply_log(const VSMap *in, VSMap *out, void *user_data, VSCore *core, const VSAPI *vsapi) {
    char msg_buff[256] = "ApplyLog: ";
    char *msg = msg_buff + strlen(msg_buff);
    FILE *log = NULL;

    apply_log_t *al = (apply_log_t *)calloc(sizeof(apply_log_t), 1);
    FAIL_IF_ERROR(!al, "failed to allocate handler");

    al->node = vsapi->mapGetNode(in, "clip", 0, 0);
    al->vi = vsapi->getVideoInfo(al->node);

    const char *log_name = vsapi->mapGetData(in, "log", 0, NULL);
    log = fopen(log_name, "r");
    FAIL_IF_ERROR(!log, "failed to open log %s", log_name);

    const char *ret = parse_log(al, log);
    FAIL_IF_ERROR(ret, "%s", ret);

    const VSFilterDependency deps[] = {{al->node, rpStrictSpatial}};
    vsapi->createVideoFilter(
        out, "ApplyLog", al->vi, get_frame_apply, close_apply, fmParallel, deps, 1, al, core);

    return;

fail2:
    if (log) {
        fclose(log);
    }
    close_apply(al, core, vsapi);
    vsapi->mapSetError(out, msg_buff);
}

VS_EXTERNAL_API(void)
VapourSynthPluginInit2(VSPlugin *plugin, const VSPLUGINAPI *vspapi) {
    vspapi->configPlugin("chikuzen.does.not.have.his.own.domain.scd",
                         "scd",
                         "Scene change detect filter for VapourSynth v" SCENECHANGE_PLUGIN_VERSION,
                         VS_MAKE_VERSION(0, 3),
                         VAPOURSYNTH_API_VERSION,
                         0,
                         plugin);
    vspapi->registerFunction("Detect",
                             "clip:vnode;thresh:int:opt;interval_h:int:opt;interval_v:int:opt;"
                             "log:data:opt;",
                             "clip:vnode;",
                             create_detect,
                             NULL,
                             plugin);
    vspapi->registerFunction(
        "ApplyLog", "clip:vnode;log:data;", "clip:vnode;", create_apply_log, NULL, plugin);
}
