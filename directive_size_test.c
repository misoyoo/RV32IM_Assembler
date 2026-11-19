#include <stdio.h>
#include <stdint.h>

#include "parser.h"
#include "directives.h"
#include "symbol_table.h"

int main(void)
{
    SymbolTable symbols;

    symbol_table_init(&symbols);

    symbol_table_add(&symbols, "SCREEN_H", 120);
    symbol_table_add(&symbols, "LINE_MAX", 64);

    const char *test_lines[] = {
        ".byte 0x3F, 0x06, 0x5B, 0x4F",
        ".half 0xF800, 0x07E0",
        ".word 40, 40",
        ".ascii \"ABC\"",
        ".asciz \"ABC\"",
        ".asciz \"\\n> \"",
        ".space LINE_MAX",
        ".space SCREEN_H * 512 / 4",
        ".align 2"
    };

    uint32_t addresses[] = {
        0x2000,
        0x2000,
        0x2000,
        0x2000,
        0x2000,
        0x2000,
        0x2000,
        0x2000,
        0x2001
    };

    int count = sizeof(test_lines) / sizeof(test_lines[0]);

    printf("===== Directive Size Test =====\n");

    for (int i = 0; i < count; i++) {
        ParsedLine parsed_line;
        uint32_t size;

        if (parse_line(test_lines[i], &parsed_line) != 0) {
            printf("Parse failed: %s\n", test_lines[i]);
            continue;
        }

        int result = get_directive_size(
            &parsed_line,
            &symbols,
            addresses[i],
            &size
        );

        if (result == 1) {
            printf(
                "%-35s Size = %u bytes\n",
                test_lines[i],
                size
            );
        }
        else {
            printf("Failed: %s\n", test_lines[i]);
        }
    }

    symbol_table_destroy(&symbols);

    return 0;
}