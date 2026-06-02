// SPDX-License-Identifier: LGPL-2.1-or-later
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <VapourSynth4.h>

#include "temporalsoften_kernel.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define TEMPORALSOFTEN2_VERSION "0.3.0"

typedef struct {
    VSNode *node;
    const VSVideoInfo *vi;

    // Filter parameters.
    int radius;
    int threshold[3];
    int scenechange;
    int mode;
    void(VS_CC *proc)(uint8_t *, const uint8_t **, int, int, int);
} TemporalSoftenData;

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

static const VSFrame *VS_CC temporalSoftenGetFrame(int n,
                                                   int activationReason,
                                                   void *instanceData,
                                                   void **frameData,
                                                   VSFrameContext *frameCtx,
                                                   VSCore *core,
                                                   const VSAPI *vsapi) {
    TemporalSoftenData *d = (TemporalSoftenData *)instanceData;
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

    // The reason for 16 is unclear; the most we can have is 7*2+1.
    const VSFrame *src[16] = {
        vsapi->getFrameFilter(n, d->node, frameCtx), // Frame n is always src[0].
        NULL};
    int frames = 1;               // Number of effective source frames.
    int sc_prev = 0, sc_next = 0; // Flags for checking scene changes.
    if (d->scenechange != 0) {
        sc_prev = vsapi->mapGetInt(vsapi->getFramePropertiesRO(src[0]), "_SceneChangePrev", 0, 0);
        sc_next = vsapi->mapGetInt(vsapi->getFramePropertiesRO(src[0]), "_SceneChangeNext", 0, 0);
    }

    int num = n - first;
    for (i = 1; i <= num; i++) {
        src[frames] = vsapi->getFrameFilter(n - i, d->node, frameCtx);
        if (sc_prev != 0) {
            vsapi->freeFrame(src[frames]); // Release garbage immediately.
            continue;
        }
        if (d->scenechange != 0) {
            sc_prev = vsapi->mapGetInt(
                vsapi->getFramePropertiesRO(src[frames]), "_SceneChangePrev", 0, 0);
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
            sc_next = vsapi->mapGetInt(
                vsapi->getFramePropertiesRO(src[frames]), "_SceneChangeNext", 0, 0);
        }
        frames++;
    }

    VSFrame *dst = vsapi->copyFrame(src[0], core);

    int plane;
    num = d->vi->format.numPlanes;
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

    d.node = vsapi->mapGetNode(in, "clip", 0, 0);
    d.vi = vsapi->getVideoInfo(d.node);

    FAIL_IF_ERROR(d.vi->format.colorFamily == cfUndefined || d.vi->width == 0 || d.vi->height == 0,
                  "clip must be constant format");

    FAIL_IF_ERROR(d.vi->format.sampleType != stInteger || d.vi->format.bitsPerSample > 16 ||
                      (d.vi->format.colorFamily != cfYUV && d.vi->format.colorFamily != cfRGB &&
                       d.vi->format.colorFamily != cfGray),
                  "only 8..16 bit integer YUV, RGB, or Gray input supported");

    int bshift = d.vi->format.bitsPerSample - 8;

    d.radius = vsapi->mapGetInt(in, "radius", 0, &err);
    if (err) {
        d.radius = 4;
    }

    d.threshold[0] = vsapi->mapGetInt(in, "luma_threshold", 0, &err);
    if (err) {
        d.threshold[0] = (4 << bshift);
    }
    if (d.vi->format.colorFamily == cfRGB) {
        d.threshold[1] = d.threshold[0];
    }

    if (d.vi->format.colorFamily == cfYUV) {
        d.threshold[1] = vsapi->mapGetInt(in, "chroma_threshold", 0, &err);
        if (err) {
            d.threshold[1] = (8 << bshift);
        }
    }
    d.threshold[2] = d.threshold[1];

    d.scenechange = vsapi->mapGetInt(in, "scenechange", 0, &err);
    if (err) {
        d.scenechange = 1;
    }
    d.mode = vsapi->mapGetInt(in, "mode", 0, &err);
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
                      (d.vi->format.colorFamily == cfRGB || d.vi->format.colorFamily == cfGray),
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

    const VSFilterDependency deps[] = {{data->node, rpGeneral}};
    vsapi->createVideoFilter(out,
                             "TemporalSoften2",
                             data->vi,
                             temporalSoftenGetFrame,
                             temporalSoftenFree,
                             fmParallel,
                             deps,
                             1,
                             data,
                             core);
    return;

fail:
    vsapi->freeNode(d.node);
    vsapi->mapSetError(out, msg_buff);
}

VS_EXTERNAL_API(void)
VapourSynthPluginInit2(VSPlugin *plugin, const VSPLUGINAPI *vspapi) {
    vspapi->configPlugin("chikuzen.does.not.have.his.own.domain.focus2",
                         "focus2",
                         "VapourSynth TemporalSoften Filter v" TEMPORALSOFTEN2_VERSION,
                         VS_MAKE_VERSION(0, 3),
                         VAPOURSYNTH_API_VERSION,
                         0,
                         plugin);
    vspapi->registerFunction("TemporalSoften2",
                             "clip:vnode;radius:int:opt;luma_threshold:int:opt;"
                             "chroma_threshold:int:opt;scenechange:int:opt;mode:int:opt;",
                             "clip:vnode;",
                             temporalSoftenCreate,
                             0,
                             plugin);
}
