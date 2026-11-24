#include "app_runtime.h"
#include "mcu.h"
#include "check_row.h"

#define GROUP_COUNT    12u
#define REG8(address)  (*(volatile uint8_t *)(uintptr_t)(address))
#define REG16(address) (*(volatile uint16_t *)(uintptr_t)(address))

static uint32_t case_count;
static uint32_t group_id;
static uint32_t group_failed;
static uint32_t groups_failed;

static void group_begin(uint32_t id)
{
    group_id     = id;
    group_failed = 0u;
    SIM_EVENT    = EVENT_WORD(EVENT_GROUP_BEGIN, id);
}





static void group_end(void)
{
    SIM_EVENT = (group_failed ? EVENT_FAIL : 0u) | EVENT_WORD(EVENT_GROUP, group_id);
    if (group_failed) ++groups_failed;
}





static void check(uint32_t passed)
{
    SIM_EVENT = (passed ? 0u : EVENT_FAIL) | EVENT_WORD(EVENT_CASE, case_count);
    ++case_count;
    if (!passed) {
        group_failed = 1u;
        row_failed   = 1u;
    }
}



static void check_led_pins(uint32_t color, uint32_t expected)
{
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_LED, (color << 17) | (expected & 0xffffu));
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_LED, (color << 17) | (1u << 16) | (expected >> 16));
}



static void check_hex_pins(uint32_t digit, uint32_t pattern)
{
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_HEX, (digit << 8) | (pattern & 0x7fu));
}




static void check_gpio_pins(uint32_t field, uint32_t expected)
{
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_GPIO, (field << 17) | (expected & 0xffffu));
    SIM_EVENT = EVENT_WORD(EVENT_CHECK_GPIO, (field << 17) | (1u << 16) | (expected >> 16));
}


static void set_gpio(uint32_t value)
{
    SIM_EVENT = EVENT_WORD(EVENT_SET_GPIO, value & 0xffffu);
    SIM_EVENT = EVENT_WORD(EVENT_SET_GPIO, (1u << 16) | (value >> 16));
}





static void set_switches(uint32_t value)
{
    SIM_EVENT = EVENT_WORD(EVENT_SET_SW, value & 0x3ffu);
}





static void set_keys(uint32_t pressed)
{
    SIM_EVENT = EVENT_WORD(EVENT_SET_KEY, pressed & 0xfu);
}



static uint32_t wait_for(uint32_t address, uint32_t expected)
{
    for (uint32_t attempt = 0u; attempt < 64u; ++attempt) {
        if (REG32(address) == expected) return 1u;
    }
    return 0u;
}





static const uint32_t led_mask[3] = {0x0003ffffu, 0x000000ffu, 0x00000001u};





static volatile uint32_t buffer[4];

static void test_dmem(void)
{
    volatile uint8_t  *bytes         = (volatile uint8_t *)buffer;
    volatile uint16_t *halves        = (volatile uint16_t *)buffer;
    volatile int8_t   *signed_bytes  = (volatile int8_t *)buffer;
    volatile int16_t  *signed_halves = (volatile int16_t *)buffer;
    uint32_t last_address            = DMEM_BASE + DMEM_BYTES - 4u;
    uint32_t saved;

    group_begin(1u);
    section(1u, "DMEM through the bus");
    buffer[0] = 0x12345678u;
    buffer[1] = 0x9abcdef0u;
    check(buffer[0] == 0x12345678u);
    check(buffer[1] == 0x9abcdef0u);
    row("Stores and returns a 32-bit word");
    check(bytes[0] == 0x78u && bytes[1] == 0x56u && bytes[2] == 0x34u && bytes[3] == 0x12u);
    check(halves[0] == 0x5678u && halves[1] == 0x1234u);
    row("Reads a single byte or halfword");
    bytes[1] = 0xa5u;
    bytes[2] = 0x5au;
    check(buffer[0] == 0x125aa578u);
    halves[1] = 0xbeefu;
    check(buffer[0] == 0xbeefa578u);
    row("Writes a single byte or halfword");
    check(signed_bytes[1] == (int8_t)0xa5);
    check(signed_halves[1] == (int16_t)0xbeef);
    row("LB and LH keep the sign of the value");
    check(buffer[1] == 0x9abcdef0u);
    saved = REG32(last_address);
    REG32(last_address) = 0xc001d00du;
    check(REG32(last_address) == 0xc001d00du);
    REG32(last_address) = saved;
    row("Last word of DMEM can be used");
    group_end();
}





