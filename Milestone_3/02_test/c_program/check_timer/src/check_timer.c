#include "io_test.h"


static volatile uint32_t work[256];
static volatile uint8_t  bytes_a[1024];
static volatile uint8_t  bytes_b[1024];

static const uint32_t expected_cycles[5] = {23894u, 2258u, 63502u, 12619u, 7180u};



static void timer_free_run(void) {
    TIMER_CTRL = TIMER_EN | TIMER_CLR;
}


static uint32_t time_loop(uint32_t n) {
    uint32_t first, last;
    __asm__ volatile ("lw %0, 4(%3)\n1: addi %2, %2, -1\nbnez %2, 1b\nlw %1, 4(%3)" : "=&r"(first), "=&r"(last), "+r"(n) : "r"(TIMER_BASE) : "memory");
    return last - first;
}





static uint32_t time_nops(void) {
    uint32_t first, last;
    __asm__ volatile ("lw %0, 4(%2)\n.rept 100\nnop\n.endr\nlw %1, 4(%2)" : "=&r"(first), "=&r"(last) : "r"(TIMER_BASE) : "memory");
    return last - first;
}




static uint32_t time_memory(void) {
    uint32_t first, last, scratch;
    __asm__ volatile ("lw %0, 4(%3)\n.rept 32\nsw %2, 0(%4)\nlw %2, 0(%4)\nlbu %2, 1(%4)\nsh %2, 2(%4)\nlhu %2, 2(%4)\n.endr\nlw %1, 4(%3)"
                      : "=&r"(first), "=&r"(last), "=&r"(scratch) : "r"(TIMER_BASE), "r"(work) : "memory");
    return last - first;
}




static uint32_t time_jumps(void) {
    uint32_t first, last, link;
    __asm__ volatile ("lw %0, 4(%3)\n.rept 20\njal %2, 1f\n1: beq x0, x0, 2f\n2: bne x0, x0, 3f\n3: auipc %2, 0\njalr x0, 8(%2)\n.endr\nlw %1, 4(%3)"
                      : "=&r"(first), "=&r"(last), "=&r"(link) : "r"(TIMER_BASE) : "memory");
    return last - first;
}



static uint32_t time_back_to_back(void) {
    uint32_t steps = 0u, previous, current;
    previous       = TIMER_COUNT_LO;
    for (uint32_t index = 0u; index < 50u; ++index) {
        __asm__ volatile ("lw %0, 4(%1)" : "=r"(current) : "r"(TIMER_BASE) : "memory");
        __asm__ volatile ("lw %0, 4(%1)" : "=r"(previous) : "r"(TIMER_BASE) : "memory");
        if (previous - current == 1u) ++steps;
    }
    return steps;
}





static void timed_row(const char *format, uint32_t a, uint32_t b, uint32_t observed, uint32_t expected) {
    uint32_t passed = observed == expected;
    ++checks_total;
    if (!passed) ++checks_failed;
    ee_printf(passed ? "@P " : "@F ");
    ee_printf(format, a, b);
    ee_printf("\n");
    if (!passed) ee_printf("        observed = %u cycles, expected = %u cycles\n", observed, expected);
}




static void part_instruction_timing(void) {
    static const uint32_t loops[5] = {1u, 2u, 10u, 1000u, 50000u};

    section(1u, "Timer measures instruction time");
    timer_free_run();
    for (uint32_t index = 0u; index < 5u; ++index) {
        uint32_t n = loops[index];
        timed_row(n == 1u ? "Loop of %u iteration takes %u cycles" : "Loop of %u iterations takes %u cycles", n, n + n + 1u, time_loop(n), n + n + 1u);
    }
    expect("100 NOP instructions take 101 cycles", time_nops(), 101u);
    expect("160 load and store instructions take 161 cycles", time_memory(), 161u);
    expect("100 jump and branch instructions take 101 cycles", time_jumps(), 101u);
    expect("Two COUNT_LO reads in a row differ by 1", time_back_to_back(), 50u);
}



static void part_timer_rules(void) {
    uint32_t a, b, c;
    uint32_t base = TIMER_BASE;

    section(2u, "Timer control");

    __asm__ volatile ("sw %3, 0(%2)\nlw %0, 4(%2)\nlw %1, 4(%2)" : "=&r"(a), "=&r"(b) : "r"(base), "r"(TIMER_EN | TIMER_CLR) : "memory");
    report(a == 0u && b == 1u, "Count starts at 0 and goes up by 1 when EN is set", b, 1u);
    expect("CTRL reads back EN, and CLR reads 0", TIMER_CTRL, 1u);

    TIMER_CTRL = 0u;
    a          = TIMER_COUNT_LO;
    c          = 200u;
    __asm__ volatile ("1: addi %0, %0, -1\nbnez %0, 1b" : "+r"(c) : : "memory");
    expect("Count holds when EN is 0", TIMER_COUNT_LO, a);

    __asm__ volatile ("sw %3, 0(%2)\nlw %0, 4(%2)\nlw %1, 4(%2)" : "=&r"(c), "=&r"(b) : "r"(base), "r"(TIMER_EN) : "memory");
    report(c == a && b == a + 1u, "Count continues from the held value when EN is 1", b, a + 1u);

    __asm__ volatile ("sw %3, 0(%2)\nlw %0, 4(%2)\nlw %1, 4(%2)" : "=&r"(a), "=&r"(b) : "r"(base), "r"(TIMER_EN | TIMER_CLR) : "memory");
    report(a == 0u && b == 1u, "Writing CLR while running sets the count to 0", b, 1u);
    TIMER_CTRL = TIMER_CLR;
    report(TIMER_COUNT_LO == 0u && TIMER_CTRL == 0u, "Writing CLR while stopped sets the count to 0 and EN to 0", TIMER_COUNT_LO, 0u);
    expect("COUNT_HI stays 0 while the count is small", TIMER_COUNT_HI, 0u);
}




