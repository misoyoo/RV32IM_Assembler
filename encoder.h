
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

// S-type
int encode_s_type(
    const char *mnemonic,
    int rs1,
    int rs2,
    int imm,
    uint32_t *instruction
);

// B-type
int encode_b_type(
    const char *mnemonic,
    int rs1,
    int rs2,
    int imm,
    uint32_t *instruction
);

// U-type
int encode_u_type(
    const char *mnemonic,
    int rd,
    int imm,
    uint32_t *instruction
);

// J-type
int encode_j_type(
    const char *mnemonic,
    int rd,
    int imm,
    uint32_t *instruction
);

// CSR-type
int encode_csr_type(
    const char *mnemonic,
    int rd,
    const char *csr_name,
    int rs1,
    uint32_t *instruction
);

// System-type
int encode_system_type(
    const char *mnemonic,
    uint32_t *instruction
);

#endif
