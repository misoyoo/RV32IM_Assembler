
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
    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    // ABI register name lookup
    for (int i = 0; i < 32; i++) {
        if (strcmp(name, register_names[i]) == 0) {
            return i;
        }
    }

    // fp is an alias for s0
    if (strcmp(name, "fp") == 0) {
        return 8;
    }

    // x0 ~ x31 format
    if (name[0] != 'x' ||
        name[1] == '\0') {
        return -1;
    }

    // Reject leading zeros except for x0
    if (name[1] == '0' && name[2] != '\0') {
        return -1;
    }

    int num = 0;

    for (const char *p = name + 1; *p != '\0'; p++) {
        // Only decimal digits are allowed after 'x'
        if (*p < '0' || *p > '9') {
            return -1;
        }

        num = num * 10 + (*p - '0');

        if (num > 31) {
            return -1;
        }
    }

    return num;
}
