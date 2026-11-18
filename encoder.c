
#include <stdio.h>
#include <string.h>
#include "encoder.h"

#define R_OPCODE 0x33

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

    // 레지스터 번호 범위 검사
    if (rd < 0 || rd > 31 ||
        rs1 < 0 || rs1 > 31 ||
        rs2 < 0 || rs2 > 31) {
        return -1;
    }

    int funct3 = -1;
    int funct7 = -1;

    // 명령어 테이블 검색
    int count = sizeof(r_table) / sizeof(r_table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(mnemonic, r_table[i].name) == 0) {
            funct3 = r_table[i].funct3;
            funct7 = r_table[i].funct7;
            break;
        }
    }

    // 지원하지 않는 명령어
    if (funct3 == -1) {
        return -1;
    }

    // R-type 기계어 생성
    *instruction =
        ((uint32_t)funct7 << 25) |
        ((uint32_t)rs2 << 20) |
        ((uint32_t)rs1 << 15) |
        ((uint32_t)funct3 << 12) |
        ((uint32_t)rd << 7) |
        R_OPCODE;

    return 0;
}
