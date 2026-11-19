#ifndef PASS2_H
#define PASS2_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "parser.h"

#include "pass1.h"
#include "symbol_table.h"

typedef struct {
    uint32_t address;
    uint32_t machine_code;
    uint8_t byte_count;

    char source_line[PARSER_MAX_LINE_LENGTH];
} Pass2ListingEntry;

typedef struct {
    uint8_t *bytes;
    uint32_t size;
    uint32_t base_address;

    Pass2ListingEntry *listing_entries;
    size_t listing_count;
    size_t listing_capacity;
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

int pass2_write_listing(
    const Pass2Result *result,
    const SymbolTable *symbols,
    const char *filename
);

#endif
