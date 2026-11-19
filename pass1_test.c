
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
        ".equ CONST, 10\n"
        ".text\n"
        ".global _start\n"
        "_start:\n"
        "    li t0, 10\n"
        "    li t1, 50000000\n"
        "    la t2, DATA\n"
        "    j LOOP\n"
        "LOOP:\n"
        "    addi t0, t0, -1\n"
        ".data\n"
        "    .align 2\n"
        "DATA:\n"
        "    .word CONST\n"
        "    .byte 1, 2, 3\n"
        "    .align 2\n",
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
        printf("Pass 1 failed!\n");
        symbol_table_destroy(&symbols);
        fclose(source);
        return 1;
    }

    printf("===== Pass 1 Test =====\n");

    printf(
        "Text Base : 0x%08X\n",
        layout.text_base
    );

    printf(
        "Text Size : %u bytes\n",
        layout.text_size
    );

    printf(
        "Data Base : 0x%08X\n",
        layout.data_base
    );

    printf(
        "Data Size : %u bytes\n",
        layout.data_size
    );

    printf("\n===== Symbol Table =====\n");

    for (size_t i = 0; i < symbols.count; i++) {
        printf(
            "%-10s = 0x%08X\n",
            symbols.symbols[i].name,
            symbols.symbols[i].address
        );
    }

    symbol_table_destroy(&symbols);
    fclose(source);

    return 0;
}