static void test_unmapped(void)
{
    uint32_t first_word = REG32(DMEM_BASE);

    group_begin(2u);
    section(2u, "Unmapped addresses");
    check(REG32(0x40006000u) == 0u);
    check(REG32(0x4000e000u) == 0u);
    check(REG32(0x40010000u) == 0u);
    check(REG32(0x90000000u) == 0u);
    check(REG32(DMEM_BASE + DMEM_BYTES) == 0u);
    row("Unmapped addresses read as 0");
    REG32(DMEM_BASE + DMEM_BYTES) = ~first_word;
    check(REG32(DMEM_BASE) == first_word);
    REG32(DMEM_BASE) = first_word;
    row("Writing past the end of DMEM changes nothing");
    group_end();
}





static void test_led(void)
{
    static const uint32_t patterns[6] = {0xffffffffu, 0xa5a5a5a5u, 0x5a5a5a5au, 0x00000001u, 0x80000000u, 0u};

    group_begin(3u);
    section(3u, "LED");
    check(LEDR == 0u && LEDG == 0u && LEDB == 0u);
    for (uint32_t color = 0u; color < 3u; ++color) check_led_pins(color, 0u);
    row("LED registers and pins start at 0");
    LED = 0x155u;
    REG32(0x40006000u) = 0x3ffu;
    REG32(0x40010000u) = 0x3ffu;
    REG32(0x40100000u) = 0x3ffu;
    check(LED == 0x155u);
    check(REG32(LED_BASE + 0xcu) == 0u);
    REG32(LED_BASE + 0xcu) = 0x2aau;
    check(LED == 0x155u);
    check_led_pins(0u, 0x155u);
    row("Writing other addresses does not change the LEDs");
    LED = 0u;
    for (uint32_t color = 0u; color < 3u; ++color) {
        for (uint32_t index = 0u; index < 6u; ++index) {
            REG32(LED_BASE + 4u * color) = patterns[index];
            check((REG32(LED_BASE + 4u * color) & led_mask[color]) == (patterns[index] & led_mask[color]));
            check_led_pins(color, patterns[index]);
        }
    }
    row("Each LED register drives its pins");
    LEDR = 0x12345678u;
    LEDG = 0x9abcdef0u;
    LEDB = 0x0f1e2d3du;
    check((LEDR & led_mask[0]) == (0x12345678u & led_mask[0]) && (LEDG & led_mask[1]) == (0x9abcdef0u & led_mask[1]) && (LEDB & led_mask[2]) == (0x0f1e2d3du & led_mask[2]));
    check_led_pins(0u, 0x12345678u);
    check_led_pins(1u, 0x9abcdef0u);
    check_led_pins(2u, 0x0f1e2d3du);
    row("Red, green and blue are separate registers");
    LEDR = 0u;
    LEDG = 0u;
    LEDB = 0u;
    for (uint32_t color = 0u; color < 3u; ++color) check_led_pins(color, 0u);
    REG8(LED_BASE + 1u) = 0xabu;
    REG16(LED_BASE + 2u) = 0x0003u;
    check((LEDR & led_mask[0]) == 0x0003ab00u);
    check(REG8(LED_BASE + 1u) == 0xabu && (REG16(LED_BASE + 2u) & 0x3u) == 0x3u);
    check_led_pins(0u, 0x0003ab00u);
    row("LED registers accept byte and halfword writes");
    LEDR = 0u;
    group_end();
}



