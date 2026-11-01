#include "app_runtime.h"

#define OP_ADD  0u
#define OP_SUB  1u
#define OP_MUL  2u
#define OP_DIV  3u
#define OP_SQRT 4u
#define CASE_COUNT 14u

typedef struct {
    const char *name;
    uint32_t operation;
    uint16_t left;
    uint16_t right;
    uint16_t expected;
} small_case_t;

typedef struct {
    const char *name;
    uint32_t operation;
    uint32_t left;
    uint32_t right;
    uint32_t expected;
} f32_case_t;

typedef struct {
    const char *name;
    uint32_t operation;
    uint64_t left;
    uint64_t right;
    uint64_t expected;
} f64_case_t;

static float float_from_bits(uint32_t bits);
static uint32_t float_bits(float value);
static double double_from_bits(uint64_t bits);
static uint64_t double_bits(double value);
static uint16_t float_to_binary16(float value);
static float binary16_to_float(uint16_t value);
static uint16_t float_to_bfloat16(float value);
static float bfloat16_to_float(uint16_t value);
static float square_root32(float value);
static double square_root64(double value);
static const char *operation_name(uint32_t operation);
static float calculate32(uint32_t operation, float left, float right);
static double calculate64(uint32_t operation, double left, double right);
static void print_all_inputs(void);
static void print_binary16_inputs(void);
static void print_bfloat16_inputs(void);
static void print_binary32_inputs(void);
static void print_binary64_inputs(void);
static void run_all_tests(volatile uint32_t *passed, volatile uint32_t *total);
static void run_binary16_tests(volatile uint32_t *passed, volatile uint32_t *total);
static void run_bfloat16_tests(volatile uint32_t *passed, volatile uint32_t *total);
static void run_binary32_tests(volatile uint32_t *passed, volatile uint32_t *total);
static void run_binary64_tests(volatile uint32_t *passed, volatile uint32_t *total);
static void print_small_input(const char *format, const small_case_t *test, float left, float right, uint32_t edge_case);
static void print_f32_input(const f32_case_t *test, float left, float right, uint32_t edge_case);
static void print_f64_input(const f64_case_t *test, double left, double right, uint32_t edge_case);
static void check16(const char *format, const char *name, uint16_t actual, uint16_t expected, volatile uint32_t *passed, volatile uint32_t *total);
static void check32(const char *name, uint32_t actual, uint32_t expected, volatile uint32_t *passed, volatile uint32_t *total);
static void check64(const char *name, uint64_t actual, uint64_t expected, volatile uint32_t *passed, volatile uint32_t *total);

static const small_case_t binary16_cases[CASE_COUNT] = {
    {"ADD.NORMAL",    OP_ADD,  0x3e00u,0x4080u,0x4380u},
    {"ADD.SIGNED",    OP_ADD,  0xcac0u,0x4440u,0xc8a0u},
    {"SUB.SIGNED",    OP_SUB,  0x57bcu,0xc840u,0x5822u},
    {"SUB.NEGATIVE",  OP_SUB,  0xb800u,0x3a00u,0xbd00u},
    {"SUB.CLOSE",     OP_SUB,  0x63d0u,0x63cfu,0x3800u},
    {"MUL.SIGNED",    OP_MUL,  0xcc60u,0x48a0u,0xd90fu},
    {"MUL.FRACTION",  OP_MUL,  0x3000u,0x2c00u,0x2000u},
    {"DIV.INEXACT",   OP_DIV,  0x63b7u,0x4fd0u,0x4fe6u},
    {"DIV.EXACT",     OP_DIV,  0x4700u,0x4c00u,0x3700u},
    {"SQRT.TWO",      OP_SQRT, 0x4000u,0x0000u,0x3da8u},
    {"SQRT.INEXACT",  OP_SQRT, 0x57b4u,0x0000u,0x498du},
    {"SQRT.FRACTION", OP_SQRT, 0x2c00u,0x0000u,0x3400u},
    {"MUL.OVERFLOW",  OP_MUL,  0x7bffu,0x4000u,0x7c00u},
    {"DIV.SUBNORMAL", OP_DIV,  0x0400u,0x4000u,0x0200u}
};