static void bubble_sort(void) {
    uint32_t seed = 12345u;
    for (uint32_t index = 0u; index < 64u; ++index) {
        seed        = seed * 1103515245u + 12345u;
        work[index] = seed >> 8;
    }
    for (uint32_t pass = 0u; pass < 63u; ++pass) {
        for (uint32_t index = 0u; index < 63u - pass; ++index) {
            uint32_t left = work[index], right = work[index + 1u];
            if (left > right) {
                work[index]      = right;
                work[index + 1u] = left;
            }
        }
    }
}




static uint32_t hanoi_moves;
static void hanoi(uint32_t disks, uint32_t from, uint32_t to, uint32_t spare) {
    if (disks == 0u) return;
    hanoi(disks - 1u, from, spare, to);
    ++hanoi_moves;
    hanoi(disks - 1u, spare, to, from);
}



static uint32_t crc32(void) {
    uint32_t crc = 0xffffffffu;
    for (uint32_t index = 0u; index < 1024u; ++index) {
        crc ^= bytes_a[index];
        for (uint32_t bit = 0u; bit < 8u; ++bit) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}




static uint32_t products(void) {
    uint32_t sum = 0u;
    for (uint32_t index = 1u; index <= 100u; ++index) sum += multiply_u32(index * 7919u, index + 104729u);
    return sum;
}


static void copy_bytes(void) {
    for (uint32_t index = 0u; index < 1024u; ++index) bytes_b[index] = bytes_a[index];
}





static uint32_t decimal_digits(uint32_t value, uint8_t digits[8]) {
    static const uint32_t powers[8] = {10000000u, 1000000u, 100000u, 10000u, 1000u, 100u, 10u, 1u};
    uint32_t overflow = value > 99999999u;
    for (uint32_t index = 0u; index < 8u; ++index) {
        uint8_t digit = 0u;
        while (value >= powers[index]) {
            value -= powers[index];
            ++digit;
        }
        digits[7u - index] = digit;
    }
    return overflow;
}




static void show_decimal(uint32_t value) {
    uint8_t  digits[8];
    uint32_t leading = 1u;
    decimal_digits(value, digits);
    for (uint32_t index = 8u; index-- > 0u;) {
        uint32_t blank = leading && digits[index] == 0u && index != 0u;
        if (!blank) leading = 0u;
        HEX(index) = blank ? 0u : hex_font[digits[index]];
    }
}


static void check_display(uint32_t value) {
    uint8_t  digits[8];
    uint32_t leading = 1u;
    decimal_digits(value, digits);
    for (uint32_t index = 8u; index-- > 0u;) {
        uint32_t blank = leading && digits[index] == 0u && index != 0u;
        if (!blank) leading = 0u;
        check_hex_pins(index, blank ? 0u : hex_font[digits[index]]);
    }
}



static void part_workloads(void) {
    static const char *const names[5] = {"Bubble sort of 64 words takes %u cycles", "Tower of Hanoi with 10 disks takes %u cycles", "CRC-32 of 1 KiB takes %u cycles", "100 software multiplies take %u cycles", "Byte copy of 1 KiB takes %u cycles"};
    static const uint32_t expected_results[5] = {0u, 1023u, 0x6fea9368u, 0xc49ad9a8u, 0u};
    uint32_t measured[5];

    section(3u, "Timer measures C programs");
    for (uint32_t index = 0u; index < 1024u; ++index) bytes_a[index] = (uint8_t)(index * 37u + 11u);

    for (uint32_t index = 0u; index < 5u; ++index) {
        uint32_t start, cycles, value = 0u;
        LEDR                          = 1u << index;
        timer_free_run();
        start = TIMER_COUNT_LO;
        if (index == 0u) bubble_sort();
        else if (index == 1u) {
            hanoi_moves = 0u;
            hanoi(10u, 1u, 3u, 2u);
            value = hanoi_moves;
        } else if (index == 2u) value = crc32();
        else if (index == 3u) value = products();
        else copy_bytes();
        cycles = TIMER_COUNT_LO - start;
        if (index == 0u) for (uint32_t i = 0u; i < 63u; ++i) if (work[i] > work[i + 1u]) value = 1u;
        if (index == 4u) for (uint32_t i = 0u; i < 1024u; ++i) if (bytes_b[i] != bytes_a[i]) value = 1u;
        timed_row(names[index], expected_cycles[index], 0u, value == expected_results[index] ? cycles : 0u, expected_cycles[index]);
        measured[index] = cycles;
    }
    for (uint32_t index = 0u; index < 5u; ++index) {
        show_decimal(measured[index]);
        check_display(measured[index]);
    }
    check_led_pins(0u, 0x10u);
    expect("HEX pins show each measured cycle count", LEDR, 0x10u);
}


int main(void) {
    ee_printf("\n\nHCMUT - Computer Architecture Laboratory - DOELAB\n");
    ee_printf("Milestone 3 timer test\n");
    part_instruction_timing();
    part_timer_rules();
    part_workloads();
    io_finish("Cycle measurement");
}
