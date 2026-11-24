#include "io_test.h"


static uint32_t errors;

static void set_and_check(uint32_t value) {
    set_switches(value);
    if (wait_for(SW_BASE, value, 16u)) return;
    if (errors == 0u) ee_printf("        switches = 0x%03x, SW = 0x%03x\n", value, SW & 0x3ffu);
    ++errors;
}




static void finish_row(const char *name) {
    expect(name, errors, 0u);
    errors = 0u;
}


int main(void) {
    uint32_t bit, value, pattern = 1u;

    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n");
    ee_printf("Milestone 3 switch and LED test\n");
    section(1u, "Switches");
    for (bit = 0u; bit < 10u; ++bit) set_and_check(1u << bit);
    finish_row("Each SW bit follows its own switch");
    for (bit = 0u; bit < 10u; ++bit) set_and_check(0x3ffu ^ (1u << bit));
    finish_row("Switches do not affect each other");
    set_and_check(0x3ffu);
    set_and_check(0u);
    finish_row("SW is 0x3FF when all are on, 0 when all are off");
    for (uint32_t index = 0u; index < 1024u; ++index) {
        set_and_check(pattern);
        pattern = ((pattern << 1) | (((pattern >> 9) ^ (pattern >> 6)) & 1u)) & 0x3ffu;
    }
    set_and_check(0u);
    finish_row("SW is correct for every on/off pattern of the 10 switches");

    section(2u, "LEDs");
    for (bit = 0u; bit < 18u; ++bit) {
        LEDR = 1u << bit;
        check_led_pins(0u, 1u << bit);
        if ((LEDR & 0x3ffffu) != (1u << bit)) ++errors;
    }
    finish_row("LEDR[17:0] drives the red LED pins");
    for (bit = 0u; bit < 8u; ++bit) {
        LEDG = 1u << bit;
        check_led_pins(1u, 1u << bit);
        if ((LEDG & 0xffu) != (1u << bit)) ++errors;
    }
    finish_row("LEDG[7:0] drives the green LED pins");
    LEDB = 1u;
    check_led_pins(2u, 1u);
    if ((LEDB & 1u) != 1u) ++errors;
    LEDB = 0u;
    check_led_pins(2u, 0u);
    if ((LEDB & 1u) != 0u) ++errors;
    finish_row("LEDB[0] drives the blue LED pin");
    pattern = 1u;
    for (uint32_t index = 0u; index < 256u; ++index) {
        set_and_check(pattern);
        value = SW & 0x3ffu;
        LEDR  = value;
        LEDG  = ~value;
        LEDB  = value;
        check_led_pins(0u, value);
        check_led_pins(1u, ~value);
        check_led_pins(2u, value);
        if ((LEDR & 0x3ffffu) != value || (LEDG & 0xffu) != (~value & 0xffu) || (LEDB & 1u) != (value & 1u)) ++errors;
        pattern = ((pattern << 1) | (((pattern >> 9) ^ (pattern >> 6)) & 1u)) & 0x3ffu;
    }
    finish_row("LEDs show the switch value and keep the value written");
    LEDR = 0u;
    LEDG = 0u;
    LEDB = 0u;
    io_finish("Switches and LEDs");
}