static const small_case_t bfloat16_cases[CASE_COUNT] = {
    {"ADD.NORMAL",    OP_ADD,  0x3fc0u,0x4010u,0x4070u},
    {"ADD.SIGNED",    OP_ADD,  0xc158u,0x4088u,0xc114u},
    {"SUB.SIGNED",    OP_SUB,  0x42f8u,0xc108u,0x4304u},
    {"SUB.NEGATIVE",  OP_SUB,  0xbf00u,0x3f40u,0xbfa0u},
    {"SUB.CLOSE",     OP_SUB,  0x447au,0x447au,0x0000u},
    {"MUL.SIGNED",    OP_MUL,  0xc18cu,0x4114u,0xc322u},
    {"MUL.FRACTION",  OP_MUL,  0x3e00u,0x3d80u,0x3c00u},
    {"DIV.INEXACT",   OP_DIV,  0x4477u,0x41fau,0x41fdu},
    {"DIV.EXACT",     OP_DIV,  0x40e0u,0x4180u,0x3ee0u},
    {"SQRT.TWO",      OP_SQRT, 0x4000u,0x0000u,0x3fb5u},
    {"SQRT.INEXACT",  OP_SQRT, 0x42f6u,0x0000u,0x4131u},
    {"SQRT.FRACTION", OP_SQRT, 0x3d80u,0x0000u,0x3e80u},
    {"MUL.OVERFLOW",  OP_MUL,  0x7f7fu,0x4000u,0x7f80u},
    {"DIV.SUBNORMAL", OP_DIV,  0x0080u,0x4000u,0x0040u}
};

static const f32_case_t binary32_cases[CASE_COUNT] = {
    {"ADD.NORMAL",    OP_ADD,  0x3fc00000u,0x40100000u,0x40700000u},
    {"ADD.SIGNED",    OP_ADD,  0xc1580000u,0x40880000u,0xc1140000u},
    {"SUB.SIGNED",    OP_SUB,  0x42f78000u,0xc1080000u,0x43044000u},
    {"SUB.NEGATIVE",  OP_SUB,  0xbf000000u,0x3f400000u,0xbfa00000u},
    {"SUB.CLOSE",     OP_SUB,  0x447a0000u,0x4479e000u,0x3f000000u},
    {"MUL.SIGNED",    OP_MUL,  0xc18c0000u,0x41140000u,0xc321e000u},
    {"MUL.FRACTION",  OP_MUL,  0x3e000000u,0x3d800000u,0x3c000000u},
    {"DIV.INEXACT",   OP_DIV,  0x4476e000u,0x41fa0000u,0x41fccccdu},
    {"DIV.EXACT",     OP_DIV,  0x40e00000u,0x41800000u,0x3ee00000u},
    {"SQRT.TWO",      OP_SQRT, 0x40000000u,0x00000000u,0x3fb504f3u},
    {"SQRT.INEXACT",  OP_SQRT, 0x42f68000u,0x00000000u,0x4131a0fbu},
    {"SQRT.FRACTION", OP_SQRT, 0x3d800000u,0x00000000u,0x3e800000u},
    {"MUL.OVERFLOW",  OP_MUL,  0x7f7fffffu,0x40000000u,0x7f800000u},
    {"DIV.SUBNORMAL", OP_DIV,  0x00800000u,0x40000000u,0x00400000u}
};

