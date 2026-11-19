
#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

// R-type
int encode_r_type(
    const char *mnemonic,
    int rd,
    int rs1,
    int rs2,
    uint32_t *instruction
);

// I-type
int encode_i_type(
    const char *mnemonic,
    int rd,
    int rs1,
    int imm,
    uint32_t *instruction
);

#endif
