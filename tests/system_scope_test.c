#include <stdio.h>
#include <string.h>

#include "encoder.h"
#include "pass1.h"
#include "symbol_table.h"

typedef struct {
    const char *mnemonic;
    uint32_t expected_instruction;
} RequiredInstruction;

static int check_encoder(
    const char *mnemonic,
    int should_encode,
    uint32_t expected_instruction
)
{
    uint32_t instruction = 0;
    int encoded = encode_system_type(mnemonic, &instruction) == 0;
    int passed = encoded == should_encode &&
                 (!should_encode || instruction == expected_instruction);
    printf("[%s] encoder %s\n", passed ? "PASS" : "FAIL", mnemonic);
    return passed ? 0 : -1;
}

static int check_pass1(const char *mnemonic, int should_pass)
{
    char source_text[128];
    snprintf(source_text, sizeof(source_text), ".text\n%s\n", mnemonic);

    FILE *source = tmpfile();
    if (source == NULL) {
        perror("tmpfile");
        return -1;
    }
    if (fputs(source_text, source) == EOF || fflush(source) != 0) {
        fclose(source);
        return -1;
    }

    SymbolTable symbols;
    if (symbol_table_init(&symbols) != 0) {
        fclose(source);
        return -1;
    }

    Pass1Layout layout;
    int passed_assembly = pass1_run(source, &symbols, &layout) == 0;
    int passed = passed_assembly == should_pass;
    printf("[%s] Pass 1 %s\n", passed ? "PASS" : "FAIL", mnemonic);

    symbol_table_destroy(&symbols);
    fclose(source);
    return passed ? 0 : -1;
}

int main(void)
{
    static const RequiredInstruction required[] = {
        {"mret", 0x30200073},
        {"wfi", 0x10500073},
        {"ecall", 0x00000073},
        {"ebreak", 0x00100073},
        {"fence", 0x0FF0000F}
    };
    static const char *const excluded[] = {
        "sret", "fence.i", "uret"
    };
    int failed = 0;

    for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); i++) {
        failed += check_encoder(required[i].mnemonic, 1,
                                required[i].expected_instruction) != 0;
        failed += check_pass1(required[i].mnemonic, 1) != 0;
    }
    for (size_t i = 0; i < sizeof(excluded) / sizeof(excluded[0]); i++) {
        failed += check_encoder(excluded[i], 0, 0) != 0;
        failed += check_pass1(excluded[i], 0) != 0;
    }

    printf("System scope tests: %s\n", failed == 0 ? "PASS" : "FAIL");
    return failed == 0 ? 0 : 1;
}