static const f64_case_t binary64_cases[CASE_COUNT] = {
    {"ADD.NORMAL",    OP_ADD,  0x3ff8000000000000ULL,0x4002000000000000ULL,0x400e000000000000ULL},
    {"ADD.SIGNED",    OP_ADD,  0xc02b000000000000ULL,0x4011000000000000ULL,0xc022800000000000ULL},
    {"SUB.SIGNED",    OP_SUB,  0x405ef00000000000ULL,0xc021000000000000ULL,0x4060880000000000ULL},
    {"SUB.NEGATIVE",  OP_SUB,  0xbfe0000000000000ULL,0x3fe8000000000000ULL,0xbff4000000000000ULL},
    {"SUB.CLOSE",     OP_SUB,  0x408f400000000000ULL,0x408f3c0000000000ULL,0x3fe0000000000000ULL},
    {"MUL.SIGNED",    OP_MUL,  0xc031800000000000ULL,0x4022800000000000ULL,0xc0643c0000000000ULL},
    {"MUL.FRACTION",  OP_MUL,  0x3fc0000000000000ULL,0x3fb0000000000000ULL,0x3f80000000000000ULL},
    {"DIV.INEXACT",   OP_DIV,  0x408edc0000000000ULL,0x403f400000000000ULL,0x403f99999999999aULL},
    {"DIV.EXACT",     OP_DIV,  0x401c000000000000ULL,0x4030000000000000ULL,0x3fdc000000000000ULL},
    {"SQRT.TWO",      OP_SQRT, 0x4000000000000000ULL,0,0x3ff6a09e667f3bccULL},
    {"SQRT.INEXACT",  OP_SQRT, 0x405ed00000000000ULL,0,0x4026341f58bada14ULL},
    {"SQRT.FRACTION", OP_SQRT, 0x3fb0000000000000ULL,0,0x3fd0000000000000ULL},
    {"MUL.OVERFLOW",  OP_MUL,  0x7fefffffffffffffULL,0x4000000000000000ULL,0x7ff0000000000000ULL},
    {"DIV.SUBNORMAL", OP_DIV,  0x0010000000000000ULL,0x4000000000000000ULL,0x0008000000000000ULL}
};

int main(void) {
    volatile uint32_t passed = 0u;
    volatile uint32_t total = 0u;

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n\n");
    ee_printf("RV32I software floating-point arithmetic\n\n");

    print_all_inputs();
    run_all_tests(&passed, &total);

    ee_printf("\nSelf-check: %u/%u  %s\n\n", passed, total, passed == total ? "PASS" : "FAIL");

    app_finish();
}



static void print_all_inputs(void) {
    ee_printf("=== Input ===\n");
    ee_printf("Each operand is shown as a decimal value followed by its encoded bits.\n");
    ee_printf("Square root uses only operand A.\n\n");

    print_binary16_inputs();
    ee_printf("\n");
    print_bfloat16_inputs();
    ee_printf("\n");
    print_binary32_inputs();
    ee_printf("\n");
    print_binary64_inputs();
}



static void print_binary16_inputs(void) {
    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        float left = binary16_to_float(binary16_cases[index].left);
        float right = binary16_to_float(binary16_cases[index].right);

        print_small_input("binary16", &binary16_cases[index], left, right, index >= 12u);
    }
}

static void print_bfloat16_inputs(void) {
    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        float left = bfloat16_to_float(bfloat16_cases[index].left);
        float right = bfloat16_to_float(bfloat16_cases[index].right);

        print_small_input("bfloat16", &bfloat16_cases[index], left, right, index >= 12u);
    }
}

static void print_binary32_inputs(void) {
    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        float left = float_from_bits(binary32_cases[index].left);
        float right = float_from_bits(binary32_cases[index].right);

        print_f32_input(&binary32_cases[index], left, right, index >= 12u);
    }
}

static void print_binary64_inputs(void) {
    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        double left = double_from_bits(binary64_cases[index].left);
        double right = double_from_bits(binary64_cases[index].right);

        print_f64_input(&binary64_cases[index], left, right, index >= 12u);
    }
}

