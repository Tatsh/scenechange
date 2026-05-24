// SPDX-License-Identifier: LGPL-2.1-or-later
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <VapourSynth.h>

#include "temporalsoften_kernel.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define TEMPORALSOFTEN2_VERSION "0.2.0"

typedef struct {
    VSNodeRef *node;
    const VSVideoInfo *vi;

    // Filter parameters.
    int radius;
    int threshold[3];
    int scenechange;
    int mode;
    void(VS_CC *proc)(uint8_t *, const uint8_t **, int, int, int);
} TemporalSoftenData;

static void VS_CC temporalSoftenInit(
    VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
    TemporalSoftenData *d = (TemporalSoftenData *)*instanceData;
    vsapi->setVideoInfo(d->vi, 1, node);
}

// MSVC's <stdlib.h> defines max and min as macros, so the wrapper functions
// have to use distinct names.
static int ts_max(int n, int m) {
    if (n < m) {
        return m;
    }
    return n;
}

static int ts_min(int n, int m) {
    if (n > m) {
        return m;
    }
    return n;
}

static const VSFrameRef *VS_CC temporalSoftenGetFrame(int n,
                                                      int activationReason,
                                                      void **instanceData,
                                                      void **frameData,
                                                      VSFrameContext *frameCtx,
                                                      VSCore *core,
                                                      const VSAPI *vsapi) {
    TemporalSoftenData *d = (TemporalSoftenData *)*instanceData;
    n = ts_min(ts_max(n, 0), d->vi->numFrames - 1);
    int first = ts_max(n - d->radius, 0);
    int last = ts_min(n + d->radius, d->vi->numFrames - 1);
    int i;

    if (activationReason == arInitial) {
        for (i = first; i <= last; i++) {
            vsapi->requestFrameFilter(i, d->node, frameCtx);
        }
        return NULL;
    }

    if (activationReason != arAllFramesReady) {
        return NULL;
    }

    // Not sure why 16... the most we can have is 7*2+1
    const VSFrameRef *src[16] = {
        vsapi->getFrameFilter(n, d->node, frameCtx), // frame n is always src[0]
        NULL};
    int frames = 1;               // number of effective source frames
    int sc_prev = 0, sc_next = 0; // flags for checking scene change
    if (d->scenechange != 0) {
        sc_prev = vsapi->propGetInt(vsapi->getFramePropsRO(src[0]), "_SceneChangePrev", 0, 0);
        sc_next = vsapi->propGetInt(vsapi->getFramePropsRO(src[0]), "_SceneChangeNext", 0, 0);
    }

    int num = n - first;
    for (i = 1; i <= num; i++) {
        src[frames] = vsapi->getFrameFilter(n - i, d->node, frameCtx);
        if (sc_prev != 0) {
            vsapi->freeFrame(src[frames]); // Release garbage immediately
            continue;
        }
        if (d->scenechange != 0) {
            sc_prev =
                vsapi->propGetInt(vsapi->getFramePropsRO(src[frames]), "_SceneChangePrev", 0, 0);
        }
        frames++;
    }
    num = last - n;
    for (i = 1; i <= num; i++) {
        src[frames] = vsapi->getFrameFilter(n + i, d->node, frameCtx);
        if (sc_next != 0) {
            vsapi->freeFrame(src[frames]);
            continue;
        }
        if (d->scenechange != 0) {
            sc_next =
                vsapi->propGetInt(vsapi->getFramePropsRO(src[frames]), "_SceneChangeNext", 0, 0);
        }
        frames++;
    }

    VSFrameRef *dst = vsapi->copyFrame(src[0], core);

    int plane;
    num = d->vi->format->numPlanes;
    for (plane = 0; plane < num; plane++) {
        if (d->threshold[plane] == 0) {
            continue;
        }
        const uint8_t *srcp[16];
        for (i = 0; i < frames; i++) {
            srcp[i] = vsapi->getReadPtr(src[i], plane);
        }
        int frame_size = vsapi->getFrameHeight(dst, plane) * vsapi->getStride(dst, plane);
        uint8_t *dstp = vsapi->getWritePtr(dst, plane);

        d->proc(dstp, srcp, frames, frame_size, d->threshold[plane]);
    }

    for (i = 0; i < frames; i++) {
        vsapi->freeFrame(src[i]);
    }

    return dst;
}

static void VS_CC temporalSoftenFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
    TemporalSoftenData *d = (TemporalSoftenData *)instanceData;
    vsapi->freeNode(d->node);
    free(d);
}

