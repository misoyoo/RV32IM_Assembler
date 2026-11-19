
#include <stdio.h>
#include <stdint.h>

#include "pass1.h"
#include "symbol_table.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: ./pass1_file_test <source_file>\n");
        return 1;
    }

    FILE *source = fopen(argv[1], "r");

    if (source == NULL) {
        perror("Failed to open source file");
        return 1;
    }

    SymbolTable symbols;
    Pass1Layout layout;

    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return 1;
    }

    printf("===== Pass 1: %s =====\n\n", argv[1]);

    if (pass1_run(source, &symbols, &layout) != 0) {
        printf("\nPass 1 FAILED!\n");

        symbol_table_destroy(&symbols);
        fclose(source);

        return 1;
    }

    printf("===== Section Layout =====\n");
    printf("Text Base : 0x%08X\n", layout.text_base);
    printf("Text Size : %u bytes\n", layout.text_size);
    printf("Data Base : 0x%08X\n", layout.data_base);
    printf("Data Size : %u bytes\n", layout.data_size);
    printf("BSS Base  : 0x%08X\n", layout.bss_base);
    printf("BSS Size  : %u bytes\n", layout.bss_size);

    printf("\n===== Symbol Table =====\n");

    for (size_t i = 0; i < symbols.count; i++) {
        printf(
            "%-20s = 0x%08X | Type: %c | Global: %-3s | Defined: %s\n",
            symbols.symbols[i].name,
            symbols.symbols[i].address,
            symbols.symbols[i].type,
            symbols.symbols[i].is_global ? "Yes" : "No",
            symbols.symbols[i].defined ? "Yes" : "No"
        );
    }

    printf("\nPass 1 SUCCESS!\n");

    symbol_table_destroy(&symbols);
    fclose(source);

    return 0;
}