static void test_hex(void)
{
    group_begin(4u);
    section(4u, "Seven-segment displays");
    for (uint32_t digit = 0u; digit < 8u; ++digit) {
        check((HEX(digit) & 0x7fu) == 0u);
        check_hex_pins(digit, 0u);
    }
    check(REG32(HEX_BASE + 0x20u) == 0u);
    row("Digit registers start at 0, all segments off");
    for (uint32_t digit = 0u; digit < 8u; ++digit) HEX(digit) = hex_font[digit + 1u];
    for (uint32_t digit = 0u; digit < 8u; ++digit) {
        check((HEX(digit) & 0x7fu) == hex_font[digit + 1u]);
        check_hex_pins(digit, hex_font[digit + 1u]);
    }
    row("Each digit has its own register, pins are active low");
    HEX(2) = 0xffffffffu;
    check((HEX(2) & 0x7fu) == 0x7fu);
    check((HEX(1) & 0x7fu) == hex_font[2] && (HEX(3) & 0x7fu) == hex_font[4]);
    check_hex_pins(2u, 0x7fu);
    row("Each digit uses bits 6:0");
    REG8(HEX_BASE + 0x9u) = 0x12u;
    check((HEX(2) & 0x7fu) == 0x7fu);
    REG8(HEX_BASE + 0x8u) = 0x3fu;
    check((HEX(2) & 0x7fu) == 0x3fu);
    REG16(HEX_BASE + 0xau) = 0x1234u;
    check((HEX(2) & 0x7fu) == 0x3fu);
    REG16(HEX_BASE + 0x8u) = 0x0055u;
    check((HEX(2) & 0x7fu) == 0x55u);
    check_hex_pins(2u, 0x55u);
    row("Digit registers accept byte and halfword writes");
    HEX(2) = 0x7fu;
    REG32(HEX_BASE + 0x20u) = 0x7fu;
    REG32(HEX_BASE + 0x24u) = 0x7fu;
    check((HEX(0) & 0x7fu) == hex_font[1] && (HEX(1) & 0x7fu) == hex_font[2] && (HEX(7) & 0x7fu) == hex_font[8]);
    row("Writing past HEX7 changes nothing");
    for (uint32_t digit = 0u; digit < 8u; ++digit) {
        HEX(digit) = 0u;
        check_hex_pins(digit, 0u);
    }
    group_end();
}




static void test_switches(void)
{
    static const uint32_t patterns[6] = {0x3ffu, 0x2aau, 0x155u, 0x001u, 0x200u, 0x000u};

    group_begin(5u);
    section(5u, "Switches");
    check(SW == 0u);
    check(REG32(SW_BASE + 0x4u) == 0u);
    row("SW starts at 0");
    for (uint32_t index = 0u; index < 6u; ++index) {
        set_switches(patterns[index]);
        check(wait_for(SW_BASE, patterns[index]));
    }
    row("SW reads the switch pins");
    set_switches(0x155u);
    check(wait_for(SW_BASE, 0x155u));
    SW = 0u;
    check(SW == 0x155u);
    row("SW is read-only");
    group_end();
}


static void test_key_state(void)
{
    static const uint32_t patterns[5] = {0x1u, 0x6u, 0x8u, 0xfu, 0x0u};

    group_begin(6u);
    section(6u, "Buttons");
    check(KEY == 0u);
    check(REG32(KEY_BASE + 0x4u) == 0u);
    row("KEY starts at 0");
    for (uint32_t index = 0u; index < 5u; ++index) {
        set_keys(patterns[index]);
        check(wait_for(KEY_BASE, patterns[index]));
        check(KEY == patterns[index]);
    }
    row("KEY reads 1 when a button is pressed");
    KEY = 0xfu;
    check(KEY == 0u);
    row("KEY is read-only");
    group_end();
}


static void test_key_level(void)
{
    uint32_t held = 1u;

    group_begin(7u);
    section(7u, "Button press and release");
    set_keys(0x4u);
    check(wait_for(KEY_BASE, 0x4u));
    for (uint32_t index = 0u; index < 64u; ++index) held &= (KEY == 0x4u);
    check(held);
    row("KEY stays 1 while a button is held");
    set_keys(0x5u);
    check(wait_for(KEY_BASE, 0x5u));
    set_keys(0x1u);
    check(wait_for(KEY_BASE, 0x1u));
    KEY = 0u;
    check(KEY == 0x1u);
    row("Each button works on its own");
    set_keys(0x0u);
    check(wait_for(KEY_BASE, 0x0u));
    held = 1u;
    for (uint32_t index = 0u; index < 64u; ++index) held &= (KEY == 0u);
    check(held);
    row("KEY returns to 0 when a button is released");
    check(REG32(KEY_BASE + 0x4u) == 0u);
    group_end();
}