static void run_all_tests(volatile uint32_t *passed, volatile uint32_t *total) {
    ee_printf("\n=== Result ===\n");

    run_binary16_tests(passed, total);
    run_bfloat16_tests(passed, total);
    run_binary32_tests(passed, total);
    run_binary64_tests(passed, total);
}

static void run_binary16_tests(volatile uint32_t *passed, volatile uint32_t *total) {
    ee_printf("\n-- IEEE binary16 arithmetic --\n");

    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        float left = binary16_to_float(binary16_cases[index].left);
        float right = binary16_to_float(binary16_cases[index].right);
        uint16_t actual = float_to_binary16(calculate32(binary16_cases[index].operation, left, right));

        check16("binary16", binary16_cases[index].name, actual, binary16_cases[index].expected, passed, total);
    }
}

static void run_bfloat16_tests(volatile uint32_t *passed, volatile uint32_t *total) {
    ee_printf("\n-- bfloat16 arithmetic --\n");

    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        float left = bfloat16_to_float(bfloat16_cases[index].left);
        float right = bfloat16_to_float(bfloat16_cases[index].right);
        uint16_t actual = float_to_bfloat16(calculate32(bfloat16_cases[index].operation, left, right));

        check16("bfloat16", bfloat16_cases[index].name, actual, bfloat16_cases[index].expected, passed, total);
    }
}

static void run_binary32_tests(volatile uint32_t *passed, volatile uint32_t *total) {
    ee_printf("\n-- IEEE binary32 arithmetic --\n");

    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        volatile float left = float_from_bits(binary32_cases[index].left);
        volatile float right = float_from_bits(binary32_cases[index].right);
        uint32_t actual = float_bits(calculate32(binary32_cases[index].operation, left, right));

        check32(binary32_cases[index].name, actual, binary32_cases[index].expected, passed, total);
    }
}

static void run_binary64_tests(volatile uint32_t *passed, volatile uint32_t *total) {
    ee_printf("\n-- IEEE binary64 arithmetic --\n");

    for (uint32_t index = 0u; index < CASE_COUNT; ++index) {
        volatile double left = double_from_bits(binary64_cases[index].left);
        volatile double right = double_from_bits(binary64_cases[index].right);
        uint64_t actual = double_bits(calculate64(binary64_cases[index].operation, left, right));

        check64(binary64_cases[index].name, actual, binary64_cases[index].expected, passed, total);
    }
}

static float float_from_bits(uint32_t bits) {
    union {
        float value;
        uint32_t bits;
    } conversion;

    conversion.bits = bits;
    return conversion.value;
}

static uint32_t float_bits(float value) {
    union {
        float value;
        uint32_t bits;
    } conversion;

    conversion.value = value;
    return conversion.bits;
}

static double double_from_bits(uint64_t bits) {
    union {
        double value;
        uint64_t bits;
    } conversion;

    conversion.bits = bits;
    return conversion.value;
}

static uint64_t double_bits(double value) {
    union {
        double value;
        uint64_t bits;
    } conversion;

    conversion.value = value;
    return conversion.bits;
}


static uint16_t float_to_binary16(float value) {
    uint32_t bits     = float_bits(value);
    uint32_t sign     = (bits >> 16) & 0x8000u;
    uint32_t exponent = (bits >> 23) & 0xffu;
    uint32_t fraction = bits & 0x007fffffu;

    if (exponent == 0xffu) {
        uint32_t infinity_or_nan = fraction == 0u ? 0x7c00u : 0x7e00u;

        return (uint16_t)(sign | infinity_or_nan);
    }

    int32_t unbiased = (int32_t)exponent - 127;

    if (unbiased > 15)      return (uint16_t)(sign | 0x7c00u);
    if (unbiased < -24)     return (uint16_t)sign;
    if (unbiased >= -14) {
        uint32_t half_exponent = (uint32_t)(unbiased + 15) << 10;
        uint32_t rounded = fraction >> 13;
        uint32_t remainder = fraction & 0x1fffu;

        if (remainder > 0x1000u || (remainder == 0x1000u && (rounded & 1u) != 0u)) {
            ++rounded;
        }

        if (rounded == 0x400u) {
            rounded = 0u;
            half_exponent += 0x400u;
        }

        return (uint16_t)(sign | half_exponent | rounded);
    }

    uint32_t significand = 0x00800000u | fraction;
    uint32_t shift       = (uint32_t)(-unbiased - 1);
    uint32_t rounded     = significand >> shift;
    uint32_t remainder   = significand & ((1u << shift) - 1u);
    uint32_t halfway     = 1u << (shift - 1u);

    if (remainder > halfway || (remainder == halfway && (rounded & 1u) != 0u)) {
        ++rounded;
    }

    return (uint16_t)(sign | rounded);
}

