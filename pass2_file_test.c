
#include <stdio.h>
#include <stdint.h>

#include "pass1.h"
#include "pass2.h"
#include "symbol_table.h"

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf(
            "Usage: ./pass2_file_test "
            "<source_file> <binary_file> <listing_file>\n"
        );
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
    printf("BSS Base  : 0x%08X\n", layout.bss_base);
    printf("BSS Size  : %u bytes\n", layout.bss_size);

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

        uint32_t data_start_offset = layout.text_size;

    if (layout.data_size != 0) {
        if (layout.data_base < layout.text_base) {
            printf("Invalid data base address!\n");
            goto cleanup;
        }

        data_start_offset = layout.data_base - layout.text_base;
    }

    uint32_t expected_size =
        data_start_offset + layout.data_size;

    printf("Generated Size : %u bytes\n", result.size);
    printf("Expected Size  : %u bytes\n", expected_size);

    if (result.size != expected_size) {
        printf("Binary size mismatch!\n");
        goto cleanup;
    }
    /* Write binary file */
    if (pass2_write_binary(&result, argv[2]) != 0) {
        printf("\nFailed to write binary file!\n");
        goto cleanup;
    }

    if (pass2_write_listing(
            &result,
            &symbols,
            argv[3]
        ) != 0) {
        printf("\nFailed to write listing file!\n");
        goto cleanup;
    }

    printf("\nBinary File  : %s\n", argv[2]);
    printf("Listing File : %s\n", argv[3]);
    printf("Assembly SUCCESS!\n");

    exit_code = 0;

cleanup:
    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);

    return exit_code;
}
