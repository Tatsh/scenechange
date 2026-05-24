// SPDX-License-Identifier: LGPL-2.1-or-later
#include <arm_neon.h>
#include <math.h>
#include <stdint.h>

#include "temporalsoften_kernel.h"

#ifdef _MSC_VER
#define TS_ALIGN __declspec(align(16))
#else
#define TS_ALIGN __attribute__((aligned(16)))
#endif

// NEON has no direct signed/unsigned mulhi for 16-bit lanes, but the SSE2
// kernel relies on the (a * b) >> 16 reciprocal-multiply pattern for its
// final divide. Build the equivalent via a widening 16x16 -> 32 multiply and
// a narrowing >> 16, producing one 16-bit half-result per call.
static inline int16x8_t ts_mulhi_s16(int16x8_t a, int16_t b) {
    int16x4_t lo = vshrn_n_s32(vmull_n_s16(vget_low_s16(a), b), 16);
    int16x4_t hi = vshrn_n_s32(vmull_n_s16(vget_high_s16(a), b), 16);
    return vcombine_s16(lo, hi);
}

static inline uint16x8_t ts_mulhi_u16(uint16x8_t a, uint16_t b) {
    uint16x4_t lo = vshrn_n_u32(vmull_n_u16(vget_low_u16(a), b), 16);
    uint16x4_t hi = vshrn_n_u32(vmull_n_u16(vget_high_u16(a), b), 16);
    return vcombine_u16(lo, hi);
}

void VS_CC
ts_mode2_8bit(uint8_t *dstp, const uint8_t **srcp, int frames, int frame_size, int threshold) {
    int16_t half_frames = (int16_t)(frames / 2);
    int16_t mul = (int16_t)ceil((1.0 / frames) * 65536);
    uint8x16_t thrsh = vdupq_n_u8((uint8_t)threshold);
    uint8x16_t zero8 = vdupq_n_u8(0);

    do {
        uint8x16_t dstpx = vld1q_u8(dstp);

        uint16x8_t sum_lo =
            vqaddq_u16(vdupq_n_u16((uint16_t)half_frames), vmovl_u8(vget_low_u8(dstpx)));
        uint16x8_t sum_hi =
            vqaddq_u16(vdupq_n_u16((uint16_t)half_frames), vmovl_u8(vget_high_u8(dstpx)));

        for (int f = 1; f < frames; f++) {
            uint8x16_t srcpf = vld1q_u8(srcp[f]);

            // |dst - src| via max - min (always non-negative, so saturating not needed).
            uint8x16_t diff = vqsubq_u8(vmaxq_u8(dstpx, srcpf), vminq_u8(dstpx, srcpf));

            // mask byte = 0xFF where diff <= threshold (saturating sub then compare with 0).
            uint8x16_t mask = vceqq_u8(vqsubq_u8(diff, thrsh), zero8);

            uint8x16_t selected = vbslq_u8(mask, srcpf, dstpx);

            sum_lo = vqaddq_u16(sum_lo, vmovl_u8(vget_low_u8(selected)));
            sum_hi = vqaddq_u16(sum_hi, vmovl_u8(vget_high_u8(selected)));
        }

        int16x8_t out_lo;
        int16x8_t out_hi;
        if (frames > 2) {
            out_lo = ts_mulhi_s16(vreinterpretq_s16_u16(sum_lo), mul);
            out_hi = ts_mulhi_s16(vreinterpretq_s16_u16(sum_hi), mul);
        } else {
            // Runtime shift count: vshlq_s16 with negative immediate-vector shifts right.
            int16x8_t shift = vdupq_n_s16((int16_t)-(frames - 1));
            out_lo = vshlq_s16(vreinterpretq_s16_u16(sum_lo), shift);
            out_hi = vshlq_s16(vreinterpretq_s16_u16(sum_hi), shift);
        }
        vst1q_u8(dstp, vcombine_u8(vqmovun_s16(out_lo), vqmovun_s16(out_hi)));

        dstp += 16;
        for (int f = 1; f < frames; srcp[f++] += 16)
            ;
    } while ((frame_size -= 16) > 0);
}

