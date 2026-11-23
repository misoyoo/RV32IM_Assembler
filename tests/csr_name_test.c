#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "encoder.h"

typedef struct {
    const char *name;
    int address;
} NamedCsr;

static int check_named_csr(const NamedCsr *csr)
{
    uint32_t instruction = 0;
    uint32_t expected = ((uint32_t)csr->address << 20) |
                        (2u << 15) | (1u << 12) | (1u << 7) | 0x73u;
    int result = encode_csr_type("csrrw", 1, csr->name, 2, &instruction);
    int passed = result == 0 && instruction == expected;

    printf("[%s] specified CSR %s\n", passed ? "PASS" : "FAIL", csr->name);
    return passed ? 0 : -1;
}

static int check_rejected_name(const char *name)
{
    uint32_t instruction = 0;
    int passed = encode_csr_type("csrrw", 1, name, 2, &instruction) != 0;

    printf("[%s] reject extra CSR name %s\n",
           passed ? "PASS" : "FAIL", name);
    return passed ? 0 : -1;
}

static int check_numeric_address(const char *text, int address)
{
    uint32_t instruction = 0;
    uint32_t expected = ((uint32_t)address << 20) |
                        (2u << 15) | (1u << 12) | (1u << 7) | 0x73u;
    int result = encode_csr_type("csrrw", 1, text, 2, &instruction);
    int passed = result == 0 && instruction == expected;

    printf("[%s] numeric CSR address %s\n",
           passed ? "PASS" : "FAIL", text);
    return passed ? 0 : -1;
}

int main(void)
{
    static const NamedCsr specified[] = {
        {"mstatus", 0x300}, {"mie", 0x304}, {"mtvec", 0x305},
        {"mscratch", 0x340}, {"mepc", 0x341}, {"mcause", 0x342},
        {"mtval", 0x343}, {"mip", 0x344}, {"mhartid", 0xF14}
    };
    static const char *const extra_names[] = {
        "misa", "medeleg", "mideleg", "mcounteren",
        "cycle", "time", "instret"
    };
    int failed = 0;

    for (size_t i = 0; i < sizeof(specified) / sizeof(specified[0]); i++) {
        failed += check_named_csr(&specified[i]) != 0;
    }
    for (size_t i = 0; i < sizeof(extra_names) / sizeof(extra_names[0]); i++) {
        failed += check_rejected_name(extra_names[i]) != 0;
    }
    failed += check_numeric_address("0x301", 0x301) != 0;
    failed += check_numeric_address("0xC00", 0xC00) != 0;
    failed += check_numeric_address("4095", 4095) != 0;

    printf("CSR name tests: %s\n", failed == 0 ? "PASS" : "FAIL");
    return failed == 0 ? 0 : 1;
}
