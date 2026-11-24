#ifndef M3_CHECK_ROW_H
#define M3_CHECK_ROW_H

#include "custom_printf.h"
#include <stdint.h>

static uint32_t row_failed;

static void section(uint32_t id, const char *name) { ee_printf("\n[%u] %s\n", (unsigned)id, name); }

static void row(const char *behavior) {
    ee_printf("%s%s\n", row_failed ? "@F " : "@P ", behavior);
    row_failed = 0u;
}

#endif
