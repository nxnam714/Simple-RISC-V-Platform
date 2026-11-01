/* This file is based on the CoreMark source code provided by EEMBC. */

#include "custom_printf.h"

#include <stdarg.h>
#include <stdint.h>

#define SIM_CONSOLE (*(volatile uint8_t *)0x8008fff0u)

static void emit_character(char character, int *count) {
    uart_send_char(character);
    ++*count;
}

void uart_send_char(char character) {
    SIM_CONSOLE = (uint8_t)character;
}

static void emit_padding(char character, int amount, int *count) {
    while (amount-- > 0) emit_character(character, count);
}

static int string_length(const char *string, int precision) {
    int length = 0;

    while (string[length] != '\0' && (precision < 0 || length < precision)) ++length;
    return length;
}

static void emit_string(const char *string, int width, int precision, int left, int *count) {
    int length;

    if (string == 0) string = "(null)";
    length = string_length(string, precision);
    if (!left) emit_padding(' ', width - length, count);
    for (int index = 0; index < length; ++index) emit_character(string[index], count);
    if (left) emit_padding(' ', width - length, count);
}

static void emit_number(uint32_t value, int negative, uint32_t base, int width,
                        int precision, int left, int zero, int plus, int space,
                        int alternate, int uppercase, int *count)
{

    char digits[32];
    int digit_count = 0;
    int prefix_count = 0;
    char sign = 0;
    char prefix_character = uppercase ? 'X' : 'x';

    if (negative) sign = '-';
    else if (plus) sign = '+';
    else if (space) sign = ' ';

    if (value == 0u) digits[digit_count++] = '0';
    while (value != 0u) {
        uint32_t digit = value % base;
        digits[digit_count++] = (char)(digit < 10u ? '0' + digit :
                                      (uppercase ? 'A' : 'a') + digit - 10u);
        value /= base;
    }

    if (precision < digit_count) precision = digit_count;
    if (alternate && base == 16u) prefix_count = 2;
    else if (alternate && base == 8u) prefix_count = 1;

    int content_width = precision + prefix_count + (sign != 0);
    int padding = width > content_width ? width - content_width : 0;

    if (!left && !zero) emit_padding(' ', padding, count);
    if (sign != 0) emit_character(sign, count);
    if (prefix_count == 2) {
        emit_character('0', count);
        emit_character(prefix_character, count);
    }
    else if (prefix_count == 1) emit_character('0', count);
    if (!left && zero) emit_padding('0', padding, count);
    emit_padding('0', precision - digit_count, count);
    while (digit_count-- > 0) emit_character(digits[digit_count], count);
    if (left) emit_padding(' ', padding, count);
}

static double double_from_words(uint32_t high_word, uint32_t low_word) {
    union {
        double value;
        uint32_t words[2];
    } conversion;

    conversion.words[0] = low_word;
    conversion.words[1] = high_word;
    return conversion.value;
}

static void emit_fixed(double value, int width, int precision, int left, int zero,
                       int plus, int space, int *count)
{
    char buffer[64];
    char reversed[16];
    int length = 0;
    int integer_digits = 0;
    int negative = value < 0.0;
    volatile uint32_t ten_integer = 10u;
    volatile uint32_t half_high_word = 0x3fe00000u;
    volatile uint32_t half_low_word = 0x00000000u;
    volatile uint32_t tenth_high_word = 0x3fb99999u;
    volatile uint32_t tenth_low_word = 0x9999999au;
    double ten = (double)ten_integer;
    double rounding = double_from_words(half_high_word, half_low_word);
    double tenth = double_from_words(tenth_high_word, tenth_low_word);
    uint32_t integer_part;
    double fractional_part;

    if (precision < 0) precision = 6;
    if (precision > 12) precision = 12;
    if (negative) value = -value;
    for (int digit = 0; digit < precision; ++digit) rounding *= tenth;
    value += rounding;
    integer_part = (uint32_t)value;
    fractional_part = value - (double)integer_part;

    if (negative) buffer[length++] = '-';
    else if (plus) buffer[length++] = '+';
    else if (space) buffer[length++] = ' ';

    if (integer_part == 0u) reversed[integer_digits++] = '0';
    while (integer_part != 0u) {
        reversed[integer_digits++] = (char)('0' + integer_part % 10u);
        integer_part /= 10u;
    }
    while (integer_digits-- > 0) buffer[length++] = reversed[integer_digits];

    if (precision != 0) buffer[length++] = '.';
    for (int digit = 0; digit < precision; ++digit) {
        uint32_t decimal_digit;
        fractional_part *= ten;
        decimal_digit = (uint32_t)fractional_part;
        if (decimal_digit > 9u) decimal_digit = 9u;
        buffer[length++] = (char)('0' + decimal_digit);
        fractional_part -= (double)decimal_digit;
    }

    int padding = width > length ? width - length : 0;
    if (!left) emit_padding(zero ? '0' : ' ', padding, count);
    for (int index = 0; index < length; ++index) emit_character(buffer[index], count);
    if (left) emit_padding(' ', padding, count);
}

