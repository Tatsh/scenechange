// SPDX-License-Identifier: LGPL-2.1-or-later
#include <emmintrin.h>
#include <math.h>
#include <stdint.h>

#include "temporalsoften_kernel.h"

#ifdef _MSC_VER
#define TS_ALIGN __declspec(align(16))
#else
#define TS_ALIGN __attribute__((aligned(16)))
#endif

// force_align_arg_pointer is an i386-only attribute that asks GCC to realign
// the incoming stack pointer in the function prologue, which __m128i locals
// need but the legacy i386 SysV ABI does not guarantee. It is a silent no-op
// on x86-64 (which guarantees 16-byte alignment) and unsupported on MSVC.
#if defined(__GNUC__) && defined(__i386__)
#define TS_FUNC_ALIGN __attribute__((force_align_arg_pointer))
#else
#define TS_FUNC_ALIGN
#endif

void TS_FUNC_ALIGN VS_CC
ts_mode2_8bit(uint8_t *dstp, const uint8_t **srcp, int frames, int frame_size, int threshold) {
    int16_t half_frames = frames / 2;
    int f;
    int16_t mul = (int16_t)ceil((1.0 / frames) * 65536);
    __m128i thrsh, dstpx, sum_lo, sum_hi, srcpf, temp0, temp1, temp2;

    thrsh = _mm_set1_epi8((int8_t)threshold);

    do {
        sum_lo = _mm_set1_epi16(half_frames);
        sum_hi = _mm_set1_epi16(half_frames);

        dstpx = _mm_load_si128((__m128i *)dstp);

        temp0 = _mm_setzero_si128();

        temp1 = _mm_unpacklo_epi8(dstpx, temp0);
        sum_lo = _mm_adds_epu16(sum_lo, temp1);

        temp1 = _mm_unpackhi_epi8(dstpx, temp0);
        sum_hi = _mm_adds_epu16(sum_hi, temp1);

        for (f = 1; f < frames; f++) {
            srcpf = _mm_load_si128((__m128i *)srcp[f]);

            temp0 = _mm_max_epu8(dstpx, srcpf);
            temp1 = _mm_min_epu8(dstpx, srcpf);
            temp0 = _mm_subs_epu8(temp0, temp1); // abs(dstp - srcp[f])

            temp1 = _mm_subs_epu8(temp0, thrsh);
            temp0 = _mm_setzero_si128();
            temp0 = _mm_cmpeq_epi8(temp1, temp0);

            srcpf = _mm_and_si128(srcpf, temp0); // effective values of srcp[f]

            temp1 = _mm_set1_epi8((char)0xFF);
            temp0 = _mm_xor_si128(temp0, temp1); // invert temp0
            temp0 = _mm_and_si128(dstpx, temp0); // effective values of dstp

            srcpf = _mm_or_si128(srcpf, temp0); // all effective values

            temp0 = _mm_setzero_si128();

            temp1 = _mm_unpacklo_epi8(srcpf, temp0);
            sum_lo = _mm_adds_epu16(sum_lo, temp1);

            temp1 = _mm_unpackhi_epi8(srcpf, temp0);
            sum_hi = _mm_adds_epu16(sum_hi, temp1);
        }
        if (frames > 2) {
            temp2 = _mm_set1_epi16(mul);
            sum_lo = _mm_mulhi_epi16(sum_lo, temp2);
            sum_hi = _mm_mulhi_epi16(sum_hi, temp2);
        } else {
            sum_lo = _mm_srai_epi16(sum_lo, frames - 1);
            sum_hi = _mm_srai_epi16(sum_hi, frames - 1);
        }
        sum_lo = _mm_packus_epi16(sum_lo, sum_hi);
        _mm_store_si128((__m128i *)dstp, sum_lo);

        dstp += 16;
        for (f = 1; f < frames; srcp[f++] += 16)
            ;
    } while ((frame_size -= 16) > 0);
}

