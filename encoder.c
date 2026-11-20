
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

// ========================================
// S-type
// ========================================

typedef struct {
    const char *name;
    int funct3;
} SInstruction;

static const SInstruction s_table[] = {
    {"sb", 0b000},
    {"sh", 0b001},
    {"sw", 0b010}
};

int encode_s_type(
    const char *mnemonic,
    int rs1,
    int rs2,
    int imm,
    uint32_t *instruction
)
{
    if (mnemonic == NULL || instruction == NULL) {
        return -1;
    }

    if (rs1 < 0 || rs1 > 31 ||
        rs2 < 0 || rs2 > 31) {
        return -1;
    }

    int funct3 = -1;

    int count = sizeof(s_table) / sizeof(s_table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(mnemonic, s_table[i].name) == 0) {
            funct3 = s_table[i].funct3;
            break;
        }
    }

    if (funct3 == -1) {
        return -1;
    }

    // Immediate range check
    if (imm < -2048 || imm > 2047) {
        return -1;
    }

    // Split immediate into two parts
    uint32_t encoded_imm = (uint32_t)imm & 0xFFF;

    uint32_t imm_11_5 = (encoded_imm >> 5) & 0x7F;
    uint32_t imm_4_0  = encoded_imm & 0x1F;

    *instruction =
        (imm_11_5 << 25) |
        ((uint32_t)rs2 << 20) |
        ((uint32_t)rs1 << 15) |
        ((uint32_t)funct3 << 12) |
        (imm_4_0 << 7) |
        0x23;

    return 0;
}

// ========================================
// B-type
// ========================================

typedef struct {
    const char *name;
    int funct3;
} BInstruction;

static const BInstruction b_table[] = {
    {"beq",  0b000},
    {"bne",  0b001},
    {"blt",  0b100},
    {"bge",  0b101},
    {"bltu", 0b110},
    {"bgeu", 0b111}
};

int encode_b_type(
    const char *mnemonic,
    int rs1,
    int rs2,
    int imm,
    uint32_t *instruction
)
{
    if (mnemonic == NULL || instruction == NULL) {
        return -1;
    }

    if (rs1 < 0 || rs1 > 31 ||
        rs2 < 0 || rs2 > 31) {
        return -1;
    }

    int funct3 = -1;

    int count = sizeof(b_table) / sizeof(b_table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(mnemonic, b_table[i].name) == 0) {
            funct3 = b_table[i].funct3;
            break;
        }
    }

    if (funct3 == -1) {
        return -1;
    }

    // Immediate range and alignment check
    if (imm < -4096 || imm > 4094 || imm % 2 != 0) {
        return -1;
    }

    uint32_t encoded_imm = (uint32_t)imm & 0x1FFF;

    uint32_t imm_12   = (encoded_imm >> 12) & 0x1;
    uint32_t imm_10_5 = (encoded_imm >> 5)  & 0x3F;
    uint32_t imm_4_1  = (encoded_imm >> 1)  & 0xF;
    uint32_t imm_11   = (encoded_imm >> 11) & 0x1;

    *instruction =
        (imm_12 << 31) |
        (imm_10_5 << 25) |
        ((uint32_t)rs2 << 20) |
        ((uint32_t)rs1 << 15) |
        ((uint32_t)funct3 << 12) |
        (imm_4_1 << 8) |
        (imm_11 << 7) |
        0x63;

    return 0;
}

// ========================================
// U-type
// ========================================

typedef struct {
    const char *name;
    int opcode;
} UInstruction;

static const UInstruction u_table[] = {
    {"lui",   0x37},
    {"auipc", 0x17}
};

int encode_u_type(
    const char *mnemonic,
    int rd,
    int imm,
    uint32_t *instruction
)
{
    if (mnemonic == NULL || instruction == NULL) {
        return -1;
    }

    if (rd < 0 || rd > 31) {
        return -1;
    }

    int opcode = -1;

    int count = sizeof(u_table) / sizeof(u_table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(mnemonic, u_table[i].name) == 0) {
            opcode = u_table[i].opcode;
            break;
        }
    }

    if (opcode == -1) {
        return -1;
    }

    // Immediate range check (20-bit)
    if (imm < -524288 || imm > 1048575) {
        return -1;
    }

    uint32_t encoded_imm = (uint32_t)imm & 0xFFFFF;

    *instruction =
        (encoded_imm << 12) |
        ((uint32_t)rd << 7) |
        (uint32_t)opcode;

    return 0;
}

// ========================================
// J-type
// ========================================

int encode_j_type(
    const char *mnemonic,
    int rd,
    int imm,
    uint32_t *instruction
)
{
    if (mnemonic == NULL || instruction == NULL) {
        return -1;
    }

    if (rd < 0 || rd > 31) {
        return -1;
    }

    if (strcmp(mnemonic, "jal") != 0) {
        return -1;
    }

    // Immediate range and alignment check
    if (imm < -1048576 || imm > 1048574 || imm % 2 != 0) {
        return -1;
    }

    uint32_t encoded_imm = (uint32_t)imm & 0x1FFFFF;

    uint32_t imm_20    = (encoded_imm >> 20) & 0x1;
    uint32_t imm_10_1  = (encoded_imm >> 1)  & 0x3FF;
    uint32_t imm_11    = (encoded_imm >> 11) & 0x1;
    uint32_t imm_19_12 = (encoded_imm >> 12) & 0xFF;

    *instruction =
        (imm_20 << 31) |
        (imm_10_1 << 21) |
        (imm_11 << 20) |
        (imm_19_12 << 12) |
        ((uint32_t)rd << 7) |
        0x6F;

    return 0;
}

