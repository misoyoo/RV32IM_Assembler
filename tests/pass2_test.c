
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
        ".global _start\n"
        "_start:\n"
        "    addi t0, zero, 10\n"
        "    addi t1, t0, -1\n"
        "    add t2, t0, t1\n"
        "    sw t2, 0(sp)\n"
        "    lw a0, 0(sp)\n"
        "    beq a0, zero, done\n"
        "    addi a1, zero, 1\n"
        "done:\n"
        "    lui a2, 0x12345\n"
        "    jal x0, done\n"
        "    mret\n",
        source
    );

    fflush(source);

    SymbolTable symbols;
    Pass1Layout layout;
    Pass2Result result = {0};

    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return 1;
    }

    if (pass1_run(source, &symbols, &layout) != 0) {
        printf("Pass 1 FAILED!\n");

        symbol_table_destroy(&symbols);
        fclose(source);

        return 1;
    }

    printf("Pass 1 SUCCESS!\n");

    if (pass2_run(
            source,
            &symbols,
            &layout,
            &result
        ) != 0) {

        printf("Pass 2 FAILED!\n");

        symbol_table_destroy(&symbols);
        fclose(source);

        return 1;
    }

    printf("Pass 2 SUCCESS!\n\n");

    static const uint32_t expected[] = {
        0x00A00293,
        0xFFF28313,
        0x006283B3,
        0x00712023,
        0x00012503,
        0x00050463,
        0x00100593,
        0x12345637,
        0xFFDFF06F,
        0x30200073
    };

    size_t instruction_count =
        sizeof(expected) / sizeof(expected[0]);

    int passed = 0;

    printf("===== Machine Code Validation =====\n");

    printf("Generated size: %u bytes\n\n", result.size);

    if (result.size != instruction_count * 4) {
        printf("Unexpected binary size!\n");
        goto cleanup;
    }

    for (size_t i = 0; i < instruction_count; i++) {
        uint32_t actual =
            (uint32_t)result.bytes[i * 4] |
            ((uint32_t)result.bytes[i * 4 + 1] << 8) |
            ((uint32_t)result.bytes[i * 4 + 2] << 16) |
            ((uint32_t)result.bytes[i * 4 + 3] << 24);

        printf(
            "[%s] Address: 0x%08X | Expected: 0x%08X | Actual: 0x%08X\n",
            actual == expected[i] ? "PASS" : "FAIL",
            layout.text_base + (uint32_t)(i * 4),
            expected[i],
            actual
        );

        if (actual == expected[i]) {
            passed++;
        }
    }

    printf("\n===== Test Summary =====\n");
    printf("Passed: %d / %zu\n", passed, instruction_count);
    printf("Failed: %zu / %zu\n",
           instruction_count - (size_t)passed,
           instruction_count);

cleanup:
    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);

    return passed == (int)instruction_count ? 0 : 1;
}
