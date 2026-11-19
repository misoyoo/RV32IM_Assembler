
#include <stdio.h>
#include <stdint.h>

#include "pass1.h"
#include "pass2.h"
#include "symbol_table.h"

int main(void)
{
    FILE *source = tmpfile();

    if (source == NULL) {
        perror("tmpfile");
        return 1;
    }

    fputs(
        ".text\n"
        "_start:\n"
        "    addi zero, zero, 0\n"
        "\n"
        ".data\n"
        "byte_values: .byte 0x12, 0x34\n"
        "half_value: .half 0x1234\n"
        "word_value: .word 0x12345678\n"
        "ascii_value: .ascii \"A\"\n"
        "asciz_value: .asciz \"BC\"\n"
        "space_value: .space 3, 0xAA\n"
        "    .align 2\n"
        "aligned_value: .byte 0x55\n",
        source
    );

    fflush(source);

    SymbolTable symbols;
    Pass1Layout layout;
    Pass2Result result = {0};

    int exit_code = 1;
    int passed = 0;

    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return 1;
    }

    if (pass1_run(source, &symbols, &layout) != 0) {
        printf("Pass 1 FAILED!\n");
        goto cleanup;
    }

    printf("Pass 1 SUCCESS!\n");

    printf("Text Size: %u bytes\n", layout.text_size);
    printf("Data Base: 0x%08X\n", layout.data_base);
    printf("Data Size: %u bytes\n\n", layout.data_size);

    if (pass2_run(
            source,
            &symbols,
            &layout,
            &result
        ) != 0) {

        printf("Pass 2 FAILED!\n");
        goto cleanup;
    }

    printf("Pass 2 SUCCESS!\n\n");

    static const uint8_t expected[] = {
        /* .text */
        0x13, 0x00, 0x00, 0x00,

        /* .byte */
        0x12, 0x34,

        /* .half */
        0x34, 0x12,

        /* .word */
        0x78, 0x56, 0x34, 0x12,

        /* .ascii "A" */
        0x41,

        /* .asciz "BC" */
        0x42, 0x43, 0x00,

        /* .space 3, 0xAA */
        0xAA, 0xAA, 0xAA,

        /* .align 2 */
        0x00,

        /* .byte 0x55 */
        0x55
    };

    size_t expected_size =
        sizeof(expected) / sizeof(expected[0]);

    printf("===== Data Section Validation =====\n");

    printf("Generated size: %u bytes\n", result.size);
    printf("Expected size: %zu bytes\n\n", expected_size);

    if (layout.text_size != 4 ||
        layout.data_size != 17 ||
        result.size != expected_size) {

        printf("[FAIL] Unexpected section size!\n");
        goto cleanup;
    }

    for (size_t i = 0; i < expected_size; i++) {
        uint8_t actual = result.bytes[i];

        printf(
            "[%s] Address: 0x%08X | Expected: 0x%02X | Actual: 0x%02X\n",
            actual == expected[i] ? "PASS" : "FAIL",
            layout.text_base + (uint32_t)i,
            expected[i],
            actual
        );

        if (actual == expected[i]) {
            passed++;
        }
    }

    printf("\n===== Test Summary =====\n");
    printf("Passed: %d / %zu\n", passed, expected_size);
    printf("Failed: %zu / %zu\n",
           expected_size - (size_t)passed,
           expected_size);

    exit_code = passed == (int)expected_size ? 0 : 1;

cleanup:
    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);

    return exit_code;
}
