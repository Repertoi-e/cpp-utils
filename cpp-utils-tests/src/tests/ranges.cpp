#pragma once

#include "../test.h"

#include <cppu/format/fmt.h>

template <size_t N>
void test_expected(Array<s32, N> expected, s64 start, s64 stop, s64 step = 1) {
    Dynamic_Array<s32> result;
    for (s32 i : range(start, stop, step)) {
        result.add(i);
    }
    assert_eq(result, expected);
}

TEST(basic) {
    test_expected(to_array<s32>(0, 1, 2, 3, 4), 0, 5);
    test_expected(to_array<s32>(-3, -2, -1, 0, 1), -3, 2);
}

TEST(variable_steps) {
    Dynamic_Array<s32> result;
    for (s32 i : range(2, -3, 2)) {
        result.add(i);
    }
    assert_eq(result.Count, 0);

    test_expected(to_array<s32>(-3, -1, 1), -3, 2, 2);
    test_expected(to_array<s32>(10, 13), 10, 15, 3);

    test_expected(to_array<s32>(2, 4, 6, 8), 2, 10, 2);
}

TEST(reversed) {
    test_expected(to_array<s32>(5, 4, 3, 2, 1), 5, 0, -1);
    test_expected(to_array<s32>(2, 1, 0, -1, -2), 2, -3, -1);

    test_expected(to_array<s32>(2, 0, -2), 2, -3, -2);
    test_expected(to_array<s32>(15, 12), 15, 10, -3);

    test_expected(to_array<s32>(10, 8, 6, 4), 10, 2, -2);
}