static float binary16_to_float(uint16_t value) {
    uint32_t sign     = ((uint32_t)value & 0x8000u) << 16;
    uint32_t exponent = ((uint32_t)value >> 10) & 0x1fu;
    uint32_t fraction = (uint32_t)value & 0x03ffu;
    uint32_t bits;

    if (exponent == 0u) {
        if (fraction == 0u) {
            bits = sign;
        }
        else {
            int32_t adjusted = -14;

            while ((fraction & 0x0400u) == 0u) {
                fraction <<= 1;
                --adjusted;
            }

            bits = sign | ((uint32_t)(adjusted + 127) << 23) | ((fraction & 0x03ffu) << 13);
        }
    }
    else if (exponent == 0x1fu) {
        bits = sign | 0x7f800000u | (fraction << 13);
    }
    else {
        bits = sign | ((exponent - 15u + 127u) << 23) | (fraction << 13);
    }

    return float_from_bits(bits);
}



static uint16_t float_to_bfloat16(float value) {
    uint32_t bits = float_bits(value);

    if ((bits & 0x7fffffffu) > 0x7f800000u) {
        bits |= 0x00400000u;
    }

    bits += 0x00007fffu + ((bits >> 16) & 1u);
    return (uint16_t)(bits >> 16);
}



static float bfloat16_to_float(uint16_t value) {
    return float_from_bits((uint32_t)value << 16);
}



static float square_root32(float value) {
    float estimate = value * 0.5f + 1.0f;

    for (uint32_t index = 0u; index < 8u; ++index) {
        estimate = 0.5f * (estimate + value / estimate);
    }

    return estimate;
}

static double square_root64(double value) {
    double estimate = value * 0.5 + 1.0;

    for (uint32_t index = 0u; index < 12u; ++index) {
        estimate = 0.5 * (estimate + value / estimate);
    }

    return estimate;
}

static const char *operation_name(uint32_t operation) {
    if (operation == OP_ADD)        return "ADD";
    if (operation == OP_SUB)        return "SUB";
    if (operation == OP_MUL)        return "MUL";
    if (operation == OP_DIV)        return "DIV";
    return "SQRT";
}

static float calculate32(uint32_t operation, float left, float right) {
    volatile float first  = left;
    volatile float second = right;

    if (operation == OP_ADD)    return first + second;
    if (operation == OP_SUB)    return first - second;
    if (operation == OP_MUL)    return first * second;
    if (operation == OP_DIV)    return first / second;

    return square_root32(first);
}

static double calculate64(uint32_t operation, double left, double right) {
    volatile double first = left;
    volatile double second = right;

    if (operation == OP_ADD)    return first + second;
    if (operation == OP_SUB)    return first - second;
    if (operation == OP_MUL)    return first * second;
    if (operation == OP_DIV)    return first / second;

    return square_root64(first);
}

