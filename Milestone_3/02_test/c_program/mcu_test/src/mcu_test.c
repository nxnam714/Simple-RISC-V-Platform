#include "io_test.h"
#include "mcu_expected.h"


#define PART_TOTAL 8u
#define SIEVE_N    50000u
#define BIG_GROUPS 12u

static uint32_t case_count;
static uint32_t group_failed;
static uint32_t groups_failed;
static uint32_t cycles;
static uint32_t mark;
static uint32_t part_cycles[8];
static uint8_t sieve_bits[(SIEVE_N + 7u) / 8u];

static void lap_start(void) { mark = TIMER_COUNT_LO; }
static void lap_stop(void) { cycles += TIMER_COUNT_LO - mark; }

static void check(uint32_t passed) {
    SIM_EVENT = (passed ? 0u : EVENT_FAIL) | EVENT_WORD(EVENT_CASE, case_count);
    ++case_count;
    if (!passed) {
        group_failed = 1u;
        row_failed = 1u;
    }
}





static void part_begin(uint32_t id, const char *name) {
    section(id, name);
    SIM_EVENT = EVENT_WORD(EVENT_GROUP_BEGIN, id);
    group_failed = 0u;
    cycles = 0u;
    TIMER_CTRL = TIMER_EN | TIMER_CLR;
}





static void part_end(uint32_t id) {
    part_cycles[id] = cycles;
    SIM_EVENT = (group_failed ? EVENT_FAIL : 0u) | EVENT_WORD(EVENT_GROUP, id);
    if (group_failed) ++groups_failed;
}





static uint32_t read_switches(uint32_t value) {
    set_switches(value);
    check(wait_for(SW_BASE, value & 0x3ffu, 16u));
    return SW & 0x3ffu;
}




static int32_t read_switches_signed(int32_t value) { return ((int32_t)(read_switches((uint32_t)value) << 22)) >> 22; }

static uint32_t read_pins(uint32_t value) {
    set_gpio(value);
    check(wait_for(GPIO_BASE + 0x8u, value, 16u));
    return GPIO_IN;
}




static uint32_t read_keys(uint32_t value) {
    uint32_t result = 0u;
    for (uint32_t shift = 8u; shift > 0u; shift -= 4u) {
        uint32_t nibble = (value >> (shift - 4u)) & 0xfu;
        SIM_EVENT = EVENT_WORD(EVENT_SET_KEY, nibble);
        check(wait_for(KEY_BASE, nibble, 16u));
        result = (result << 4) | (KEY & 0xfu);
    }
    SIM_EVENT = EVENT_WORD(EVENT_SET_KEY, 0u);
    check(wait_for(KEY_BASE, 0u, 16u));
    return result;
}




static void show_on_led(uint32_t value) {
    LEDR = value;
    check_led_pins(0u, value);
}



static void show_on_hex(uint32_t value) {
    for (uint32_t digit = 0u; digit < 8u; ++digit) {
        uint32_t pattern = (value >> (digit + digit + digit)) & 0x7fu;
        HEX(digit) = pattern;
        check_hex_pins(digit, pattern);
    }
}


static void show_on_pins(uint32_t value) {
    GPIO_OUT = value;
    GPIO_DIR = 0xffffffffu;
    check_gpio_pins(0u, value);
    check_gpio_pins(1u, 0xffffffffu);
}




static uint32_t divide_u32(uint32_t top, uint32_t bottom, uint32_t *remainder) {
    uint32_t quotient = 0u;
    uint32_t rest = 0u;
    for (uint32_t bit = 32u; bit > 0u; --bit) {
        rest = (rest << 1) | (top >> 31);
        top <<= 1;
        quotient <<= 1;
        if (rest >= bottom) {
            rest -= bottom;
            quotient |= 1u;
        }
    }
    if (remainder) *remainder = rest;
    return quotient;
}





static uint32_t magnitude(int32_t value) { return value < 0 ? 0u - (uint32_t)value : (uint32_t)value; }

static int32_t divide_i32(int32_t top, int32_t bottom) {
    uint32_t quotient = divide_u32(magnitude(top), magnitude(bottom), 0);
    return ((top < 0) != (bottom < 0)) ? -(int32_t)quotient : (int32_t)quotient;
}





