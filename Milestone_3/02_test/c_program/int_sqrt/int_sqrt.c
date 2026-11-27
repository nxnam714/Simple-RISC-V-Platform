#include <stdint.h>

#include "../mcu.h"

#define DIGIT_COUNT 4u

static uint32_t multiply_u32(uint32_t left, uint32_t right)
{
    uint32_t product = 0u;

    while (right != 0u) {
        if ((right & 1u) != 0u) {
            product += left;
        }
        left <<= 1;
        right >>= 1;
    }

    return product;
}

static uint32_t integer_square_root_scaled(uint32_t value)
{
    uint32_t low = 0u;
    uint32_t high = 10000u;
    uint32_t result = 0u;

    value = multiply_u32(value, 10000u);
    while (low <= high) {
        uint32_t middle = (low + high) / 2u;
        uint32_t square = multiply_u32(middle, middle);

        if (square <= value) {
            result = middle;
            low = middle + 1u;
        }
        else {
            high = middle - 1u;
        }
    }

    if (result < 10000u) {
        uint32_t lower_error = value - multiply_u32(result, result);
        uint32_t next = result + 1u;
        uint32_t upper_error = multiply_u32(next, next) - value;

        if (upper_error < lower_error) {
            result = next;
        }
    }

    return result;
}

static uint32_t decimal_place(uint32_t index)
{
    if (index == 0u) return 1u;
    if (index == 1u) return 10u;
    if (index == 2u) return 100u;
    return 1000u;
}

static uint32_t decimal_digit(uint32_t value, uint32_t index)
{
    uint32_t place = decimal_place(index);
    uint32_t digit = 0u;

    while (value >= 10u * place) {
        value -= 10u * place;
    }
    while (value >= place) {
        value -= place;
        ++digit;
    }

    return digit;
}

static uint32_t seven_segment_pattern(uint32_t digit)
{
    if (digit == 0u) return 0x3fu;
    if (digit == 1u) return 0x06u;
    if (digit == 2u) return 0x5bu;
    if (digit == 3u) return 0x4fu;
    if (digit == 4u) return 0x66u;
    if (digit == 5u) return 0x6du;
    if (digit == 6u) return 0x7du;
    if (digit == 7u) return 0x07u;
    if (digit == 8u) return 0x7fu;
    return 0x6fu;
}

static void display_digit(uint32_t index, uint32_t value)
{
    HEX(index) = seven_segment_pattern(value);
}

static void display_value(uint32_t value)
{
    uint32_t digit;

    for (digit = 0u; digit < DIGIT_COUNT; ++digit) {
        display_digit(digit, decimal_digit(value, digit));
    }
}

static void increment_selected_digit(uint32_t *value, uint32_t index)
{
    uint32_t power = decimal_place(index);
    uint32_t digit = decimal_digit(*value, index);

    if (digit == 9u) {
        *value -= 9u * power;
    }
    else {
        *value += power;
    }

    display_value(*value);
    LEDR = *value;
}

static void clear_display(void)
{
    uint32_t digit;

    for (digit = 0u; digit < DIGIT_COUNT; ++digit) {
        HEX(digit) = 0u;
    }
    LEDR = 0u;
}

int main(void)
{
    uint32_t input_value = 0u;
    uint32_t previous_keys = 0u;

    clear_display();

    for (;;) {
        uint32_t keys = KEY & 0x0fu;
        uint32_t pressed = keys & ~previous_keys;
        uint32_t selected_digit = SW & 0x03u;

        previous_keys = keys;

        if ((pressed & 0x02u) != 0u) {
            increment_selected_digit(&input_value, selected_digit);
        }

        if ((pressed & 0x04u) != 0u) {
            display_value(integer_square_root_scaled(input_value));
        }
    }
}
