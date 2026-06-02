// SPDX-License-Identifier: LGPL-2.1-or-later
#include <stdlib.h>

#include "temporalsoften_kernel.h"

// Generic scalar fallback for architectures without a dedicated SIMD backend.
// Modelled on the scalar reference for mode 2 that previously sat in a
// disabled block at the top of temporalsoften.c, then extended with the
// 9-or-10-bit case and adapted to the (frames, frame_size, threshold)
// signature so it is a drop-in replacement for the SSE2 kernel.

void VS_CC
ts_mode2_8bit(uint8_t *dstp, const uint8_t **srcp, int frames, int frame_size, int threshold) {
    int half_frames = frames / 2;

    for (int x = 0; x < frame_size; x++) {
        unsigned sum = dstp[x];
        for (int f = 1; f < frames; f++) {
            int val = dstp[x];
            if (abs(val - srcp[f][x]) <= threshold) {
                val = srcp[f][x];
            }
            sum += val;
        }
        dstp[x] = (uint8_t)((sum + half_frames) / frames);
    }
}

void VS_CC
ts_mode2_9_or_10(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold) {
    uint16_t *dstp = (uint16_t *)dstp8;
    const uint16_t *srcp[16];
    int half_frames = frames / 2;
    int samples = frame_size / 2;

    for (int f = 1; f < frames; f++) {
        srcp[f] = (const uint16_t *)srcp8[f];
    }

    for (int x = 0; x < samples; x++) {
        unsigned sum = dstp[x];
        for (int f = 1; f < frames; f++) {
            int val = dstp[x];
            if (abs(val - srcp[f][x]) <= threshold) {
                val = srcp[f][x];
            }
            sum += val;
        }
        dstp[x] = (uint16_t)((sum + half_frames) / frames);
    }
}

void VS_CC
ts_mode2_16bit(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold) {
    uint16_t *dstp = (uint16_t *)dstp8;
    const uint16_t *srcp[16];
    int half_frames = frames / 2;
    int samples = frame_size / 2;

    for (int f = 1; f < frames; f++) {
        srcp[f] = (const uint16_t *)srcp8[f];
    }

    for (int x = 0; x < samples; x++) {
        unsigned sum = dstp[x];
        for (int f = 1; f < frames; f++) {
            int val = dstp[x];
            if (abs(val - srcp[f][x]) <= threshold) {
                val = srcp[f][x];
            }
            sum += val;
        }
        dstp[x] = (uint16_t)((sum + half_frames) / frames);
    }
}
