
#include <stdio.h>
#include <stdint.h>

#include "pass1.h"
#include "symbol_table.h"

int main(void)
{
    FILE *source = tmpfile();

    if (source == NULL) {
        perror("tmpfile");
        return 1;
    }

    fputs(
        ".equ LEDR, 0xFF200000\n"
        ".equ SW, 0xFF200040\n"
        ".global _start\n"
        ".text\n"
        "_start: li sp, _stack_top\n"
        "    li s0, SW\n"
        "    li s1, LEDR\n"
        "    la s2, count\n"
        "main:\n"
        "    lw a0, 0(s0)\n"
        "    call show\n"
        "    lw t0, 0(s2)\n"
        "    addi t0, t0, 1\n"
        "    sw t0, 0(s2)\n"
        "    j main\n"
        "show:\n"
        "    sw a0, 0(s1)\n"
        "    ret\n"
        ".data\n"
        "count: .word 0\n",
        source
    );

    fflush(source);

    SymbolTable symbols;
    Pass1Layout layout;

    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return 1;
    }

    if (pass1_run(source, &symbols, &layout) != 0) {
        printf("Professor example test failed!\n");
        symbol_table_destroy(&symbols);
        fclose(source);
        return 1;
    }

    printf("===== Professor Example Test =====\n");

    printf("Text Base : 0x%08X\n", layout.text_base);
    printf("Text Size : %u bytes\n", layout.text_size);
    printf("Data Base : 0x%08X\n", layout.data_base);
    printf("Data Size : %u bytes\n", layout.data_size);

    printf("\n===== Symbol Table =====\n");

    for (size_t i = 0; i < symbols.count; i++) {
        printf(
            "%-12s = 0x%08X\n",
            symbols.symbols[i].name,
            symbols.symbols[i].address
        );
    }

    symbol_table_destroy(&symbols);
    fclose(source);

    return 0;
}
