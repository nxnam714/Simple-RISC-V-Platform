#include <stdint.h>

#include "../mcu.h"

#define INPUT_DIGITS 4u
#define RESULT_DIGITS 20u
#define RESULT_PAGE_DIGITS 6u
#define STATUS_INPUT 0u
#define STATUS_RESULT 1u
#define STATUS_ERROR 2u

static uint32_t decimal_place(uint32_t index)
{
    if (index == 0u) return 1u;
    if (index == 1u) return 10u;
    if (index == 2u) return 100u;
    return 1000u;
}

static __attribute__((noinline)) uint64_t multiply_u64_u32(
    uint64_t value,
    uint32_t factor
)
{
    uint64_t product = 0u;

    while (factor != 0u) {
        if ((factor & 1u) != 0u) {
            product += value;
        }
        value <<= 1;
        factor >>= 1;
    }

    return product;
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
    if (digit == 9u) return 0x6fu;
    if (digit == 10u) return 0x00u;
    return 0x79u;
}

static void display_digit(uint32_t index, uint32_t digit)
{
    HEX(index) = seven_segment_pattern(digit);
}

static void display_input(uint32_t input)
{
    uint32_t digit;

    for (digit = 0u; digit < INPUT_DIGITS; ++digit) {
        display_digit(digit, decimal_digit(input, digit));
    }
    LEDR = input;
}

static void display_error(void)
{
    uint32_t digit;

    display_digit(0u, 11u);
    for (digit = 1u; digit < RESULT_PAGE_DIGITS; ++digit) {
        display_digit(digit, 10u);
    }
}

static uint64_t factorial(uint32_t input)
{
    uint64_t result = 1u;
    uint32_t factor;

    for (factor = 2u; factor <= input; ++factor) {
        result = multiply_u64_u32(result, factor);
    }

    return result;
}

static uint64_t fibonacci(uint32_t input)
{
    uint64_t previous = 0u;
    uint64_t current = 1u;
    uint32_t index;

    for (index = 0u; index < input; ++index) {
        uint64_t next = previous + current;
        previous = current;
        current = next;
    }

    return previous;
}

static uint64_t divide_by_ten(uint64_t value, uint32_t *remainder)
{
    uint64_t quotient = 0u;
    uint32_t bit;
    uint32_t rem = 0u;

    for (bit = 64u; bit != 0u; --bit) {
        rem = (rem << 1) | (uint32_t)((value >> (bit - 1u)) & 1u);
        quotient <<= 1;
        if (rem >= 10u) {
            rem -= 10u;
            quotient |= 1u;
        }
    }

    *remainder = rem;
    return quotient;
}

static void to_decimal_digits(uint64_t value, uint8_t digits[RESULT_DIGITS])
{
    uint32_t index;

    for (index = 0u; index < RESULT_DIGITS; ++index) {
        uint32_t remainder;
        value = divide_by_ten(value, &remainder);
        digits[index] = (uint8_t)remainder;
    }
}

static void display_result_page(
    const uint8_t digits[RESULT_DIGITS],
    uint32_t page
)
{
    uint32_t position;
    uint32_t first_digit = page * RESULT_PAGE_DIGITS;

    for (position = 0u; position < RESULT_PAGE_DIGITS; ++position) {
        uint32_t digit_index = first_digit + position;
        uint32_t digit = digit_index < RESULT_DIGITS
            ? digits[digit_index]
            : 10u;
        display_digit(position, digit);
    }
}

static void increment_selected_digit(uint32_t *input, uint32_t index)
{
    uint32_t place = decimal_place(index);
    uint32_t digit = decimal_digit(*input, index);

    if (digit == 9u) {
        *input -= 9u * place;
    }
    else {
        *input += place;
    }
    display_input(*input);
}

int main(void)
{
    uint32_t input = 0u;
    uint32_t previous_keys = 0u;
    uint32_t mode = STATUS_INPUT;
    uint32_t displayed_page = 0xffffffffu;
    uint8_t result_digits[RESULT_DIGITS];

    display_input(input);

    for (;;) {
        uint32_t keys = KEY & 0x0fu;
        uint32_t pressed = keys & ~previous_keys;
        uint32_t switches = SW;
        uint32_t selected_input_digit = switches & 0x03u;
        uint32_t selected_result_page = (switches >> 2) & 0x07u;

        previous_keys = keys;

        if ((pressed & 0x02u) != 0u) {
            increment_selected_digit(&input, selected_input_digit);
            mode = STATUS_INPUT;
        }

        if ((pressed & 0x04u) != 0u) {
            if (input <= 20u) {
                to_decimal_digits(factorial(input), result_digits);
                mode = STATUS_RESULT;
                displayed_page = 0xffffffffu;
            }
            else {
                display_error();
                mode = STATUS_ERROR;
            }
        }

        if ((pressed & 0x08u) != 0u) {
            if (input <= 93u) {
                to_decimal_digits(fibonacci(input), result_digits);
                mode = STATUS_RESULT;
                displayed_page = 0xffffffffu;
            }
            else {
                display_error();
                mode = STATUS_ERROR;
            }
        }

        if (mode == STATUS_RESULT && selected_result_page != displayed_page) {
            display_result_page(result_digits, selected_result_page);
            displayed_page = selected_result_page;
        }
    }
}
