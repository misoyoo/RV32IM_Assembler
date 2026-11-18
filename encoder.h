#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

int encode_r_type(
    const char *mnemonic,
    int rd,
    int rs1,
    int rs2,
    uint32_t *instruction
);

#endif