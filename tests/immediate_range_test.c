#include <stdio.h>
#include <string.h>

#include "pass1.h"
#include "pass2.h"
#include "symbol_table.h"

typedef struct {
    const char *name;
    const char *source_text;
    int should_assemble;
} TestCase;

static int run_case(const TestCase *test)
{
    FILE *source = tmpfile();
    if (source == NULL) {
        perror("tmpfile");
        return -1;
    }

    if (fputs(test->source_text, source) == EOF || fflush(source) != 0) {
        fclose(source);
        return -1;
    }

    SymbolTable symbols;
    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return -1;
    }

    Pass1Layout layout;
    Pass2Result result = {0};
    int assembled = 0;

    if (pass1_run(source, &symbols, &layout) == 0 &&
        pass2_run(source, &symbols, &layout, &result) == 0) {
        assembled = 1;
    }

    int passed = assembled == test->should_assemble;
    printf("[%s] %s\n", passed ? "PASS" : "FAIL", test->name);

    pass2_destroy(&result);
    symbol_table_destroy(&symbols);
    fclose(source);
    return passed ? 0 : -1;
}

int main(void)
{
    static const TestCase tests[] = {
        {"addi accepts -1", ".text\naddi t0, t0, -1\n", 1},
        {"addi accepts (-1)", ".text\naddi t0, t0, (-1)\n", 1},
        {"addi accepts 0-1", ".text\naddi t0, t0, 0-1\n", 1},
        {"addi accepts 2047", ".text\naddi t0, t0, 2047\n", 1},
        {"addi accepts -2048", ".text\naddi t0, t0, -2048\n", 1},
        {"addi rejects 0xFFFFFFFF", ".text\naddi t0, t0, 0xFFFFFFFF\n", 0},
        {"addi rejects positive .equ 0xFFFFFFFF",
         ".equ POS, 0xFFFFFFFF\n.text\naddi t0, t0, POS\n", 0},
        {"addi accepts negative .equ",
         ".equ NEG, -1\n.text\naddi t0, t0, NEG\n", 1},
        {"li preserves unsigned 32-bit constant",
         ".text\nli t0, 0xFFFFFFFF\n", 1},
        {"li preserves unsigned 32-bit .equ",
         ".equ POS, 0xFFFFFFFF\n.text\nli t0, POS\n", 1},
        {"li preserves negative .equ",
         ".equ NEG, -1\n.text\nli t0, NEG\n", 1},
        {".word preserves unsigned 32-bit constant",
         ".data\n.word 0xFFFFFFFF\n", 1},
        {".byte and .half preserve negative .equ",
         ".equ NEG, -1\n.data\n.byte NEG, 0\n.half NEG\n", 1}
    };

    int failed = 0;
    size_t count = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < count; i++) {
        if (run_case(&tests[i]) != 0) {
            failed++;
        }
    }

    printf("Immediate range tests: %zu passed, %d failed\n",
           count - (size_t)failed, failed);
    return failed == 0 ? 0 : 1;
}
