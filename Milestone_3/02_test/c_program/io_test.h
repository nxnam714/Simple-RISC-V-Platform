#ifndef M3_IO_TEST_H
#define M3_IO_TEST_H

#include "app_runtime.h"
#include "mcu.h"
#include "check_row.h"


static uint32_t checks_failed;
static uint32_t checks_total;

static void report(uint32_t passed, const char *name, uint32_t observed, uint32_t expected) {
    ++checks_total;
    if (!passed) { ++checks_failed; row_failed = 1u; }
    row(name);
    if (!passed) ee_printf("        observed = 0x%08x, expected = 0x%08x\n", observed, expected);
}

static void expect(const char *name, uint32_t observed, uint32_t expected) { report(observed == expected, name, observed, expected); }

static void check_led_pins(uint32_t color, uint32_t expected) {
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_LED, (color << 17) | (expected & 0xffffu));
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_LED, (color << 17) | (1u << 16) | (expected >> 16));
}

static void check_hex_pins(uint32_t digit, uint32_t pattern) { SIM_EVENT = EVENT_WORD(EVENT_CHECK_HEX, (digit << 8) | (pattern & 0x7fu)); }

static void check_gpio_pins(uint32_t field, uint32_t expected) {
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_GPIO, (field << 17) | (expected & 0xffffu));
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_GPIO, (field << 17) | (1u << 16) | (expected >> 16));
}

static void set_switches(uint32_t value) { SIM_EVENT = EVENT_WORD(EVENT_SET_SW, value & 0x3ffu); }

static void set_gpio(uint32_t value) {
    SIM_EVENT = EVENT_WORD(EVENT_SET_GPIO, value & 0xffffu);
    SIM_EVENT = EVENT_WORD(EVENT_SET_GPIO, (1u << 16) | (value >> 16));
}

static uint32_t wait_for(uint32_t address, uint32_t expected, uint32_t attempts) {
    for (uint32_t attempt = 0u; attempt < attempts; ++attempt) if (REG32(address) == expected) return 1u;
    return 0u;
}

__attribute__((noreturn)) static void io_finish(const char *program) {
    ee_printf("\n%s: %u/%u checks passed\n", program, checks_total - checks_failed, checks_total);
    app_finish();
}

#endif