#define FAIL_IF_ERROR(cond, ...)                                                                   \
    {                                                                                              \
        if (cond) {                                                                                \
            snprintf(msg, 235, __VA_ARGS__);                                                       \
            goto fail;                                                                             \
        }                                                                                          \
    }

static void VS_CC temporalSoftenCreate(
    const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
    TemporalSoftenData d = {0};
    int err;
    char msg_buff[256] = "TemporalSoften2: ";
    char *msg = msg_buff + strlen(msg_buff);

    d.node = vsapi->propGetNode(in, "clip", 0, 0);
    d.vi = vsapi->getVideoInfo(d.node);

    FAIL_IF_ERROR(!d.vi->format || d.vi->width == 0 || d.vi->height == 0,
                  "clip must be constant format");

    FAIL_IF_ERROR(d.vi->format->sampleType != stInteger || d.vi->format->bitsPerSample > 16 ||
                      (d.vi->format->colorFamily != cmYUV && d.vi->format->colorFamily != cmRGB &&
                       d.vi->format->colorFamily != cmGray),
                  "only 8..16 bit integer YUV, RGB, or Gray input supported");

    int bshift = d.vi->format->bitsPerSample - 8;

    d.radius = vsapi->propGetInt(in, "radius", 0, &err);
    if (err) {
        d.radius = 4;
    }

    d.threshold[0] = vsapi->propGetInt(in, "luma_threshold", 0, &err);
    if (err) {
        d.threshold[0] = (4 << bshift);
    }
    if (d.vi->format->colorFamily == cmRGB) {
        d.threshold[1] = d.threshold[0];
    }

    if (d.vi->format->colorFamily == cmYUV) {
        d.threshold[1] = vsapi->propGetInt(in, "chroma_threshold", 0, &err);
        if (err) {
            d.threshold[1] = (8 << bshift);
        }
    }
    d.threshold[2] = d.threshold[1];

    d.scenechange = vsapi->propGetInt(in, "scenechange", 0, &err);
    if (err) {
        d.scenechange = 1;
    }
    d.mode = vsapi->propGetInt(in, "mode", 0, &err);
    if (err) {
        d.mode = 2;
    }

    int maximum = 0xFF << bshift;

    FAIL_IF_ERROR(d.radius < 1 || d.radius > 7, "radius must be between 1 and 7 (inclusive)");

    FAIL_IF_ERROR(d.threshold[0] < 0 || d.threshold[0] > maximum,
                  "luma_threshold must be between 0 and %d (inclusive)",
                  maximum);

    FAIL_IF_ERROR(d.threshold[1] < 0 || d.threshold[2] > maximum,
                  "chroma_threshold must be between 0 and %d (inclusive)",
                  maximum);

    FAIL_IF_ERROR(d.threshold[0] == 0 &&
                      (d.vi->format->colorFamily == cmRGB || d.vi->format->colorFamily == cmGray),
                  "luma_threshold must not be 0 when the input is RGB or Gray");

    FAIL_IF_ERROR(d.threshold[0] == 0 && d.threshold[1] == 0,
                  "luma_threshold and chroma_threshold can't both be 0");

    FAIL_IF_ERROR(d.mode != 2, "mode must be 2. mode 1 is not implemented");

    switch (bshift) {
    case 0:
        d.proc = ts_mode2_8bit;
        break;
    case 1:
    case 2:
        d.proc = ts_mode2_9_or_10;
        break;
    default:
        d.proc = ts_mode2_16bit;
    }

    TemporalSoftenData *data = (TemporalSoftenData *)malloc(sizeof(d));
    *data = d;

    vsapi->createFilter(in,
                        out,
                        "TemporalSoften2",
                        temporalSoftenInit,
                        temporalSoftenGetFrame,
                        temporalSoftenFree,
                        fmParallel,
                        0,
                        data,
                        core);
    return;

fail:
    vsapi->freeNode(d.node);
    vsapi->setError(out, msg_buff);
}

VS_EXTERNAL_API(void)
VapourSynthPluginInit(VSConfigPlugin configFunc,
                      VSRegisterFunction registerFunc,
                      VSPlugin *plugin) {
    configFunc("chikuzen.does.not.have.his.own.domain.focus2",
               "focus2",
               "VapourSynth TemporalSoften Filter v" TEMPORALSOFTEN2_VERSION,
               VAPOURSYNTH_API_VERSION,
               1,
               plugin);
    registerFunc("TemporalSoften2",
                 "clip:clip;radius:int:opt;luma_threshold:int:opt;"
                 "chroma_threshold:int:opt;scenechange:int:opt;mode:int:opt;",
                 temporalSoftenCreate,
                 0,
                 plugin);
}