void TS_FUNC_ALIGN VS_CC
ts_mode2_9_or_10(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold) {
    uint16_t *dstp = (uint16_t *)dstp8;
    const uint16_t *srcp[16];
    int half_frames = frames / 2;
    int f;
    int16_t mul = (int16_t)ceil((1.0 / frames) * 65536);
    __m128i thrsh, dstpx, sum, srcpf, temp0, temp1;

    for (f = 1; f < frames; f++) {
        srcp[f] = (uint16_t *)srcp8[f];
    }

    thrsh = _mm_set1_epi16((int16_t)threshold);

    do {
        sum = _mm_set1_epi16(half_frames);

        dstpx = _mm_load_si128((__m128i *)dstp);

        sum = _mm_add_epi16(sum, dstpx);

        for (f = 1; f < frames; f++) {
            srcpf = _mm_load_si128((__m128i *)srcp[f]);

            temp0 = _mm_max_epi16(dstpx, srcpf);
            temp1 = _mm_min_epi16(dstpx, srcpf);
            temp0 = _mm_sub_epi16(temp0, temp1); // abs(dstp - srcp[f])

            temp0 = _mm_cmplt_epi16(temp0, thrsh);
            srcpf = _mm_and_si128(srcpf, temp0); // effective values of srcp[f]

            temp1 = _mm_set1_epi8((char)0xFF);
            temp0 = _mm_xor_si128(temp0, temp1); // invert temp0
            temp0 = _mm_and_si128(dstpx, temp0); // effective values of dstp

            srcpf = _mm_or_si128(srcpf, temp0); // all effective values

            sum = _mm_add_epi16(sum, srcpf);
        }

        if (frames > 2) {
            temp0 = _mm_set1_epi16(mul);
            sum = _mm_mulhi_epu16(sum, temp0);
        } else {
            sum = _mm_srai_epi16(sum, frames - 1);
        }
        _mm_store_si128((__m128i *)dstp, sum);

        dstp += 8;
        for (f = 1; f < frames; srcp[f++] += 8)
            ;
    } while ((frame_size -= 16) > 0);
}

typedef struct {
    TS_ALIGN uint16_t buff[8];
} buff_t;

void TS_FUNC_ALIGN VS_CC
ts_mode2_16bit(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold) {
    uint16_t *dstp = (uint16_t *)dstp8;
    const uint16_t *srcp[16];
    int half_frames = frames / 2;
    int f;
    uint64_t r = ((uint64_t)1 << 40) / frames;
    __m128i thrsh, dstpx, srcpf, temp0, temp1;
    buff_t buffer[16];

    for (f = 1; f < frames; f++) {
        srcp[f] = (uint16_t *)srcp8[f];
    }

    thrsh = _mm_set1_epi16((int16_t)threshold);

    do {
        dstpx = _mm_load_si128((__m128i *)dstp);
        for (f = 0; f < 8; f++) {
            buffer[0].buff[f] = dstp[f];
        }

        for (f = 1; f < frames; f++) {
            srcpf = _mm_load_si128((__m128i *)srcp[f]);

            temp0 = _mm_subs_epu16(dstpx, srcpf);
            temp1 = _mm_adds_epu16(srcpf, temp0); // max(dstpx, srcpf)
            temp0 = _mm_subs_epu16(dstpx, temp0); // min(dstpx, srcpf)
            temp1 = _mm_subs_epu16(temp1, temp0); // abs(dstp - srcp[f])

            temp0 = _mm_subs_epu16(temp1, thrsh);
            temp1 = _mm_setzero_si128();
            temp0 = _mm_cmpeq_epi16(temp0, temp1);
            srcpf = _mm_and_si128(srcpf, temp0); // effective values of srcp[f]

            temp1 = _mm_set1_epi8((char)0xFF);
            temp0 = _mm_xor_si128(temp0, temp1); // invert temp0
            temp0 = _mm_and_si128(dstpx, temp0); // effective values of dstp

            srcpf = _mm_or_si128(srcpf, temp0); // all effective values

            _mm_store_si128((__m128i *)buffer[f].buff, srcpf);
        }

        // SSE2 has no integer mulhi for 32-bit lanes, so the final reduction
        // is a scalar fixed-point multiply by the precomputed reciprocal r.
        for (int t = 0; t < 8; t++) {
            uint32_t sum = half_frames;
            for (f = 0; f < frames; sum += buffer[f++].buff[t])
                ;
            dstp[t] = (uint16_t)((sum * r + (1 << 20)) >> 40);
        }

        dstp += 8;
        for (f = 1; f < frames; srcp[f++] += 8)
            ;
    } while ((frame_size -= 16) > 0);
}
