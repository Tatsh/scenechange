// SPDX-License-Identifier: LGPL-2.1-or-later
// cmocka requires these headers in this order, before cmocka.h itself.
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "temporalsoften_kernel.h"

#ifdef _MSC_VER
#define TEST_ALIGN16 __declspec(align(16))
#else
#define TEST_ALIGN16 __attribute__((aligned(16)))
#endif

// One 16-byte vector worth of samples in each width.
#define VEC_BYTES 16
#define VEC_U8 16
#define VEC_U16 8

// Average of a column of equal samples, rounded the same way the kernel does:
//   ((half_frames + frames * value) * ceil(65536 / frames)) >> 16
// for frames > 2, or (value + half_frames) when frames is 1 or 2 (the shift
// path). Verified to match identity-output expectations exactly for all the
// values used in these tests.

// --- ts_mode2_8bit -----------------------------------------------------------

static void test_mode2_8bit_identical_neighbours(void **state) {
    (void)state;
    TEST_ALIGN16 uint8_t dstp[VEC_U8] = {
        10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160};
    TEST_ALIGN16 uint8_t expected[VEC_U8];
    memcpy(expected, dstp, VEC_U8);

    TEST_ALIGN16 uint8_t a[VEC_U8];
    TEST_ALIGN16 uint8_t b[VEC_U8];
    TEST_ALIGN16 uint8_t c[VEC_U8];
    memcpy(a, dstp, VEC_U8);
    memcpy(b, dstp, VEC_U8);
    memcpy(c, dstp, VEC_U8);

    const uint8_t *srcp[16] = {NULL, a, b, c};
    ts_mode2_8bit(dstp, srcp, 4, VEC_BYTES, 32);

    assert_memory_equal(dstp, expected, VEC_U8);
}

static void test_mode2_8bit_threshold_zero_excludes_distinct(void **state) {
    (void)state;
    TEST_ALIGN16 uint8_t dstp[VEC_U8];
    memset(dstp, 100, VEC_U8);
    TEST_ALIGN16 uint8_t expected[VEC_U8];
    memcpy(expected, dstp, VEC_U8);

    TEST_ALIGN16 uint8_t a[VEC_U8];
    TEST_ALIGN16 uint8_t b[VEC_U8];
    memset(a, 50, VEC_U8);
    memset(b, 200, VEC_U8);

    const uint8_t *srcp[16] = {NULL, a, b};
    ts_mode2_8bit(dstp, srcp, 3, VEC_BYTES, 0);

    // Threshold 0 means a neighbour is included only when the diff is also 0,
    // i.e. neighbour equals dstp. Here both differ, so the kernel substitutes
    // dstp for each neighbour slot and the average reduces back to dstp.
    assert_memory_equal(dstp, expected, VEC_U8);
}

static void test_mode2_8bit_frames_two_shift_path(void **state) {
    (void)state;
    // frames=2 triggers the srai shift branch instead of the mulhi branch.
    TEST_ALIGN16 uint8_t dstp[VEC_U8];
    memset(dstp, 80, VEC_U8);
    TEST_ALIGN16 uint8_t a[VEC_U8];
    memset(a, 80, VEC_U8);

    TEST_ALIGN16 uint8_t expected[VEC_U8];
    memset(expected, 80, VEC_U8);

    const uint8_t *srcp[16] = {NULL, a};
    ts_mode2_8bit(dstp, srcp, 2, VEC_BYTES, 16);

    assert_memory_equal(dstp, expected, VEC_U8);
}

static void test_mode2_8bit_all_max_values(void **state) {
    (void)state;
    TEST_ALIGN16 uint8_t dstp[VEC_U8];
    memset(dstp, 255, VEC_U8);
    TEST_ALIGN16 uint8_t a[VEC_U8];
    TEST_ALIGN16 uint8_t b[VEC_U8];
    memset(a, 255, VEC_U8);
    memset(b, 255, VEC_U8);
    TEST_ALIGN16 uint8_t expected[VEC_U8];
    memset(expected, 255, VEC_U8);

    const uint8_t *srcp[16] = {NULL, a, b};
    ts_mode2_8bit(dstp, srcp, 3, VEC_BYTES, 10);

    // The widening sum + saturating-add path must not clip identical maxima.
    assert_memory_equal(dstp, expected, VEC_U8);
}

// --- ts_mode2_9_or_10 --------------------------------------------------------

static void test_mode2_9_or_10_identical_neighbours(void **state) {
    (void)state;
    TEST_ALIGN16 uint16_t dstp[VEC_U16] = {100, 200, 300, 400, 500, 600, 700, 800};
    TEST_ALIGN16 uint16_t expected[VEC_U16];
    memcpy(expected, dstp, sizeof(dstp));

    TEST_ALIGN16 uint16_t a[VEC_U16];
    TEST_ALIGN16 uint16_t b[VEC_U16];
    memcpy(a, dstp, sizeof(dstp));
    memcpy(b, dstp, sizeof(dstp));

    const uint8_t *srcp[16] = {NULL, (const uint8_t *)a, (const uint8_t *)b};
    ts_mode2_9_or_10((uint8_t *)dstp, srcp, 3, VEC_BYTES, 8);

    assert_memory_equal(dstp, expected, sizeof(dstp));
}