static void test_timer_registers(void)
{
    uint32_t first;
    uint32_t second;

    group_begin(8u);
    section(8u, "Timer: registers");
    check(TIMER_CTRL == 0u);
    check(TIMER_COUNT_LO == 0u);
    check(TIMER_COUNT_HI == 0u);
    check(REG32(TIMER_BASE + 0xcu) == 0u);
    row("Timer registers start at 0");
    TIMER_COUNT_LO = 0x55u;
    TIMER_COUNT_HI = 0x55u;
    check(TIMER_COUNT_LO == 0u && TIMER_COUNT_HI == 0u);
    row("COUNT_LO and COUNT_HI are read-only");
    TIMER_CTRL = 0xfffffffdu;
    check(TIMER_CTRL == TIMER_EN);
    row("CTRL stores only EN, and CLR always reads 0");
    first  = TIMER_COUNT_LO;
    second = TIMER_COUNT_LO;
    check(second != first);
    TIMER_COUNT_LO = 0u;
    check(TIMER_COUNT_LO != 0u);
    row("Timer counts when EN is 1");
    REG8(TIMER_BASE + 1u) = 0u;
    check(TIMER_CTRL == TIMER_EN);
    TIMER_CTRL = TIMER_CLR;
    check(TIMER_CTRL == 0u);
    check(TIMER_COUNT_LO == 0u);
    row("Writing CLR sets the count to 0");
    REG8(TIMER_BASE) = TIMER_EN;
    check(TIMER_CTRL == TIMER_EN);
    TIMER_CTRL = TIMER_CLR;
    check(TIMER_COUNT_HI == 0u);
    row("CTRL accepts byte writes");
    group_end();
}




static void test_timer_count(void)
{
    uint32_t read_1;
    uint32_t read_2;
    uint32_t read_5;
    uint32_t high;

    group_begin(9u);
    section(9u, "Timer: counting");
    __asm__ volatile (
        "sw   %[start], 0(%[timer])\n\t"
        "lw   %[read_1], 4(%[timer])\n\t"
        "lw   %[read_2], 4(%[timer])\n\t"
        "nop\n\t"
        "nop\n\t"
        "lw   %[read_5], 4(%[timer])\n\t"
        "lw   %[high], 8(%[timer])\n\t"
        : [read_1] "=&r" (read_1), [read_2] "=&r" (read_2), [read_5] "=&r" (read_5), [high] "=&r" (high)
        : [timer] "r" (TIMER_BASE), [start] "r" (TIMER_EN | TIMER_CLR)
        : "memory");
    check(read_1 == 0u);
    check(read_2 == 1u);
    check(read_5 == 4u);
    check(high == 0u);
    row("Count goes up by 1 every clock cycle");
    group_end();
}




static void test_timer_hold(void)
{
    uint32_t held_1;
    uint32_t held_2;
    uint32_t resume_1;
    uint32_t resume_2;

    group_begin(10u);
    section(10u, "Timer: hold and resume");
    __asm__ volatile (
        "sw   zero, 0(%[timer])\n\t"
        "lw   %[held_1], 4(%[timer])\n\t"
        "lw   %[held_2], 4(%[timer])\n\t"
        "sw   %[enable], 0(%[timer])\n\t"
        "lw   %[resume_1], 4(%[timer])\n\t"
        "lw   %[resume_2], 4(%[timer])\n\t"
        : [held_1] "=&r" (held_1), [held_2] "=&r" (held_2), [resume_1] "=&r" (resume_1), [resume_2] "=&r" (resume_2)
        : [timer] "r" (TIMER_BASE), [enable] "r" (TIMER_EN)
        : "memory");
    check(held_1 == held_2);
    check(resume_1 == held_1);
    check(resume_2 == held_1 + 1u);
    row("Count holds when EN is 0 and continues when EN is 1");
    group_end();
}



static void test_timer_clear(void)
{
    uint32_t read_1;
    uint32_t read_2;
    uint32_t stopped_1;
    uint32_t stopped_2;

    group_begin(11u);
    section(11u, "Timer: clear");
    __asm__ volatile (
        "sw   %[start], 0(%[timer])\n\t"
        "lw   %[read_1], 4(%[timer])\n\t"
        "lw   %[read_2], 4(%[timer])\n\t"
        "sw   %[clear], 0(%[timer])\n\t"
        "lw   %[stopped_1], 4(%[timer])\n\t"
        "lw   %[stopped_2], 4(%[timer])\n\t"
        : [read_1] "=&r" (read_1), [read_2] "=&r" (read_2), [stopped_1] "=&r" (stopped_1), [stopped_2] "=&r" (stopped_2)
        : [timer] "r" (TIMER_BASE), [start] "r" (TIMER_EN | TIMER_CLR), [clear] "r" (TIMER_CLR)
        : "memory");
    check(read_1 == 0u);
    check(read_2 == 1u);
    check(stopped_1 == 0u);
    check(stopped_2 == 0u);
    check(TIMER_CTRL == 0u);
    row("Writing CLR while running sets the count to 0");
    group_end();
}