static int32_t divide_round(int32_t top, int32_t bottom) {
    uint32_t quotient = divide_u32(magnitude(top) + (magnitude(bottom) >> 1), magnitude(bottom), 0);
    return ((top < 0) != (bottom < 0)) ? -(int32_t)quotient : (int32_t)quotient;
}





static void print_list(const int32_t *values, uint32_t count) {
    ee_printf("[");

    for (uint32_t i = 0u; i < count; ++i) {
        ee_printf(i ? ", %d" : "%d", values[i]);
    }
    ee_printf("]");
}



static void print_fixed(int32_t value, uint32_t digits) {
    uint32_t rest;
    uint32_t whole = divide_u32(magnitude(value), digits == 1u ? 10u : 10000u, &rest);
    ee_printf(digits == 1u ? "%s%u.%u" : "%s%u.%04u", value < 0 ? "-" : "", whole, rest);
}


static void print_fixed_list(const int32_t *values, uint32_t count, uint32_t digits) {
    ee_printf("[");

    for (uint32_t i = 0u; i < count; ++i) {
        if (i) ee_printf(", ");

        print_fixed(values[i], digits);
    }
    ee_printf("]");
}




static void print_matrix(const int32_t *values, uint32_t size) {
    for (uint32_t row = 0u; row < size; ++row) {
        ee_printf("[");

        for (uint32_t col = 0u; col < size; ++col) {
            ee_printf("%5d", values[row * size + col]);
        }
        ee_printf(" ]\n");
    }
}




static uint32_t is_marked(uint32_t n) { return (sieve_bits[n >> 3] >> (n & 7u)) & 1u; }

static void part_primes(void) {
    uint32_t n;
    uint32_t count = 0u;
    uint32_t passed;
    int32_t last[5] = {0, 0, 0, 0, 0};

    part_begin(1u, "Prime numbers");
    GPIO_DIR = 0u;
    n = read_pins(SIEVE_N);
    if (n > SIEVE_N) n = SIEVE_N;
    lap_start();
    for (uint32_t i = 0u; i < sizeof(sieve_bits); ++i) {
        sieve_bits[i] = 0u;
    }
    uint32_t square = 4u;
    for (uint32_t i = 2u; square < n; ++i) {
        if (!is_marked(i)) {
            for (uint32_t m = square; m < n; m += i) {
                sieve_bits[m >> 3] |= (uint8_t)(1u << (m & 7u));
            }
        }
        square += i + i + 1u;
    }
    for (uint32_t i = 2u; i < n; ++i) {
        if (is_marked(i)) continue;
        for (uint32_t k = 0u; k < 4u; ++k) {
            last[k] = last[k + 1u];
        }
        last[4] = (int32_t)i;
        ++count;
    }
    lap_stop();
    passed = count == EXPECT_PRIME_COUNT;
    for (uint32_t k = 0u; k < 5u; ++k) {
        passed &= ((uint32_t)last[k] == expect_primes_last[k]);
    }
    ee_printf("Input    : N = %u\n", n);

    ee_printf("Result   : %u primes below N, last 5 = ", count);

    print_list(last, 5u);
    ee_printf("\nExpected : " TEXT_PRIMES "\n");

    ee_printf("Read     : GPIO = %u\n", n);

    ee_printf("Write    : LEDR = %u\n", count);

    show_on_led(count);
    check(passed);
    set_gpio(0u);
    row("Prime count matches");
    part_end(1u);
}



static void print_line(int32_t slope, int32_t intercept) {
    ee_printf("y = ");

    print_fixed(slope, 4u);
    ee_printf(intercept < 0 ? "x - " : "x + ");

    print_fixed(intercept < 0 ? -intercept : intercept, 4u);
    ee_printf("\n");
}




