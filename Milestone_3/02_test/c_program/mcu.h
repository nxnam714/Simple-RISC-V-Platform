#ifndef M3_MCU_H
#define M3_MCU_H

#include <stdint.h>

#define REG32(address)      (*(volatile uint32_t *)(uintptr_t)(address))

#define DMEM_BASE           0x84000000u
#define DMEM_BYTES          0x00004000u

#define LED_BASE            0x40000000u
#define HEX_BASE            0x40001000u
#define SW_BASE             0x40002000u
#define KEY_BASE            0x40003000u
#define TIMER_BASE          0x40004000u
#define GPIO_BASE           0x40005000u
#define SIM_BASE            0x4000f000u

#define LEDR                REG32(LED_BASE + 0x0u)
#define LEDG                REG32(LED_BASE + 0x4u)
#define LEDB                REG32(LED_BASE + 0x8u)
#define LED                 LEDR
#define HEX(n)              REG32(HEX_BASE + 4u * (uint32_t)(n))
#define SW                  REG32(SW_BASE  + 0x0u)
#define KEY                 REG32(KEY_BASE + 0x0u)
#define TIMER_CTRL          REG32(TIMER_BASE + 0x0u)
#define TIMER_COUNT_LO      REG32(TIMER_BASE + 0x4u)
#define TIMER_COUNT_HI      REG32(TIMER_BASE + 0x8u)
#define GPIO_OUT            REG32(GPIO_BASE + 0x0u)
#define GPIO_DIR            REG32(GPIO_BASE + 0x4u)
#define GPIO_IN             REG32(GPIO_BASE + 0x8u)
#define SIM_EVENT           REG32(SIM_BASE + 0x0u)
#define SIM_PRINT           REG32(SIM_BASE + 0x4u)
#define SIM_HALT            REG32(SIM_BASE + 0x8u)

#define TIMER_EN            0x1u
#define TIMER_CLR           0x2u

#define EVENT_FAIL          0x80000000u
#define EVENT_CASE          0x01u
#define EVENT_SET_SW        0x60u
#define EVENT_SET_KEY       0x61u
#define EVENT_CHECK_LED     0x62u
#define EVENT_CHECK_HEX     0x63u
#define EVENT_SET_GPIO      0x65u
#define EVENT_CHECK_GPIO    0x66u
#define EVENT_GROUP_BEGIN   0x7cu
#define EVENT_GROUP         0x7du
#define EVENT_SUMMARY       0x7fu

#define EVENT_WORD(code, payload) ((((uint32_t)(code) & 0x7fu) << 24) | ((uint32_t)(payload) & 0x00ffffffu))

static uint32_t tick_next;

static inline void tick_start(uint32_t period) {
    TIMER_CTRL = TIMER_EN | TIMER_CLR;
    tick_next  = period;
}

static inline uint32_t tick_elapsed(uint32_t period) {
    uint32_t now = TIMER_COUNT_LO;
    if ((int32_t)(now - tick_next) < 0) return 0u;
    do { tick_next += period; } while ((int32_t)(now - tick_next) >= 0);
    return 1u;
}

static const uint8_t hex_font[16] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71
};

#endif
