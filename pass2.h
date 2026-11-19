
#ifndef PASS2_H
#define PASS2_H

#include <stdint.h>
#include <stdio.h>

#include "pass1.h"
#include "symbol_table.h"

typedef struct {
    uint8_t *bytes;
    uint32_t size;
    uint32_t base_address;
} Pass2Result;

int pass2_run(
    FILE *source,
    const SymbolTable *symbols,
    const Pass1Layout *layout,
    Pass2Result *result
);

void pass2_destroy(Pass2Result *result);

int pass2_write_binary(
    const Pass2Result *result,
    const char *filename
);

#endif
