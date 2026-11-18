
#include <stdio.h>
#include <inttypes.h>

#include "register.h"
#include "encoder.h"

int main(void)
{
    uint32_t instruction;

    int rd  = get_register_number("t0");
    int rs1 = get_register_number("a0");
    int rs2 = get_register_number("a1");

    printf("RV32IM Assembler Start!\n\n");

    if (encode_r_type("add", rd, rs1, rs2, &instruction) == 0) {
        printf("add t0, a0, a1 = 0x%08" PRIX32 "\n", instruction);
    } else {
        printf("Encoding Error!\n");
    }

    if (encode_r_type("sub", rd, rs1, rs2, &instruction) == 0) {
        printf("sub t0, a0, a1 = 0x%08" PRIX32 "\n", instruction);
    }

    if (encode_r_type("mul", rd, rs1, rs2, &instruction) == 0) {
        printf("mul t0, a0, a1 = 0x%08" PRIX32 "\n", instruction);
    }

    return 0;
}