static void part_regression(void) {
    static const int32_t x_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    static const int32_t y_in[10] = {23, 41, 78, 86, 129, 134, 177, 182, 226, 239};
    int32_t x[10];
    int32_t y[10];
    int32_t sx = 0;
    int32_t sy = 0;
    int32_t sxy = 0;
    int32_t sxx = 0;
    int32_t den;
    int32_t slope;
    int32_t intercept;

    part_begin(2u, "Linear regression");
    for (uint32_t i = 0u; i < 10u; ++i) {
        x[i] = (int32_t)read_switches((uint32_t)x_in[i]);
        y[i] = (int32_t)read_switches((uint32_t)y_in[i]);
    }
    lap_start();
    for (uint32_t i = 0u; i < 10u; ++i) {
        sx += x[i];
        sy += y[i];
        sxy += multiply_i32(x[i], y[i]);
        sxx += multiply_i32(x[i], x[i]);
    }
    den = multiply_i32(10, sxx) - multiply_i32(sx, sx);
    slope = divide_round(multiply_i32(multiply_i32(10, sxy) - multiply_i32(sx, sy), 1000), den);
    intercept = divide_round(multiply_i32(multiply_i32(sy, sxx) - multiply_i32(sx, sxy), 1000), den);
    lap_stop();
    ee_printf("Input    : x = ");

    print_list(x, 10u);
    ee_printf("\n           y = ");

    print_fixed_list(y, 10u, 1u);
    ee_printf("\nResult   : ");

    print_line(slope, intercept);
    ee_printf("Expected : " TEXT_LINE "\n");

    ee_printf("Read     : SW = x, y (one entry per read)\n");

    ee_printf("Write    : LEDR = %d, %d (one entry per write)\n", slope, intercept);

    show_on_led((uint32_t)slope);
    show_on_led((uint32_t)intercept);
    check((slope == EXPECT_SLOPE) && (intercept == EXPECT_INTERCEPT));
    row("Fitted line matches");
    part_end(2u);
}





static void big_set(uint32_t *number, uint32_t value) {
    for (uint32_t i = 0u; i < BIG_GROUPS; ++i) {
        number[i] = 0u;
    }
    number[0] = value;
}


static void big_multiply(uint32_t *number, uint32_t factor) {
    uint32_t carry = 0u;
    uint32_t rest;
    for (uint32_t i = 0u; i < BIG_GROUPS; ++i) {
        carry = divide_u32(multiply_u32(number[i], factor) + carry, 10000u, &rest);
        number[i] = rest;
    }
}


static void big_add(uint32_t *sum, const uint32_t *a, const uint32_t *b) {
    uint32_t carry = 0u;
    for (uint32_t i = 0u; i < BIG_GROUPS; ++i) {
        uint32_t total = a[i] + b[i] + carry;
        carry = total >= 10000u;
        sum[i] = carry ? total - 10000u : total;
    }
}





static void big_print(const uint32_t *number) {
    uint32_t top = BIG_GROUPS - 1u;
    while ((top > 0u) && (number[top] == 0u)) {
        --top;
    }
    ee_printf("%u", number[top]);

    while (top > 0u) {
        ee_printf("%04u", number[--top]);
    }
    ee_printf("\n");
}


static void big_print_all(uint32_t numbers[3][BIG_GROUPS], uint32_t n1, uint32_t n2, uint32_t n3) {
    ee_printf("Result   : %u!    = ", n1);

    big_print(numbers[0]);
    ee_printf("           2^%u  = ", n2);

    big_print(numbers[1]);
    ee_printf("           F(%u) = ", n3);

    big_print(numbers[2]);
}





static void part_big(void) {
    uint32_t result[3][BIG_GROUPS];
    uint32_t a[BIG_GROUPS];
    uint32_t b[BIG_GROUPS];
    uint32_t n1;
    uint32_t n2;
    uint32_t n3;
    uint32_t passed = 1u;

    part_begin(3u, "Big numbers");
    n1 = read_switches(30u);
    n2 = read_switches(100u);
    n3 = read_switches(100u);
    lap_start();
    big_set(result[0], 1u);
    for (uint32_t k = 2u; k <= n1; ++k) {
        big_multiply(result[0], k);
    }
    big_set(result[1], 1u);
    for (uint32_t k = 0u; k < n2; ++k) {
        big_multiply(result[1], 2u);
    }
    big_set(a, 0u);
    big_set(b, 1u);
    for (uint32_t k = 0u; k < n3; ++k) {
        big_add(result[2], a, b);
        for (uint32_t i = 0u; i < BIG_GROUPS; ++i) {
            a[i] = b[i];
            b[i] = result[2][i];
        }
    }
    for (uint32_t i = 0u; i < BIG_GROUPS; ++i) {
        result[2][i] = a[i];
    }
    lap_stop();
    for (uint32_t k = 0u; k < 3u; ++k) {
        for (uint32_t i = 0u; i < BIG_GROUPS; ++i) {
            passed &= (result[k][i] == expect_big[k][i]);
        }
    }
    ee_printf("Input    : [%u!, 2^%u, F(%u)]\n", n1, n2, n3);

    big_print_all(result, n1, n2, n3);
    ee_printf("Expected : " TEXT_BIG "\n");

    ee_printf("Read     : SW = %u, %u, %u (one entry per read)\n", n1, n2, n3);

    ee_printf("Write    : HEX7-HEX0 = %u, %u, %u (one entry per write)\n", multiply_u32(result[0][1], 10000u) + result[0][0], multiply_u32(result[1][1], 10000u) + result[1][0], multiply_u32(result[2][1], 10000u) + result[2][0]);

    for (uint32_t k = 0u; k < 3u; ++k) {
        show_on_hex(multiply_u32(result[k][1], 10000u) + result[k][0]);
    }
    check(passed);
    row("Big numbers match");
    part_end(3u);
}




