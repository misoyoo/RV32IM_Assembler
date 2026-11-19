
#include <stdio.h>
#include <string.h>
#include "encoder.h"

#define R_OPCODE 0x33

// ========================================
// R-type
// ========================================

typedef struct {
    const char *name;
    int funct3;
    int funct7;
} RInstruction;

static const RInstruction r_table[] = {
    {"add",    0b000, 0b0000000},
    {"sub",    0b000, 0b0100000},
    {"sll",    0b001, 0b0000000},
    {"slt",    0b010, 0b0000000},
    {"sltu",   0b011, 0b0000000},
    {"xor",    0b100, 0b0000000},
    {"srl",    0b101, 0b0000000},
    {"sra",    0b101, 0b0100000},
    {"or",     0b110, 0b0000000},
    {"and",    0b111, 0b0000000},

    {"mul",    0b000, 0b0000001},
    {"mulh",   0b001, 0b0000001},
    {"mulhsu", 0b010, 0b0000001},
    {"mulhu",  0b011, 0b0000001},
    {"div",    0b100, 0b0000001},
    {"divu",   0b101, 0b0000001},
    {"rem",    0b110, 0b0000001},
    {"remu",   0b111, 0b0000001}
};

int encode_r_type(
    const char *mnemonic,
    int rd,
    int rs1,
    int rs2,
    uint32_t *instruction
)
{
    if (mnemonic == NULL || instruction == NULL) {
        return -1;
    }

    if (rd < 0 || rd > 31 ||
        rs1 < 0 || rs1 > 31 ||
        rs2 < 0 || rs2 > 31) {
        return -1;
    }

    int funct3 = -1;
    int funct7 = -1;

    int count = sizeof(r_table) / sizeof(r_table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(mnemonic, r_table[i].name) == 0) {
            funct3 = r_table[i].funct3;
            funct7 = r_table[i].funct7;
            break;
        }
    }

    if (funct3 == -1) {
        return -1;
    }

    *instruction =
        ((uint32_t)funct7 << 25) |
        ((uint32_t)rs2 << 20) |
        ((uint32_t)rs1 << 15) |
        ((uint32_t)funct3 << 12) |
        ((uint32_t)rd << 7) |
        R_OPCODE;

    return 0;
}


// ========================================
// I-type
// ========================================

typedef struct {
    const char *name;
    int opcode;
    int funct3;
} IInstruction;

static const IInstruction i_table[] = {
    {"addi",  0x13, 0b000},
    {"slti",  0x13, 0b010},
    {"sltiu", 0x13, 0b011},
    {"xori",  0x13, 0b100},
    {"ori",   0x13, 0b110},
    {"andi",  0x13, 0b111},

    {"slli",  0x13, 0b001},
    {"srli",  0x13, 0b101},
    {"srai",  0x13, 0b101},

    {"lb",    0x03, 0b000},
    {"lh",    0x03, 0b001},
    {"lw",    0x03, 0b010},
    {"lbu",   0x03, 0b100},
    {"lhu",   0x03, 0b101},

    {"jalr",  0x67, 0b000}
};

int encode_i_type(
    const char *mnemonic,
    int rd,
    int rs1,
    int imm,
    uint32_t *instruction
)
{
    if (mnemonic == NULL || instruction == NULL) {
        return -1;
    }

    if (rd < 0 || rd > 31 ||
        rs1 < 0 || rs1 > 31) {
        return -1;
    }

    int opcode = -1;
    int funct3 = -1;

    int count = sizeof(i_table) / sizeof(i_table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(mnemonic, i_table[i].name) == 0) {
            opcode = i_table[i].opcode;
            funct3 = i_table[i].funct3;
            break;
        }
    }

    if (opcode == -1) {
        return -1;
    }

    // Shift instructions
    if (strcmp(mnemonic, "slli") == 0 ||
        strcmp(mnemonic, "srli") == 0 ||
        strcmp(mnemonic, "srai") == 0) {

        if (imm < 0 || imm > 31) {
            return -1;
        }

        int upper = 0;

        if (strcmp(mnemonic, "srai") == 0) {
            upper = 0b0100000;
        }

        uint32_t encoded_imm =
            ((uint32_t)upper << 5) | (uint32_t)imm;

        *instruction =
            (encoded_imm << 20) |
            ((uint32_t)rs1 << 15) |
            ((uint32_t)funct3 << 12) |
            ((uint32_t)rd << 7) |
            (uint32_t)opcode;

        return 0;
    }

    // Immediate range check
    if (imm < -2048 || imm > 2047) {
        return -1;
    }

    uint32_t encoded_imm = (uint32_t)imm & 0xFFF;

    *instruction =
        (encoded_imm << 20) |
        ((uint32_t)rs1 << 15) |
        ((uint32_t)funct3 << 12) |
        ((uint32_t)rd << 7) |
        (uint32_t)opcode;

    return 0;
}