// ========================================
// CSR-type
// ========================================

typedef struct {
    const char *name;
    int address;
} CSR;

static const CSR csr_table[] = {
    {"mstatus",    0x300},
    {"misa",       0x301},
    {"medeleg",    0x302},
    {"mideleg",    0x303},
    {"mie",        0x304},
    {"mtvec",      0x305},
    {"mcounteren", 0x306},
    {"mscratch",   0x340},
    {"mepc",       0x341},
    {"mcause",     0x342},
    {"mtval",      0x343},
    {"mip",        0x344},
    {"cycle",      0xC00},
    {"time",       0xC01},
    {"instret",    0xC02}
};

static int get_csr_address(const char *name)
{
    if (name == NULL) {
        return -1;
    }

    int count = sizeof(csr_table) / sizeof(csr_table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(name, csr_table[i].name) == 0) {
            return csr_table[i].address;
        }
    }

    return -1;
}

int encode_csr_type(
    const char *mnemonic,
    int rd,
    const char *csr_name,
    int rs1,
    uint32_t *instruction
)
{
    if (mnemonic == NULL || csr_name == NULL || instruction == NULL) {
        return -1;
    }

    int csr = get_csr_address(csr_name);

    if (csr < 0) {
        return -1;
    }

    if (rd < 0 || rd > 31) {
        return -1;
    }

    int funct3;

    if (strcmp(mnemonic, "csrrw") == 0 ||
        strcmp(mnemonic, "csrw") == 0) {
        funct3 = 0b001;
    }
    else if (strcmp(mnemonic, "csrrs") == 0 ||
             strcmp(mnemonic, "csrs") == 0 ||
             strcmp(mnemonic, "csrr") == 0) {
        funct3 = 0b010;
    }
    else if (strcmp(mnemonic, "csrrc") == 0 ||
             strcmp(mnemonic, "csrc") == 0) {
        funct3 = 0b011;
    }
    else if (strcmp(mnemonic, "csrrwi") == 0 ||
             strcmp(mnemonic, "csrwi") == 0) {
        funct3 = 0b101;
    }
    else if (strcmp(mnemonic, "csrrsi") == 0 ||
             strcmp(mnemonic, "csrsi") == 0) {
        funct3 = 0b110;
    }
    else if (strcmp(mnemonic, "csrrci") == 0 ||
             strcmp(mnemonic, "csrci") == 0) {
        funct3 = 0b111;
    }
    else {
        return -1;
    }

    // Pseudo-instruction operand handling
    if (strcmp(mnemonic, "csrw") == 0 ||
        strcmp(mnemonic, "csrs") == 0 ||
        strcmp(mnemonic, "csrc") == 0 ||
        strcmp(mnemonic, "csrwi") == 0 ||
        strcmp(mnemonic, "csrsi") == 0 ||
        strcmp(mnemonic, "csrci") == 0) {
        rd = 0;
    }

    if (strcmp(mnemonic, "csrr") == 0) {
        rs1 = 0;
    }

    // Immediate CSR instructions use a 5-bit zimm
    if (funct3 >= 0b101) {
        if (rs1 < 0 || rs1 > 31) {
            return -1;
        }
    }
    else {
        if (rs1 < 0 || rs1 > 31) {
            return -1;
        }
    }

    *instruction =
        ((uint32_t)csr << 20) |
        ((uint32_t)rs1 << 15) |
        ((uint32_t)funct3 << 12) |
        ((uint32_t)rd << 7) |
        0x73;

    return 0;
}

// ========================================
// System-type
// ========================================

int encode_system_type(
    const char *mnemonic,
    uint32_t *instruction
)
{
    if (strcmp(mnemonic, "mret") == 0) {
        *instruction = 0x30200073;
    }
    else if (strcmp(mnemonic, "sret") == 0) {
        *instruction = 0x10200073;
    }
    else if (strcmp(mnemonic, "uret") == 0) {
        *instruction = 0x00200073;
    }
    else if (strcmp(mnemonic, "ecall") == 0) {
        *instruction = 0x00000073;
    }
    else if (strcmp(mnemonic, "ebreak") == 0) {
        *instruction = 0x00100073;
    }
    else if (strcmp(mnemonic, "wfi") == 0) {
        *instruction = 0x10500073;
    }
    else if (strcmp(mnemonic, "fence") == 0) {
        *instruction = 0x0FF0000F;
    }
    else if (strcmp(mnemonic, "fence.i") == 0) {
        *instruction = 0x0000100F;
    }
    else {
        return -1;
    }

    return 0;
}