void VS_CC
ts_mode2_9_or_10(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold) {
    int16_t *dstp = (int16_t *)dstp8;
    const int16_t *srcp[16];
    int16_t half_frames = (int16_t)(frames / 2);
    int16_t mul = (int16_t)ceil((1.0 / frames) * 65536);

    for (int f = 1; f < frames; f++) {
        srcp[f] = (const int16_t *)srcp8[f];
    }

    int16x8_t thrsh = vdupq_n_s16((int16_t)threshold);

    do {
        int16x8_t dstpx = vld1q_s16(dstp);
        int16x8_t sum = vaddq_s16(vdupq_n_s16(half_frames), dstpx);

        for (int f = 1; f < frames; f++) {
            int16x8_t srcpf = vld1q_s16(srcp[f]);
            int16x8_t diff = vsubq_s16(vmaxq_s16(dstpx, srcpf), vminq_s16(dstpx, srcpf));
            // 9- and 10-bit path uses strict less-than to mirror the SSE2 kernel.
            uint16x8_t mask = vcltq_s16(diff, thrsh);
            int16x8_t selected = vbslq_s16(mask, srcpf, dstpx);
            sum = vaddq_s16(sum, selected);
        }

        if (frames > 2) {
            sum = vreinterpretq_s16_u16(ts_mulhi_u16(vreinterpretq_u16_s16(sum), (uint16_t)mul));
        } else {
            int16x8_t shift = vdupq_n_s16((int16_t)-(frames - 1));
            sum = vshlq_s16(sum, shift);
        }
        vst1q_s16(dstp, sum);

        dstp += 8;
        for (int f = 1; f < frames; srcp[f++] += 8)
            ;
    } while ((frame_size -= 16) > 0);
}

typedef struct {
    TS_ALIGN uint16_t buff[8];
} buff_t;

void VS_CC
ts_mode2_16bit(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold) {
    uint16_t *dstp = (uint16_t *)dstp8;
    const uint16_t *srcp[16];
    int half_frames = frames / 2;
    uint64_t r = ((uint64_t)1 << 40) / frames;
    buff_t buffer[16];

    for (int f = 1; f < frames; f++) {
        srcp[f] = (const uint16_t *)srcp8[f];
    }

    uint16x8_t thrsh = vdupq_n_u16((uint16_t)threshold);
    uint16x8_t zero16 = vdupq_n_u16(0);

    do {
        uint16x8_t dstpx = vld1q_u16(dstp);
        for (int f = 0; f < 8; f++) {
            buffer[0].buff[f] = dstp[f];
        }

        for (int f = 1; f < frames; f++) {
            uint16x8_t srcpf = vld1q_u16(srcp[f]);

            // hi >= lo so the subtract cannot underflow.
            uint16x8_t diff = vsubq_u16(vmaxq_u16(dstpx, srcpf), vminq_u16(dstpx, srcpf));

            uint16x8_t mask = vceqq_u16(vqsubq_u16(diff, thrsh), zero16);
            uint16x8_t selected = vbslq_u16(mask, srcpf, dstpx);

            vst1q_u16(buffer[f].buff, selected);
        }

        // Scalar reduction: NEON, like SSE2, has no integer mulhi for 32-bit
        // lanes, so the final per-sample average uses a 64-bit fixed-point
        // reciprocal multiply.
        for (int t = 0; t < 8; t++) {
            uint32_t sum = half_frames;
            for (int f = 0; f < frames; sum += buffer[f++].buff[t])
                ;
            dstp[t] = (uint16_t)((sum * r + (1 << 20)) >> 40);
        }

        dstp += 8;
        for (int f = 1; f < frames; srcp[f++] += 8)
            ;
    } while ((frame_size -= 16) > 0);
}