static void part_pascal(void) {
    uint32_t row_values[13];
    uint32_t n;
    uint32_t sum = 0u;
    uint32_t passed = 1u;

    part_begin(4u, "Pascal's triangle");
    n = read_switches(12u);
    if (n > 12u) n = 12u;
    ee_printf("Input    : rows 0 to %u\nResult   :\n", n);

    for (uint32_t r = 0u; r <= n; ++r) {
        lap_start();
        row_values[r] = 1u;
        for (uint32_t k = r - 1u; (k > 0u) && (k < r); --k) {
            row_values[k] += row_values[k - 1u];
        }
        lap_stop();
        for (uint32_t s = n - r; s > 0u; --s) {
            ee_printf("  ");
        }
        for (uint32_t k = 0u; k <= r; ++k) {
            uint32_t v = row_values[k];
            uint32_t width = v >= 100u ? 3u : v >= 10u ? 2u : 1u;
            uint32_t left = (4u - width) >> 1;
            ee_printf("%s%u", left == 1u ? " " : "", v);

            if (k == r) continue;
            for (uint32_t s = 4u - width - left; s > 0u; --s) {
                ee_printf(" ");
            }
        }
        ee_printf("\n");
    }
    for (uint32_t k = 0u; k <= n; ++k) {
        sum += row_values[k];
        passed &= (row_values[k] == expect_pascal[k]);
        show_on_led(row_values[k]);
    }
    ee_printf("Result   : row %u sum = %u\n", n, sum);

    ee_printf("Expected : row 12 sum = " TEXT_PASCAL_SUM "\n");

    ee_printf("Read     : SW = %u\n", n);

    ee_printf("Write    : LEDR = row %u (one entry per write)\n", n);

    check(passed && (n == 12u));
    row("Binomial coefficients match");
    part_end(4u);
}





static int32_t determinant(int32_t *m, uint32_t size) {
    int32_t sign = 1;
    int32_t previous = 1;
    for (uint32_t k = 0u; k + 1u < size; ++k) {
        if (m[k * size + k] == 0) {
            uint32_t swap = k + 1u;
            while ((swap < size) && (m[swap * size + k] == 0)) {
                ++swap;
            }
            if (swap == size) return 0;
            for (uint32_t j = 0u; j < size; ++j) {
                int32_t t = m[k * size + j];
                m[k * size + j] = m[swap * size + j];
                m[swap * size + j] = t;
            }
            sign = -sign;
        }
        for (uint32_t i = k + 1u; i < size; ++i) {
            for (uint32_t j = k + 1u; j < size; ++j) {
                m[i * size + j] = divide_i32(multiply_i32(m[i * size + j], m[k * size + k]) - multiply_i32(m[i * size + k], m[k * size + j]), previous);
            }
        }
        previous = m[k * size + k];
    }
    return sign < 0 ? -m[size * size - 1u] : m[size * size - 1u];
}




