#include "io_test.h"


#define RANDOM_PATTERNS 256u

static uint32_t errors;

static uint32_t next_rand(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}



static void expect_in(uint32_t expected) {
    if (wait_for(GPIO_BASE + 0x8u, expected, 16u)) return;
    if (errors == 0u) ee_printf("        expected GPIO_IN = 0x%08x, GPIO_IN = 0x%08x\n", expected, GPIO_IN);
    ++errors;
}


static void finish_row(const char *name) {
    expect(name, errors, 0u);
    errors = 0u;
}





static void set_pins(uint32_t direction, uint32_t out, uint32_t outside) {
    GPIO_DIR = direction;
    GPIO_OUT = out;
    set_gpio(outside);
}





static void expect_out(uint32_t value) {
    check_gpio_pins(0u, value);
    if (GPIO_OUT != value) ++errors;
}



int main(void) {
    uint32_t seed = 0x6b8b4567u, bit;

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n");
    ee_printf("Milestone 3 GPIO test\n");

    section(1u, "Input pins");
    set_pins(0u, 0u, 0u);
    for (bit = 0u; bit < 32u; ++bit) {
        set_gpio(1u << bit);
        expect_in(1u << bit);
    }
    finish_row("Each GPIO_IN bit follows its own input pin");
    for (bit = 0u; bit < 32u; ++bit) {
        set_gpio(~(1u << bit));
        expect_in(~(1u << bit));
    }
    finish_row("Input pins do not affect each other");
    for (uint32_t index = 0u; index < RANDOM_PATTERNS; ++index) {
        seed = next_rand(seed);
        set_gpio(seed);
        expect_in(seed);
    }
    finish_row("GPIO_IN is correct for 256 random input patterns");

    section(2u, "Output pins");
    set_pins(0xffffffffu, 0u, 0u);
    for (bit = 0u; bit < 32u; ++bit) {
        GPIO_OUT = 1u << bit;
        expect_out(1u << bit);
    }
    finish_row("Each GPIO_OUT bit drives its own output pin");
    for (bit = 0u; bit < 32u; ++bit) {
        GPIO_OUT = ~(1u << bit);
        expect_out(~(1u << bit));
    }
    finish_row("Output pins do not affect each other");
    for (uint32_t index = 0u; index < 32u; ++index) {
        seed     = next_rand(seed);
        GPIO_OUT = seed;
        expect_in(seed);
    }
    finish_row("GPIO_IN reads back the value on output pins");

    section(3u, "Pin direction");
    for (bit = 0u; bit < 32u; ++bit) {
        GPIO_DIR = 1u << bit;
        check_gpio_pins(1u, 1u << bit);
        if (GPIO_DIR != (1u << bit)) ++errors;
    }
    finish_row("Each GPIO_DIR bit sets the direction of its own pin");
    set_pins(0u, 0xffffffffu, 0u);
    expect_in(0u);
    set_gpio(0xa5a5a5a5u);
    expect_in(0xa5a5a5a5u);
    finish_row("Input pins ignore GPIO_OUT");
    set_pins(0xffffffffu, 0u, 0xffffffffu);
    expect_in(0u);
    GPIO_OUT = 0x5a5a5a5au;
    expect_in(0x5a5a5a5au);
    finish_row("Output pins ignore the value driven from outside");
    set_pins(0xaaaaaaaau, 0xffffffffu, 0u);
    expect_in(0xaaaaaaaau);
    set_pins(0xaaaaaaaau, 0u, 0xffffffffu);
    expect_in(0x55555555u);
    set_pins(0xffff0000u, 0x12340000u, 0x00005678u);
    expect_in(0x12345678u);
    finish_row("Input and output pins work at the same time");
    for (bit = 0u; bit < 32u; ++bit) {
        set_pins(1u << bit, 0u, 0xffffffffu);
        expect_in(~(1u << bit));
        set_pins(0u, 0u, 0xffffffffu);
        expect_in(0xffffffffu);
    }
    finish_row("Each pin changes between input and output");
    set_pins(0u, 0u, 0u);
    io_finish("GPIO");
}