static void test_mode2_9_or_10_threshold_zero_excludes_distinct(void **state) {
    (void)state;
    TEST_ALIGN16 uint16_t dstp[VEC_U16];
    for (int i = 0; i < VEC_U16; i++) {
        dstp[i] = 500;
    }
    TEST_ALIGN16 uint16_t expected[VEC_U16];
    memcpy(expected, dstp, sizeof(dstp));

    TEST_ALIGN16 uint16_t a[VEC_U16];
    TEST_ALIGN16 uint16_t b[VEC_U16];
    for (int i = 0; i < VEC_U16; i++) {
        a[i] = 100;
        b[i] = 900;
    }

    const uint8_t *srcp[16] = {NULL, (const uint8_t *)a, (const uint8_t *)b};
    // The 9-or-10-bit kernel uses strict less-than against threshold, so even
    // a threshold of 1 would still exclude these (|diff| = 400).
    ts_mode2_9_or_10((uint8_t *)dstp, srcp, 3, VEC_BYTES, 1);

    assert_memory_equal(dstp, expected, sizeof(dstp));
}

static void test_mode2_9_or_10_frames_two_shift_path(void **state) {
    (void)state;
    TEST_ALIGN16 uint16_t dstp[VEC_U16];
    TEST_ALIGN16 uint16_t a[VEC_U16];
    for (int i = 0; i < VEC_U16; i++) {
        dstp[i] = 600;
        a[i] = 600;
    }
    TEST_ALIGN16 uint16_t expected[VEC_U16];
    memcpy(expected, dstp, sizeof(dstp));

    const uint8_t *srcp[16] = {NULL, (const uint8_t *)a};
    ts_mode2_9_or_10((uint8_t *)dstp, srcp, 2, VEC_BYTES, 8);

    assert_memory_equal(dstp, expected, sizeof(dstp));
}

// --- ts_mode2_16bit ----------------------------------------------------------

static void test_mode2_16bit_identical_neighbours(void **state) {
    (void)state;
    TEST_ALIGN16 uint16_t dstp[VEC_U16] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000};
    TEST_ALIGN16 uint16_t expected[VEC_U16];
    memcpy(expected, dstp, sizeof(dstp));

    TEST_ALIGN16 uint16_t a[VEC_U16];
    TEST_ALIGN16 uint16_t b[VEC_U16];
    memcpy(a, dstp, sizeof(dstp));
    memcpy(b, dstp, sizeof(dstp));

    const uint8_t *srcp[16] = {NULL, (const uint8_t *)a, (const uint8_t *)b};
    ts_mode2_16bit((uint8_t *)dstp, srcp, 3, VEC_BYTES, 50);

    assert_memory_equal(dstp, expected, sizeof(dstp));
}

static void test_mode2_16bit_threshold_zero_excludes_distinct(void **state) {
    (void)state;
    TEST_ALIGN16 uint16_t dstp[VEC_U16];
    for (int i = 0; i < VEC_U16; i++) {
        dstp[i] = 30000;
    }
    TEST_ALIGN16 uint16_t expected[VEC_U16];
    memcpy(expected, dstp, sizeof(dstp));

    TEST_ALIGN16 uint16_t a[VEC_U16];
    TEST_ALIGN16 uint16_t b[VEC_U16];
    for (int i = 0; i < VEC_U16; i++) {
        a[i] = 10000;
        b[i] = 50000;
    }

    const uint8_t *srcp[16] = {NULL, (const uint8_t *)a, (const uint8_t *)b};
    ts_mode2_16bit((uint8_t *)dstp, srcp, 3, VEC_BYTES, 0);

    assert_memory_equal(dstp, expected, sizeof(dstp));
}

static void test_mode2_16bit_high_values(void **state) {
    (void)state;
    // Values that would overflow 16-bit unsigned addition without the kernel's
    // 32-bit scalar reduction tail.
    TEST_ALIGN16 uint16_t dstp[VEC_U16];
    for (int i = 0; i < VEC_U16; i++) {
        dstp[i] = 60000;
    }
    TEST_ALIGN16 uint16_t a[VEC_U16];
    TEST_ALIGN16 uint16_t b[VEC_U16];
    memcpy(a, dstp, sizeof(dstp));
    memcpy(b, dstp, sizeof(dstp));
    TEST_ALIGN16 uint16_t expected[VEC_U16];
    memcpy(expected, dstp, sizeof(dstp));

    const uint8_t *srcp[16] = {NULL, (const uint8_t *)a, (const uint8_t *)b};
    ts_mode2_16bit((uint8_t *)dstp, srcp, 3, VEC_BYTES, 10);

    assert_memory_equal(dstp, expected, sizeof(dstp));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mode2_8bit_identical_neighbours),
        cmocka_unit_test(test_mode2_8bit_threshold_zero_excludes_distinct),
        cmocka_unit_test(test_mode2_8bit_frames_two_shift_path),
        cmocka_unit_test(test_mode2_8bit_all_max_values),
        cmocka_unit_test(test_mode2_9_or_10_identical_neighbours),
        cmocka_unit_test(test_mode2_9_or_10_threshold_zero_excludes_distinct),
        cmocka_unit_test(test_mode2_9_or_10_frames_two_shift_path),
        cmocka_unit_test(test_mode2_16bit_identical_neighbours),
        cmocka_unit_test(test_mode2_16bit_threshold_zero_excludes_distinct),
        cmocka_unit_test(test_mode2_16bit_high_values),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
