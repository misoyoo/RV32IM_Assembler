#ifndef DIRECTIVES_H
#define DIRECTIVES_H

#include <stdint.h>

#include "parser.h"
#include "symbol_table.h"

int evaluate_expression(
    const char *expression,
    const SymbolTable *symbols,
    uint32_t *value
);

int process_equ_directive(
    const ParsedLine *parsed_line,
    SymbolTable *symbols
);

int get_directive_size(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t current_address,
    uint32_t *size
);

#endif