static void part_matrix(void) {
    static const int32_t b[64] = {-6, 3, -1, -5, 4, 0, -4, 5, -1, -5, 4, 0, -4, 5, 1, -3, 4, 0, -4, 5, 1, -3, 6, 2, -4, 5, 1, -3, 6, 2, -2, -6, 1, -3, 6, 2, -2, -6, 3, -1, 6, 2, -2, -6, 3, -1, -5, 4, -2, -6, 3, -1, -5, 4, 0, -4, 3, -1, -5, 4, 0, -4, 5, 1};
    static const int32_t d_in[16] = {2, -3, 1, 5, 4, 0, -2, 3, -1, 6, 3, -4, 7, 2, -5, 1};
    int32_t a[64];
    int32_t c[64];
    int32_t d[16];
    int32_t work[16];
    int32_t det;
    uint32_t checksum = 0u;

    part_begin(5u, "Matrix multiply and determinant");
    for (uint32_t i = 0u; i < 8u; ++i) {
        for (uint32_t j = 0u; j < 8u; ++j) {
            uint32_t r;
            divide_u32(multiply_u32(i, 7u) + multiply_u32(j, 3u), 11u, &r);
            a[i * 8u + j] = read_switches_signed((int32_t)r - 5);
        }
    }
    for (uint32_t i = 0u; i < 16u; ++i) {
        d[i] = read_switches_signed(d_in[i]);
    }
    lap_start();
    for (uint32_t i = 0u; i < 8u; ++i) {
        for (uint32_t j = 0u; j < 8u; ++j) {
            int32_t sum = 0;
            for (uint32_t k = 0u; k < 8u; ++k) {
                sum += multiply_i32(a[i * 8u + k], b[k * 8u + j]);
            }
            c[i * 8u + j] = sum;
            checksum = multiply_u32(checksum, 31u) + (uint32_t)sum;
        }
    }
    for (uint32_t i = 0u; i < 16u; ++i) {
        work[i] = d[i];
    }
    det = determinant(work, 4u);
    lap_stop();
    ee_printf("Input A  :\n");

    print_matrix(a, 8u);
    ee_printf("Input B  :\n");

    print_matrix(b, 8u);
    ee_printf("Result C = A x B :\n");

    print_matrix(c, 8u);
    ee_printf("Input D  :\n");

    print_matrix(d, 4u);
    ee_printf("Result   : checksum(C) = 0x%08x, det(D) = %d\n", checksum, det);

    ee_printf("Expected : checksum(C) = " TEXT_CHECKSUM ", det(D) = " TEXT_DET "\n");

    ee_printf("Read     : SW = A, D (one entry per read)\n");

    ee_printf("Write    : LEDR = 0x%08x\n           HEX7-HEX0 = %d\n", checksum, det);

    show_on_led(checksum);
    show_on_hex((uint32_t)det);
    check(checksum == EXPECT_CHECKSUM);
    check(det == EXPECT_DET);
    row("Matrix product and determinant match");
    part_end(5u);
}




static int32_t det3(int32_t m[3][4], uint32_t replace) {
    int32_t v[3][3];
    for (uint32_t i = 0u; i < 3u; ++i) {
        for (uint32_t j = 0u; j < 3u; ++j) {
            v[i][j] = m[i][j == replace ? 3u : j];
        }
    }
    return multiply_i32(v[0][0], multiply_i32(v[1][1], v[2][2]) - multiply_i32(v[1][2], v[2][1])) - multiply_i32(v[0][1], multiply_i32(v[1][0], v[2][2]) - multiply_i32(v[1][2], v[2][0])) + multiply_i32(v[0][2], multiply_i32(v[1][0], v[2][1]) - multiply_i32(v[1][1], v[2][0]));
}



static void print_term(int32_t value, char name, uint32_t first) {
    if (first) ee_printf(value == 1 ? "  " : value == -1 ? " -" : "%2d", value);

    else ee_printf(" %c %s", value < 0 ? '-' : '+', magnitude(value) == 1u ? " " : "");

    if (!first && (magnitude(value) != 1u)) ee_printf("%u", magnitude(value));

    ee_printf("%c", name);
}



