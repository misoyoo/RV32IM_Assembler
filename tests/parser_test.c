#include <stdio.h>
#include <string.h>

#include "parser.h"

int main(void)
{
    const char *test_lines[] = {
        "addi t0, t1, 10",
        "LOOP: addi t0, t1, 10",
        "lw t0, 8(sp)",
        "sw t0, -8(sp) # comment",
        ".equ IRQ, (1 << 7)",
        ".asciz \"Hello, world\"",
        "_start:",
        "# comment only",
        "count: .word 0"
    };

    int test_count = sizeof(test_lines) / sizeof(test_lines[0]);

    for (int i = 0; i < test_count; i++) {
        ParsedLine parsed_line;

        printf("\n===== Test %d =====\n", i + 1);
        printf("Input: %s\n", test_lines[i]);

        if (parse_line(test_lines[i], &parsed_line) != 0) {
            printf("Parse failed!\n");
            continue;
        }

        printf("Label: %s\n", parsed_line.label);
        printf("Mnemonic: %s\n", parsed_line.mnemonic);
        printf("Operand count: %d\n", parsed_line.operand_count);

        for (int j = 0; j < parsed_line.operand_count; j++) {
            printf(
                "Operand[%d]: %s\n",
                j,
                parsed_line.operands[j]
            );
        }
    }

    return 0;
}