#ifndef PASS1_H
#define PASS1_H

#include <stdint.h>
#include <stdio.h>

#include "symbol_table.h"

#define DEFAULT_TEXT_BASE 0x00010000u

typedef struct {
    uint32_t text_base;
    uint32_t text_size;

    uint32_t data_base;
    uint32_t data_size;

    uint32_t bss_base;
    uint32_t bss_size;
} Pass1Layout;

int pass1_run(
    FILE *source,
    SymbolTable *symbols,
    Pass1Layout *layout
);

#endif
