
#include <stdio.h>
#include <stdint.h>

#include "pass1.h"
#include "pass2.h"
#include "symbol_table.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: ./pass2_file_test <source_file> <output_file>\n");
        return 1;
    }

    FILE *source = fopen(argv[1], "r");

    if (source == NULL) {
        perror("Failed to open source file");
        return 1;
    }

    SymbolTable symbols;
    Pass1Layout layout;
    Pass2Result result = {0};

    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return 1;
    }

    int exit_code = 1;

    printf("===== Pass 2: %s =====\n\n", argv[1]);

    /* Pass 1 */

    if (pass1_run(source, &symbols, &layout) != 0) {
        printf("Pass 1 FAILED!\n");
        goto cleanup;
    }

    printf("===== Section Layout =====\n");
    printf("Text Base : 0x%08X\n", layout.text_base);
    printf("Text Size : %u bytes\n", layout.text_size);
    printf("Data Base : 0x%08X\n", layout.data_base);
    printf("Data Size : %u bytes\n", layout.data_size);

    printf("\nPass 1 SUCCESS!\n");

    /* Pass 2 */

    if (pass2_run(
            source,
            &symbols,
            &layout,
            &result
        ) != 0) {

        printf("\nPass 2 FAILED!\n");
        goto cleanup;
    }

    printf("\nPass 2 SUCCESS!\n");

    printf("Generated Size : %u bytes\n", result.size);
    printf("Expected Size  : %u bytes\n",
           layout.text_size + layout.data_size);

    /* Write binary file */

    if (pass2_write_binary(&result, argv[2]) != 0) {
        printf("\nFailed to write binary file!\n");
        goto cleanup;
    }

    printf("\nBinary File: %s\n", argv[2]);
    printf("Assembly SUCCESS!\n");

    exit_code = 0;

cleanup:
    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);

    return exit_code;
}
