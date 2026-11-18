
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "register.h"

static const char *register_names[32] = {
    "zero", "ra", "sp", "gp", "tp",
    "t0", "t1", "t2",
    "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6"
};

int get_register_number(const char *name)
{
    // ABI 이름 검색
    for (int i = 0; i < 32; i++) {
        if (strcmp(name, register_names[i]) == 0) {
            return i;
        }
    }

    // fp는 s0와 같은 레지스터
    if (strcmp(name, "fp") == 0) {
        return 8;
    }

    // x0 ~ x31 형식 처리
    if (name[0] == 'x') {
        char *end;
        long num = strtol(name + 1, &end, 10);

        if (*end == '\0' && end != name + 1 &&
            num >= 0 && num <= 31) {
            return (int)num;
        }
    }

    return -1;
}
