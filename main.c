
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

    return 0;
}