int ee_printf(const char *format, ...)
{
    va_list arguments;
    int count = 0;

    va_start(arguments, format);
    while (*format != '\0') {
        int left = 0;
        int plus = 0;
        int space = 0;
        int alternate = 0;
        int zero = 0;
        int width = 0;
        int precision = -1;
        int long_value = 0;

        if (*format != '%') {
            emit_character(*format++, &count);
            continue;
        }
        ++format;

        for (;;) {
            if (*format == '-') left = 1;
            else if (*format == '+') plus = 1;
            else if (*format == ' ') space = 1;
            else if (*format == '#') alternate = 1;
            else if (*format == '0') zero = 1;
            else break;
            ++format;
        }

        if (*format == '*') {
            width = va_arg(arguments, int);
            ++format;
            if (width < 0) { left = 1; width = -width; }
        }
        else {
            while (*format >= '0' && *format <= '9') width = width * 10 + *format++ - '0';
        }

        if (*format == '.') {
            ++format;
            precision = 0;
            if (*format == '*') {
                precision = va_arg(arguments, int);
                ++format;
            }
            else {
                while (*format >= '0' && *format <= '9')
                    precision = precision * 10 + *format++ - '0';
            }
        }

        if (*format == 'l' || *format == 'L') {
            long_value = 1;
            ++format;
        }

        char conversion = *format == '\0' ? '\0' : *format++;
        if (conversion == '%') emit_character('%', &count);
        else if (conversion == 'c') {
            char character = (char)va_arg(arguments, int);
            if (!left) emit_padding(' ', width - 1, &count);
            emit_character(character, &count);
            if (left) emit_padding(' ', width - 1, &count);
        }
        else if (conversion == 's') {
            emit_string(va_arg(arguments, const char *), width, precision, left, &count);
        }
        else if (conversion == 'd' || conversion == 'i') {
            int32_t signed_value = long_value ? (int32_t)va_arg(arguments, long) :
                                                (int32_t)va_arg(arguments, int);
            uint32_t magnitude = signed_value < 0 ? 0u - (uint32_t)signed_value :
                                                   (uint32_t)signed_value;
            emit_number(magnitude, signed_value < 0, 10u, width, precision, left, zero,
                        plus, space, alternate, 0, &count);
        }
        else if (conversion == 'u' || conversion == 'x' || conversion == 'X' ||
                 conversion == 'o' || conversion == 'p') {
            uint32_t value;
            uint32_t base = conversion == 'o' ? 8u :
                            (conversion == 'u' ? 10u : 16u);
            if (conversion == 'p') {
                value = (uint32_t)(uintptr_t)va_arg(arguments, void *);
                alternate = 1;
            }
            else {
                value = long_value ? (uint32_t)va_arg(arguments, unsigned long) :
                                     va_arg(arguments, uint32_t);
            }
            emit_number(value, 0, base, width, precision, left, zero, plus, space,
                        alternate, conversion == 'X', &count);
        }
        else if (conversion == 'f') {
            emit_fixed(va_arg(arguments, double), width, precision, left, zero, plus,
                       space, &count);
        }
        else if (conversion != '\0') {
            emit_character('%', &count);
            emit_character(conversion, &count);
        }
    }
    va_end(arguments);
    return count;
}