static void part_linear(void) {
    static const int32_t in[3][4] = {{2, 1, -1, 8}, {-3, -1, 2, -11}, {-2, 1, 2, -3}};
    int32_t m[3][4];
    int32_t solution[3];
    int32_t d;

    part_begin(6u, "Linear system");
    GPIO_DIR = 0u;
    for (uint32_t i = 0u; i < 3u; ++i) {
        for (uint32_t j = 0u; j < 4u; ++j) {
            m[i][j] = (int32_t)read_pins((uint32_t)in[i][j]);
        }
    }
    lap_start();
    d = det3(m, 3u);
    for (uint32_t k = 0u; k < 3u; ++k) {
        solution[k] = d ? divide_i32(det3(m, k), d) : 0;
    }
    lap_stop();
    for (uint32_t i = 0u; i < 3u; ++i) {
        ee_printf(i ? "           " : "Input    : ");

        print_term(m[i][0], 'x', 1u);
        print_term(m[i][1], 'y', 0u);
        print_term(m[i][2], 'z', 0u);
        ee_printf(" = %3d\n", m[i][3]);
    }
    ee_printf("Result   : [x, y, z] = ");

    print_list(solution, 3u);
    ee_printf("\nExpected : " TEXT_SOLUTION "\n");

    ee_printf("Read     : GPIO = equations (one entry per read)\n");

    ee_printf("Write    : GPIO = %d, %d, %d (one entry per write)\n", solution[0], solution[1], solution[2]);

    for (uint32_t k = 0u; k < 3u; ++k) {
        show_on_pins((uint32_t)solution[k]);
        check(solution[k] == expect_solution[k]);
    }
    GPIO_DIR = 0u;
    GPIO_OUT = 0u;
    set_gpio(0u);
    row("Solution matches");
    part_end(6u);
}



static int32_t sine_x10000(uint32_t degrees) {
    int32_t x = (int32_t)multiply_u32(degrees, 9370165u);
    int32_t term = x;
    int32_t sum = x;
    int32_t x2 = (int32_t)(((int64_t)x * x) >> 29);
    for (int32_t k = 1; k <= 8; ++k) {
        term = -divide_i32((int32_t)(((int64_t)term * x2) >> 29), multiply_i32(k + k, k + k + 1));
        sum += term;
    }
    return (int32_t)((((int64_t)sum * 10000) + (1 << 28)) >> 29);
}


static void part_sine(void) {
    static const uint32_t angle_in[5] = {0u, 30u, 45u, 60u, 90u};
    int32_t angles[5];
    int32_t sines[5];

    part_begin(7u, "Sine");
    for (uint32_t i = 0u; i < 5u; ++i) {
        angles[i] = (int32_t)read_keys(angle_in[i]);
    }
    lap_start();
    for (uint32_t i = 0u; i < 5u; ++i) {
        sines[i] = sine_x10000((uint32_t)angles[i]);
    }
    lap_stop();
    ee_printf("Input    : angle = ");

    print_list(angles, 5u);
    ee_printf(" degrees\nResult   : sin = ");

    print_fixed_list(sines, 5u, 4u);
    ee_printf("\nExpected : " TEXT_SINE "\n");

    ee_printf("Read     : KEY = ");

    print_list(angles, 5u);
    ee_printf(" (one entry per read)\nWrite    : HEX7-HEX0 = ");

    print_list(sines, 5u);
    ee_printf(" (one entry per write)\n");

    for (uint32_t i = 0u; i < 5u; ++i) {
        show_on_hex((uint32_t)sines[i]);
        check(sines[i] == expect_sine[i]);
    }
    row("Sine values match");
    part_end(7u);
}



int main(void) {
    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n\n");

    ee_printf("Milestone 3 system test\n");

    part_primes();
    part_regression();
    part_big();
    part_pascal();
    part_matrix();
    part_linear();
    part_sine();
    SIM_EVENT = EVENT_WORD(EVENT_GROUP_BEGIN, 8u);
    group_failed = 0u;
    section(8u, "Timer");
    for (uint32_t id = 1u; id < 8u; ++id) {
        ee_printf("Part %u: %u cycles\n", id, part_cycles[id]);

        check((part_cycles[id] != 0u) && (part_cycles[id] < 50000000u));
    }
    row("Every part reads above 0 and below 50000000 cycles");
    SIM_EVENT = (group_failed ? EVENT_FAIL : 0u) | EVENT_WORD(EVENT_GROUP, 8u);
    if (group_failed) ++groups_failed;
    SIM_EVENT = EVENT_WORD(EVENT_SUMMARY, case_count);
    ee_printf("\nParts passed: %u/%u\n\n", PART_TOTAL - groups_failed, PART_TOTAL);

    app_finish();
}
