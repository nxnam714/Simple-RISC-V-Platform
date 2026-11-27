#include <stdint.h>

#define MMIO32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define CALCULATOR_BASE 0x40007000u
#define HEX_BASE     0x40001000u

#define REQ_A        MMIO32(CALCULATOR_BASE + 0x00u)
#define REQ_B        MMIO32(CALCULATOR_BASE + 0x04u)
#define REQ_OP       MMIO32(CALCULATOR_BASE + 0x08u)
#define REQ_SEQ      MMIO32(CALCULATOR_BASE + 0x0cu)
#define RES_VALUE    MMIO32(CALCULATOR_BASE + 0x10u)
#define RES_STATUS   MMIO32(CALCULATOR_BASE + 0x14u)
#define RES_SEQ      MMIO32(CALCULATOR_BASE + 0x18u)
#define HEX(index)   MMIO32(HEX_BASE + 4u * (uint32_t)(index))

#define STATUS_OK      0u
#define STATUS_DIV0    1u
#define STATUS_BAD_REQ 2u

static uint32_t magnitude(int32_t value)
{
    if (value < 0) return (uint32_t)(-value);
    return (uint32_t)value;
}

static int32_t signed_byte(uint32_t value)
{
    if ((value & 0x80u) != 0u) {
        return (int32_t)(value & 0x7fu) - 128;
    }
    return (int32_t)value;
}

static uint32_t multiply_magnitudes(uint32_t left, uint32_t right)
{
    uint32_t product = 0u;

    while (right != 0u) {
        if ((right & 1u) != 0u) product += left;
        left <<= 1;
        right >>= 1;
    }
    return product;
}

static int32_t signed_multiply(int32_t left, int32_t right)
{
    uint32_t product = multiply_magnitudes(magnitude(left), magnitude(right));

    if ((left < 0) != (right < 0)) return -(int32_t)product;
    return (int32_t)product;
}

static int32_t signed_divide(int32_t numerator, int32_t denominator)
{
    uint32_t remainder = magnitude(numerator);
    uint32_t divisor = magnitude(denominator);
    uint32_t quotient = 0u;

    while (remainder >= divisor) {
        remainder -= divisor;
        ++quotient;
    }

    if ((numerator < 0) != (denominator < 0)) return -(int32_t)quotient;
    return (int32_t)quotient;
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
    if (digit == 11u) return 0x40u;
    return 0x79u;
}

static void display_digit(uint32_t position, uint32_t digit)
{
    HEX(position) = seven_segment_pattern(digit);
}

static void display_error(void)
{
    uint32_t position;

    for (position = 0u; position < 6u; ++position) {
        display_digit(position, 10u);
    }
    display_digit(0u, 12u);
}

static void display_result(int32_t result)
{
    uint32_t value = magnitude(result);
    uint32_t position;
    uint32_t have_digit = 0u;

    for (position = 0u; position < 5u; ++position) {
        uint32_t digit = 0u;
        uint32_t divisor;

        if (position == 0u) divisor = 10000u;
        else if (position == 1u) divisor = 1000u;
        else if (position == 2u) divisor = 100u;
        else if (position == 3u) divisor = 10u;
        else divisor = 1u;

        while (value >= divisor) {
            value -= divisor;
            ++digit;
        }

        if (digit != 0u || have_digit != 0u || position == 4u) {
            display_digit(4u - position, digit);
            have_digit = 1u;
        }
        else {
            display_digit(4u - position, 10u);
        }
    }

    display_digit(5u, result < 0 ? 11u : 10u);
}

static int32_t calculate(
    int32_t operand_a,
    int32_t operand_b,
    uint32_t operation,
    uint32_t *status
)
{
    *status = STATUS_OK;

    switch (operation) {
    case 0u:
        return operand_a + operand_b;
    case 1u:
        return operand_a - operand_b;
    case 2u:
        return signed_multiply(operand_a, operand_b);
    case 3u:
        if (operand_b == 0) {
            *status = STATUS_DIV0;
            return 0;
        }
        return signed_divide(operand_a, operand_b);
    default:
        *status = STATUS_BAD_REQ;
        return 0;
    }
}

void calculator_main(void)
{
    for (;;) {
        uint32_t request_sequence = REQ_SEQ;
        uint32_t completed_sequence = RES_SEQ;
        uint32_t raw_a;
        uint32_t raw_b;
        uint32_t operation;
        uint32_t status;
        int32_t operand_a;
        int32_t operand_b;
        int32_t result;

        if (request_sequence == completed_sequence) continue;

        raw_a = REQ_A;
        raw_b = REQ_B;
        operation = REQ_OP;

        if ((raw_a > 127u && raw_a < 0xffffff80u) ||
            (raw_b > 127u && raw_b < 0xffffff80u) ||
            operation > 3u) {
            result = 0;
            status = STATUS_BAD_REQ;
        }
        else {
            operand_a = signed_byte(raw_a);
            operand_b = signed_byte(raw_b);
            result = calculate(operand_a, operand_b, operation, &status);
        }

        RES_VALUE = (uint32_t)result;
        RES_STATUS = status;
        if (status == STATUS_OK) display_result(result);
        else display_error();
        RES_SEQ = request_sequence;
    }
}
