#include <stdio.h>
#include "parser.h"

typedef struct {
    const char *source;
    int expected;
} TestCase;

int main(void)
{
    TestCase tests[] = {
        {"_start: nop", 1},
        {"loop: nop", 1},
        {".L1: nop", 1},
        {"abc123: nop", 1},
        {"a_b.c9: nop", 1},
        {"ABC: nop", 1},

        {"9loop: nop", 0},
        {"bad@name: nop", 0},
        {"loop-name: nop", 0},
        {"abc$def: nop", 0},
        {"1_start: nop", 0}
    };

    int count = sizeof(tests) / sizeof(tests[0]);
    int passed = 0;

    for (int i = 0; i < count; i++) {
        ParsedLine parsed;
        int result = parse_line(tests[i].source, &parsed);

        int success = (result == 0) == tests[i].expected;

        printf("[%s] %s\n",
               success ? "PASS" : "FAIL",
               tests[i].source);

        if (success)
            passed++;
    }

    printf("\n%d / %d tests passed\n", passed, count);

    return passed == count ? 0 : 1;
}