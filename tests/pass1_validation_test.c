
#include <stdio.h>
#include <string.h>

#include "pass1.h"
#include "symbol_table.h"

typedef struct {
    const char *name;
    const char *source;
    int should_pass;
} TestCase;

static int run_test(const TestCase *test)
{
    FILE *source = tmpfile();

    if (source == NULL) {
        perror("tmpfile");
        return -1;
    }

    fputs(test->source, source);
    rewind(source);

    SymbolTable symbols;
    Pass1Layout layout;

    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return -1;
    }

    int result = pass1_run(source, &symbols, &layout);

    int actual_pass = (result == 0);
    int test_pass = (actual_pass == test->should_pass);

    printf(
        "[%s] %s\n",
        test_pass ? "PASS" : "FAIL",
        test->name
    );

    symbol_table_destroy(&symbols);
    fclose(source);

    return test_pass ? 0 : -1;
}

int main(void)
{
    TestCase tests[] = {
        {
            "Valid assembly",
            ".text\n"
            "_start:\n"
            "    addi t0, zero, 1\n"
            ".data\n"
            "value: .word 1\n",
            1
        },
        {
            "Unknown instruction",
            ".text\n"
            "_start:\n"
            "    wrongop t0, t1\n",
            0
        },
        {
            "Undefined symbol",
            ".equ VALUE, UNKNOWN\n"
            ".text\n"
            "_start:\n"
            "    nop\n",
            0
        },
        {
            "Duplicate label",
            ".text\n"
            "_start:\n"
            "    nop\n"
            "_start:\n"
            "    nop\n",
            0
        },
        {
            "Missing .word operand",
            ".text\n"
            "_start:\n"
            "    nop\n"
            ".data\n"
            "value: .word\n",
            0
        },
        {
            "Invalid alignment",
            ".text\n"
            ".align 32\n"
            "_start:\n"
            "    nop\n",
            0
        },
        {
            "Implicit default text section",
            "    nop\n",
            1
        }
    };

    int test_count = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;

    printf("===== Pass 1 Validation Tests =====\n\n");

    for (int i = 0; i < test_count; i++) {
        if (run_test(&tests[i]) == 0) {
            passed++;
        }
    }

    printf("\n===== Test Summary =====\n");
    printf("Passed: %d / %d\n", passed, test_count);
    printf("Failed: %d / %d\n", test_count - passed, test_count);

    return passed == test_count ? 0 : 1;
}
