
#include <stdio.h>
#include <stdint.h>

#include "pass1.h"
#include "pass2.h"
#include "symbol_table.h"

int main(int argc, char *argv[])
{
    /* Check command-line arguments */

    if (argc != 4) {
        printf(
            "Usage: ./assembler "
            "<source_file> <binary_file> <listing_file>\n"
        );

        return 1;
    }

    /* Open assembly source file */

    FILE *source = fopen(argv[1], "r");

    if (source == NULL) {
        perror("Failed to open source file");
        return 1;
    }

    SymbolTable symbols;
    Pass1Layout layout = {0};
    Pass2Result result = {0};

    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return 1;
    }

    int exit_code = 1;

    printf("========================================\n");
    printf("       RV32IM TWO-PASS ASSEMBLER\n");
    printf("========================================\n\n");

    printf("Source File : %s\n", argv[1]);
    printf("Binary File : %s\n", argv[2]);
    printf("Listing File: %s\n\n", argv[3]);

    /* ========================================
       Pass 1: Address Assignment
       ======================================== */

    printf("[Pass 1] Address Assignment...\n");

    if (pass1_run(
            source,
            &symbols,
            &layout
        ) != 0) {

        printf("Pass 1 FAILED!\n");
        goto cleanup;
    }

    printf("Pass 1 SUCCESS!\n\n");

    /* Display section layout */

    printf("===== Section Layout =====\n");

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

    printf(
        "BSS Base  : 0x%08X\n",
        layout.bss_base
    );

    printf(
        "BSS Size  : %u bytes\n",
        layout.bss_size
    );

    /* ========================================
       Pass 2: Machine Code Generation
       ======================================== */

    printf("\n[Pass 2] Machine Code Generation...\n");

    if (pass2_run(
            source,
            &symbols,
            &layout,
            &result
        ) != 0) {

        printf("Pass 2 FAILED!\n");
        goto cleanup;
    }

    printf("Pass 2 SUCCESS!\n");

    /* ========================================
       Validate Binary Size
       ======================================== */

    uint32_t data_start_offset = layout.text_size;

    if (layout.data_size != 0) {
        if (layout.data_base < layout.text_base) {
            printf("Invalid data base address!\n");
            goto cleanup;
        }

        data_start_offset =
            layout.data_base - layout.text_base;
    }

    if (layout.data_size >
        UINT32_MAX - data_start_offset) {

        printf("Binary size overflow!\n");
        goto cleanup;
    }

    uint32_t expected_size =
        data_start_offset + layout.data_size;

    printf("\n===== Binary Validation =====\n");

    printf(
        "Generated Size : %u bytes\n",
        result.size
    );

    printf(
        "Expected Size  : %u bytes\n",
        expected_size
    );

    if (result.size != expected_size) {
        printf("Binary size mismatch!\n");
        goto cleanup;
    }

    printf("Binary Validation SUCCESS!\n");

    /* ========================================
       Generate Listing File
       ======================================== */

    if (pass2_write_listing(
            &result,
            &symbols,
            argv[3]
        ) != 0) {

        printf("\nFailed to write listing file!\n");
        goto cleanup;
    }

    /* ========================================
       Generate Binary File
       ======================================== */

    if (pass2_write_binary(
            &result,
            argv[2]
        ) != 0) {

        printf("\nFailed to write binary file!\n");
        goto cleanup;
    }

    printf("\n===== Assembly Results =====\n");

    printf("Binary File  : %s\n", argv[2]);
    printf("Listing File : %s\n", argv[3]);

    printf("\n========================================\n");
    printf("          ASSEMBLY SUCCESS!\n");
    printf("========================================\n");

    exit_code = 0;

cleanup:

    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);

    return exit_code;
}
