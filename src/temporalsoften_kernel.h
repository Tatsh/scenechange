// SPDX-License-Identifier: LGPL-2.1-or-later
/**
 * @file temporalsoften_kernel.h
 * @brief Per-architecture temporal soften kernel interface.
 *
 * The @c temporalsoften2 plugin extracts its inner SIMD kernel into one source
 * file per ISA backend (for example @c temporalsoften_x86.c for SSE2). All
 * backends export the same three functions declared here; the host
 * @c temporalsoften.c dispatches to whichever set the build system chose at
 * configure time based on @c CMAKE_SYSTEM_PROCESSOR.
 *
 * Each function processes one plane of a single frame in place, blending the
 * destination plane with up to fifteen reference planes (one per temporal
 * neighbour) using mode 2 from Avisynth's @c TemporalSoften filter:
 *
 *   - For every output sample, the kernel includes a neighbour sample in the
 *     average only when @c |dst - src| @<= @c threshold; otherwise the original
 *     destination value is substituted.
 *   - The final per-sample average is computed with rounding, using a
 *     precomputed fixed-point reciprocal multiplier in place of integer
 *     division.
 *
 * @par Buffer contract
 * - @c dstp (or @c dstp8 for the 16-bit variants) points to the writeable
 *   destination plane and is mutated in place.
 * - @c srcp (or @c srcp8) is an array of read-only plane pointers. The
 *   destination plane is referenced through @c dstp, not through @c srcp[0];
 *   the kernel iterates @c srcp[1..frames-1] as temporal neighbours.
 * - @c frame_size is the plane size in bytes and must be a positive multiple
 *   of 16 (one SSE register).
 * - All input and output pointers must be 16-byte aligned. VapourSynth's plane
 *   pointers satisfy this for compliant hosts.
 *
 * @par Threshold scaling
 * - @c threshold for @ref ts_mode2_8bit is in 8-bit native units (0..255).
 * - @c threshold for @ref ts_mode2_9_or_10 is in the native bit depth of the
 *   clip (so 0..511 for 9-bit, 0..1023 for 10-bit).
 * - @c threshold for @ref ts_mode2_16bit is in 16-bit native units (0..65535).
 *
 * @par Scene change behaviour
 * The kernel is invoked with whatever @c frames count the caller selected; if
 * scene change properties have already pruned neighbours, the caller passes a
 * smaller @c frames value and the kernel adapts automatically.
 *
 * @see temporalsoften.c for parameter parsing and dispatch.
 * @see temporalsoften_x86.c for the SSE2 implementation.
 */
#ifndef SCENECHANGE_TEMPORALSOFTEN_KERNEL_H
#define SCENECHANGE_TEMPORALSOFTEN_KERNEL_H

#include <stdint.h>

#include <VapourSynth.h>

/**
 * @brief Apply mode-2 temporal soften to an 8-bit plane.
 *
 * @param[in,out] dstp        Destination plane, mutated in place. Must be
 *                            16-byte aligned and at least @p frame_size bytes.
 * @param[in]     srcp        Array of read-only neighbour plane pointers.
 *                            Only @p srcp[1..frames-1] is read; @p srcp[0] is
 *                            ignored because the destination is read through
 *                            @p dstp. Each pointer must be 16-byte aligned.
 *                            The array is mutated internally as the kernel
 *                            advances the per-neighbour cursors.
 * @param[in]     frames      Total number of source frames being blended,
 *                            including the centre frame. Range 1..16.
 * @param[in]     frame_size  Plane size in bytes. Must be a positive multiple
 *                            of 16.
 * @param[in]     threshold   Maximum absolute difference at which a neighbour
 *                            sample is included in the average. Range 0..255.
 */
void VS_CC
ts_mode2_8bit(uint8_t *dstp, const uint8_t **srcp, int frames, int frame_size, int threshold);

/**
 * @brief Apply mode-2 temporal soften to a 9- or 10-bit plane.
 *
 * The 9- and 10-bit cases share an implementation because their samples fit
 * in a signed 16-bit lane without saturation when accumulated.
 *
 * @param[in,out] dstp8       Destination plane, mutated in place. The bytes
 *                            are interpreted as a row of @c uint16_t samples
 *                            in native byte order. Must be 16-byte aligned and
 *                            at least @p frame_size bytes.
 * @param[in]     srcp8       Array of read-only neighbour plane pointers.
 *                            Same contract as @ref ts_mode2_8bit.
 * @param[in]     frames      Total number of source frames being blended.
 *                            Range 1..16.
 * @param[in]     frame_size  Plane size in bytes. Must be a positive multiple
 *                            of 16.
 * @param[in]     threshold   Maximum absolute difference at which a neighbour
 *                            sample is included in the average, expressed in
 *                            the native bit depth of the clip.
 */
void VS_CC
ts_mode2_9_or_10(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold);

/**
 * @brief Apply mode-2 temporal soften to a 16-bit plane.
 *
 * The 16-bit case cannot use a vector @c mulhi for the final divide because
 * SSE2 has no integer mulhi for 32-bit lanes; the scalar tail at the end of
 * the function performs the per-sample average with a 64-bit fixed-point
 * reciprocal multiply.
 *
 * @param[in,out] dstp8       Destination plane, mutated in place. The bytes
 *                            are interpreted as a row of @c uint16_t samples
 *                            in native byte order.
 * @param[in]     srcp8       Array of read-only neighbour plane pointers.
 *                            Same contract as @ref ts_mode2_8bit.
 * @param[in]     frames      Total number of source frames being blended.
 *                            Range 1..16.
 * @param[in]     frame_size  Plane size in bytes. Must be a positive multiple
 *                            of 16.
 * @param[in]     threshold   Maximum absolute difference at which a neighbour
 *                            sample is included in the average. Range 0..65535.
 */
void VS_CC
ts_mode2_16bit(uint8_t *dstp8, const uint8_t **srcp8, int frames, int frame_size, int threshold);

#endif