static void test_gpio(void)
{
    group_begin(12u);
    section(12u, "GPIO");
    check(GPIO_OUT == 0u && GPIO_DIR == 0u && GPIO_IN == 0u);
    check_gpio_pins(1u, 0u);
    check(REG32(GPIO_BASE + 0xcu) == 0u);
    row("GPIO registers start at 0, all pins are inputs");
    set_gpio(0xa5a55a5au);
    check(wait_for(GPIO_BASE + 0x8u, 0xa5a55a5au));
    GPIO_OUT = 0x12345678u;
    check(GPIO_OUT == 0x12345678u);
    check(GPIO_IN == 0xa5a55a5au);
    check_gpio_pins(0u, 0x12345678u);
    check_gpio_pins(1u, 0u);
    row("GPIO_IN reads the input pins");
    GPIO_DIR = 0xffff0000u;
    check(GPIO_DIR == 0xffff0000u);
    check_gpio_pins(1u, 0xffff0000u);
    check(wait_for(GPIO_BASE + 0x8u, 0x12345a5au));
    GPIO_DIR = 0xffffffffu;
    check(wait_for(GPIO_BASE + 0x8u, 0x12345678u));
    GPIO_OUT = 0x87654321u;
    check_gpio_pins(0u, 0x87654321u);
    check(wait_for(GPIO_BASE + 0x8u, 0x87654321u));
    row("Output pins drive GPIO_OUT and read it back");
    GPIO_DIR = 0x0000ffffu;
    check(wait_for(GPIO_BASE + 0x8u, 0xa5a54321u));
    GPIO_IN = 0u;
    check(GPIO_IN == 0xa5a54321u);
    row("Each pin has its own direction, GPIO_IN is read-only");
    GPIO_DIR = 0u;
    check(wait_for(GPIO_BASE + 0x8u, 0xa5a55a5au));
    GPIO_DIR = 0u;
    GPIO_OUT = 0u;
    REG8(GPIO_BASE + 2u) = 0x5au;
    check(GPIO_OUT == 0x005a0000u);
    REG16(GPIO_BASE + 4u) = 0x00ffu;
    check(GPIO_DIR == 0x000000ffu);
    GPIO_DIR = 0u;
    GPIO_OUT = 0u;
    set_gpio(0u);
    check(wait_for(GPIO_BASE + 0x8u, 0u));
    check_gpio_pins(0u, 0u);
    check_gpio_pins(1u, 0u);
    GPIO_OUT = 0x11223344u;
    REG8(GPIO_BASE + 1u) = 0xabu;
    check(GPIO_OUT == 0x1122ab44u);
    REG16(GPIO_BASE + 2u) = 0xbeefu;
    check(GPIO_OUT == 0xbeefab44u);
    GPIO_DIR = 0u;
    REG8(GPIO_BASE + 5u) = 0xf0u;
    check(GPIO_DIR == 0x0000f000u);
    check_gpio_pins(1u, 0x0000f000u);
    row("GPIO registers accept byte and halfword writes");
    GPIO_DIR = 0u;
    GPIO_OUT = 0u;
    group_end();
}


int main(void)
{
    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n");
    ee_printf("Milestone 3 register test\n");
    test_dmem();
    test_unmapped();
    test_led();
    test_hex();
    test_switches();
    test_key_state();
    test_key_level();
    test_timer_registers();
    test_timer_count();
    test_timer_hold();
    test_timer_clear();
    test_gpio();
    SIM_EVENT = EVENT_WORD(EVENT_SUMMARY, case_count);
    ee_printf("\nGroups passed:   %u/%u\n", (unsigned)(GROUP_COUNT - groups_failed), (unsigned)GROUP_COUNT);
    ee_printf("Final result:    %s\n\n", groups_failed == 0u ? "PASS" : "FAIL");
    app_finish();
}
