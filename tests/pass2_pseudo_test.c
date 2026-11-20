
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
        "    li t0, 10\n"
        "    li t1, 50000000\n"
        "    la s0, target\n"
        "    call target\n"
        "    j done\n"
        "target:\n"
        "    mv a0, t0\n"
        "done:\n"
        "    ret\n",
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
        goto cleanup;
    }

    printf("Pass 1 SUCCESS!\n");

    if (pass2_run(source, &symbols, &layout, &result) != 0) {
        printf("Pass 2 FAILED!\n");
        goto cleanup;
    }

    printf("Pass 2 SUCCESS!\n\n");

    static const uint32_t expected[] = {
        0x00A00293,
        0x02FAF337,
        0x08030313,
        0x00000417,
        0x01440413,
        0x00000097,
        0x00C080E7,
        0x0080006F,
        0x00028513,
        0x00008067
    };

    size_t count = sizeof(expected) / sizeof(expected[0]);
    int passed = 0;

    printf("===== Pseudo-instruction Validation =====\n");
    printf("Generated size: %u bytes\n\n", result.size);

    if (result.size != count * 4) {
        printf("Unexpected binary size!\n");
        goto cleanup;
    }

    for (size_t i = 0; i < count; i++) {
        uint32_t actual =
            (uint32_t)result.bytes[i * 4] |
            ((uint32_t)result.bytes[i * 4 + 1] << 8) |
            ((uint32_t)result.bytes[i * 4 + 2] << 16) |
            ((uint32_t)result.bytes[i * 4 + 3] << 24);

        printf(
            "[%s] Expected: 0x%08X | Actual: 0x%08X\n",
            actual == expected[i] ? "PASS" : "FAIL",
            expected[i],
            actual
        );

        if (actual == expected[i]) {
            passed++;
        }
    }

    printf("\nPassed: %d / %zu\n", passed, count);
    printf("Failed: %zu / %zu\n",
           count - (size_t)passed,
           count);

    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);

    return passed == (int)count ? 0 : 1;

cleanup:
    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);

    return 1;
}
