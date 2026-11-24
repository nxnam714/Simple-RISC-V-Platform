#include "io_test.h"


#define TICK_CYCLES 1000u
#define TICKS       16u
#define TICK_WAIT   2000u

static uint32_t next_rand(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}





static void clear_digits(void) { for (uint32_t digit = 0u; digit < 8u; ++digit) HEX(digit) = 0u; }

static void expect_digit(uint32_t digit, uint32_t pattern, uint32_t *errors) {
    check_hex_pins(digit, pattern);
    if ((HEX(digit) & 0x7fu) != pattern) ++*errors;
}



static void segment_row(uint32_t digit) {
    uint32_t errors = 0u;
    for (uint32_t bit = 0u; bit < 7u; ++bit) {
        HEX(digit) = 1u << bit;
        expect_digit(digit, 1u << bit, &errors);
    }
    HEX(digit) = 0u;
    expect_digit(digit, 0u, &errors);
    ++checks_total;
    if (errors != 0u) ++checks_failed;
    ee_printf(errors != 0u ? "@F " : "@P ");
    ee_printf("Each bit of HEX%u drives its own segment pin\n", (unsigned)digit);
}





int main(void) {
    uint32_t errors = 0u, seed = 0x4f6cdd1du, late = 0u;

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n");
    ee_printf("Milestone 3 seven-segment display test\n");

    section(1u, "Segments");
    for (uint32_t digit = 0u; digit < 8u; ++digit) segment_row(digit);

    section(2u, "Digits");
    for (uint32_t digit = 0u; digit < 8u; ++digit) {
        clear_digits();
        HEX(digit) = 0x55u;
        for (uint32_t other = 0u; other < 8u; ++other) expect_digit(other, other == digit ? 0x55u : 0u, &errors);
    }
    expect("Writing one digit does not change the other digits", errors, 0u);
    errors = 0u;
    for (uint32_t digit = 0u; digit < 8u; ++digit) HEX(digit) = hex_font[digit];
    for (uint32_t digit = 0u; digit < 8u; ++digit) expect_digit(digit, hex_font[digit], &errors);
    expect("Each digit keeps its own value", errors, 0u);
    errors = 0u;
    for (uint32_t digit = 0u; digit < 8u; ++digit) HEX(digit) = 0x7fu;
    for (uint32_t digit = 0u; digit < 8u; ++digit) expect_digit(digit, 0x7fu, &errors);
    expect("Segment pins are all low when digits hold 0x7F", errors, 0u);
    errors = 0u;
    clear_digits();
    for (uint32_t digit = 0u; digit < 8u; ++digit) expect_digit(digit, 0u, &errors);
    expect("Segment pins are all high when digits hold 0", errors, 0u);
    errors = 0u;

    section(3u, "Timer");
    tick_start(TICK_CYCLES);
    for (uint32_t tick = 0u; tick < TICKS; ++tick) {
        uint32_t attempt = 0u;
        while (!tick_elapsed(TICK_CYCLES) && attempt < TICK_WAIT) ++attempt;
        if (attempt == TICK_WAIT) ++late;
        for (uint32_t digit = 0u; digit < 8u; ++digit) {
            seed = next_rand(seed);
            HEX(digit) = seed & 0x7fu;
            expect_digit(digit, seed & 0x7fu, &errors);
        }
    }
    TIMER_CTRL = 0u;
    clear_digits();
    expect("Digits change at each timer interval", errors + late, 0u);
    io_finish("Seven-segment displays");
}
