#include <stdio.h>
#include <stdint.h>

#include "parser.h"
#include "directives.h"
#include "symbol_table.h"

int main(void)
{
    SymbolTable symbols;

    if (symbol_table_init(&symbols) != 0) {
        return 1;
    }

    const char *equ_lines[] = {
        ".equ IRQ_TIMER, 16",
        ".equ IRQ_KEY, 18",
        ".equ SCREEN_H, 120",
        ".equ LEDR_BASE, 0xFF200000"
    };

    int count = sizeof(equ_lines) / sizeof(equ_lines[0]);

    printf("===== .equ Tests =====\n");

    for (int i = 0; i < count; i++) {
        ParsedLine parsed_line;

        if (parse_line(equ_lines[i], &parsed_line) != 0) {
            printf("Parse failed: %s\n", equ_lines[i]);
            continue;
        }

        if (process_equ_directive(&parsed_line, &symbols) == 1) {
            printf("Added: %s\n", parsed_line.operands[0]);
        }
        else {
            printf("Failed: %s\n", equ_lines[i]);
        }
    }

    printf("\n===== Expression Tests =====\n");

    const char *expressions[] = {
        "(1 << IRQ_TIMER) | (1 << IRQ_KEY)",
        "0x80000000 | IRQ_TIMER",
        "SCREEN_H * 512 / 4",
        "'0'",
        "(1 + 2) * 3"
    };

    for (int i = 0; i < 5; i++) {
        uint32_t value;

        if (evaluate_expression(
                expressions[i],
                &symbols,
                &value
            ) == 0) {
            printf(
                "%s = 0x%08X (%u)\n",
                expressions[i],
                value,
                value
            );
        }
        else {
            printf("Evaluation failed: %s\n", expressions[i]);
        }
    }

    symbol_table_destroy(&symbols);

    return 0;
}