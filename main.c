
#include <stdio.h>
#include <inttypes.h>

#include "register.h"
#include "encoder.h"

int main(void)
{
    uint32_t instruction;

    // 레지스터 번호 변환
    int rd  = get_register_number("t0");
    int rs1 = get_register_number("a0");
    int rs2 = get_register_number("a1");
    int t1  = get_register_number("t1");

    printf("RV32IM Assembler Start!\n\n");

    // =========================
    // R-type Tests
    // =========================

    printf("===== R-type Tests =====\n");

    if (encode_r_type("add", rd, rs1, rs2, &instruction) == 0) {
        printf("add t0, a0, a1 = 0x%08" PRIX32 "\n", instruction);
    }

    if (encode_r_type("sub", rd, rs1, rs2, &instruction) == 0) {
        printf("sub t0, a0, a1 = 0x%08" PRIX32 "\n", instruction);
    }

    if (encode_r_type("mul", rd, rs1, rs2, &instruction) == 0) {
        printf("mul t0, a0, a1 = 0x%08" PRIX32 "\n", instruction);
    }

    // =========================
    // I-type Tests
    // =========================

    printf("\n===== I-type Tests =====\n");

    if (encode_i_type("addi", rd, t1, 10, &instruction) == 0) {
        printf("addi t0, t1, 10 = 0x%08" PRIX32 "\n", instruction);
    }

    if (encode_i_type("addi", rd, t1, -1, &instruction) == 0) {
        printf("addi t0, t1, -1 = 0x%08" PRIX32 "\n", instruction);
    }

    if (encode_i_type("lw", rd, rs1, 8, &instruction) == 0) {
        printf("lw t0, 8(a0) = 0x%08" PRIX32 "\n", instruction);
    }

    if (encode_i_type("srai", rd, t1, 3, &instruction) == 0) {
        printf("srai t0, t1, 3 = 0x%08" PRIX32 "\n", instruction);
    }

    printf("\n===== S-type Tests =====\n");

    encode_s_type("sb", 2, 5, 8, &instruction);
    printf("sb t0, 8(sp) = 0x%08X\n", instruction);

    encode_s_type("sh", 2, 5, 8, &instruction);
    printf("sh t0, 8(sp) = 0x%08X\n", instruction);

    encode_s_type("sw", 2, 5, 8, &instruction);
    printf("sw t0, 8(sp) = 0x%08X\n", instruction);

    encode_s_type("sw", 2, 5, -8, &instruction);
    printf("sw t0, -8(sp) = 0x%08X\n", instruction);

    printf("\n===== B-type Tests =====\n");

    encode_b_type("beq", rd, t1, 16, &instruction);
    printf("beq t0, t1, 16 = 0x%08X\n", instruction);

    encode_b_type("bne", rd, t1, 16, &instruction);
    printf("bne t0, t1, 16 = 0x%08X\n", instruction);

    encode_b_type("blt", rd, t1, 16, &instruction);
    printf("blt t0, t1, 16 = 0x%08X\n", instruction);

    encode_b_type("bge", rd, t1, 16, &instruction);
    printf("bge t0, t1, 16 = 0x%08X\n", instruction);

    encode_b_type("bltu", rd, t1, 16, &instruction);
    printf("bltu t0, t1, 16 = 0x%08X\n", instruction);

    encode_b_type("bgeu", rd, t1, 16, &instruction);
    printf("bgeu t0, t1, 16 = 0x%08X\n", instruction);

    encode_b_type("beq", rd, t1, -4, &instruction);
    printf("beq t0, t1, -4 = 0x%08X\n", instruction);

    encode_b_type("beq", rd, t1, -4, &instruction);
    printf("beq t0, t1, -4 = 0x%08X\n", instruction);

    printf("\n===== U-type Tests =====\n");

    encode_u_type("lui", rd, 0x12345, &instruction);
    printf("lui t0, 0x12345 = 0x%08X\n", instruction);

    encode_u_type("auipc", t1, 0xABCDE, &instruction);
    printf("auipc t1, 0xABCDE = 0x%08X\n", instruction);

    encode_u_type("lui", rd, -1, &instruction);
    printf("lui t0, -1 = 0x%08X\n", instruction);

    printf("\n===== J-type Tests =====\n");

    encode_j_type("jal", rd, 100, &instruction);
    printf("jal t0, 100 = 0x%08X\n", instruction);

    encode_j_type("jal", 1, 2048, &instruction);
    printf("jal ra, 2048 = 0x%08X\n", instruction);

    encode_j_type("jal", 0, -4, &instruction);
    printf("jal x0, -4 = 0x%08X\n", instruction);

    return 0;
}