static void print_small_input(const char *format, const small_case_t *test,
                              float left, float right, uint32_t edge_case) {
    ee_printf("%-8s %-13s %-4s ", format, test->name, operation_name(test->operation));

    if (edge_case != 0u) {
        ee_printf("A = %13s [%04x] B = %13.8f [%04x]\n", test->operation == OP_MUL ? "MAX_FINITE" : "MIN_NORMAL", test->left, right, test->right);
    }
    else if (test->operation == OP_SQRT) {
        ee_printf("A = %13.8f [%04x] B = %13s [----]\n", left, test->left, "-");
    }
    else {
        ee_printf("A = %13.8f [%04x] B = %13.8f [%04x]\n", left, test->left, right, test->right);
    }
}

static void print_f32_input(const f32_case_t *test, float left, float right,
                            uint32_t edge_case) {
    ee_printf("binary32 %-13s %-4s ", test->name, operation_name(test->operation));

    if (edge_case != 0u) {
        ee_printf("A = %13s [%08x] B = %13.8f [%08x]\n", test->operation == OP_MUL ? "MAX_FINITE" : "MIN_NORMAL", test->left, right, test->right);
    }
    else if (test->operation == OP_SQRT) {
        ee_printf("A = %13.8f [%08x] B = %13s [--------]\n", left, test->left, "-");
    }
    else {
        ee_printf("A = %13.8f [%08x] B = %13.8f [%08x]\n", left, test->left, right, test->right);
    }
}

static void print_f64_input(const f64_case_t *test, double left, double right,
                            uint32_t edge_case) {
    uint32_t left_high = (uint32_t)(test->left >> 32);
    uint32_t left_low = (uint32_t)test->left;
    uint32_t right_high = (uint32_t)(test->right >> 32);
    uint32_t right_low = (uint32_t)test->right;

    ee_printf("binary64 %-13s %-4s ", test->name, operation_name(test->operation));

    if (edge_case != 0u) {
        ee_printf("A = %15s [%08x%08x] B = %15.10f [%08x%08x]\n", test->operation == OP_MUL ? "MAX_FINITE" : "MIN_NORMAL", left_high, left_low, right, right_high, right_low);
    }
    else if (test->operation == OP_SQRT) {
        ee_printf("A = %15.10f [%08x%08x] B = %15s [----------------]\n", left, left_high, left_low, "-");
    }
    else {
        ee_printf("A = %15.10f [%08x%08x] B = %15.10f [%08x%08x]\n", left, left_high, left_low, right, right_high, right_low);
    }
}

static void check16(const char *format, const char *name, uint16_t actual,
                    uint16_t expected, volatile uint32_t *passed,
                    volatile uint32_t *total) {
    uint32_t match = actual == expected;

    ++*total;

    if (match != 0u) {
        ++*passed;
    }

    ee_printf("%-8s %-13s expected = %04x  actual = %04x  %s\n", format, name, expected, actual, match != 0u ? "PASS" : "FAIL");
}



static void check32(const char *name, uint32_t actual, uint32_t expected,
                    volatile uint32_t *passed, volatile uint32_t *total) {
    uint32_t match = actual == expected;

    ++*total;

    if (match != 0u) {
        ++*passed;
    }

    ee_printf("binary32 %-13s expected = %08x  actual = %08x  %s\n", name, expected, actual, match != 0u ? "PASS" : "FAIL");
}



static void check64(const char *name, uint64_t actual, uint64_t expected,
                    volatile uint32_t *passed, volatile uint32_t *total) {
    uint32_t actual_high = (uint32_t)(actual >> 32);
    uint32_t actual_low = (uint32_t)actual;
    uint32_t expected_high = (uint32_t)(expected >> 32);
    uint32_t expected_low = (uint32_t)expected;
    uint32_t match = actual == expected;

    ++*total;

    if (match != 0u) {
        ++*passed;
    }

    ee_printf("binary64 %-13s expected = %08x%08x  actual = %08x%08x  %s\n", name, expected_high, expected_low, actual_high, actual_low, match != 0u ? "PASS" : "FAIL");
}


