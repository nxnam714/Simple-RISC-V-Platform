#ifndef M2_APP_RUNTIME_H
#define M2_APP_RUNTIME_H

#include <stdint.h>
#include "custom_printf.h"

#define SIM_EXIT    (*(volatile uint32_t *)0x4000f008u)

__attribute__((noinline)) int __clzsi2(uint32_t value)
{
    int count = 0;

    if (value == 0u) return 32;
    while ((value & 0x80000000u) == 0u) {
        value <<= 1;
        ++count;
    }
    return count;
}

static double double_from_words(uint32_t high_word, uint32_t low_word)
{
    union {
        double value;
        uint32_t words[2];
    } conversion;

    conversion.words[0] = low_word;
    conversion.words[1] = high_word;
    return conversion.value;
}

static uint32_t multiply_u32(uint32_t left, uint32_t right)
{
    uint32_t result = 0u;

    while (right != 0u) {
        if ((right & 1u) != 0u) result += left;
        left <<= 1;
        right >>= 1;
    }
    return result;
}

static int32_t multiply_i32(int32_t left, int32_t right)
{
    uint32_t left_magnitude;
    uint32_t right_magnitude;
    uint32_t result;
    uint32_t negative = 0u;

    if (left < 0) {
        left_magnitude = 0u - (uint32_t)left;
        negative ^= 1u;
    }
    else {
        left_magnitude = (uint32_t)left;
    }
    if (right < 0) {
        right_magnitude = 0u - (uint32_t)right;
        negative ^= 1u;
    }
    else {
        right_magnitude = (uint32_t)right;
    }

    result = multiply_u32(left_magnitude, right_magnitude);
    return negative != 0u ? (int32_t)(0u - result) : (int32_t)result;
}

__attribute__((noreturn)) static void app_finish(void)
{
    SIM_EXIT = 1u;
    for (;;) __asm__ volatile ("j .");
}

#endif
