
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "pass2.h"
#include "parser.h"
#include "encoder.h"
#include "register.h"
#include "directives.h"

/* ========================================
   Utility Functions
   ======================================== */

static int is_one_of(
    const char *value,
    const char *const names[],
    size_t count
)
{
    for (size_t i = 0; i < count; i++) {
        if (strcmp(value, names[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static int evaluate_signed(
    const char *expression,
    const SymbolTable *symbols,
    int *value
)
{
    int64_t signed_value;

    if (evaluate_expression_signed(
            expression,
            symbols,
            &signed_value
        ) != 0 ||
        signed_value < INT_MIN ||
        signed_value > INT_MAX) {
        return -1;
    }

    *value = (int)signed_value;

    return 0;
}

static int get_register(
    const char *operand,
    int *number
)
{
    if (operand == NULL || number == NULL) {
        return -1;
    }

    int result = get_register_number(operand);

    if (result < 0 || result > 31) {
        return -1;
    }

    *number = result;

    return 0;
}

static void trim_operand(char *text)
{
    size_t length = strlen(text);

    while (length > 0 &&
           (text[length - 1] == ' ' ||
            text[length - 1] == '\t')) {
        text[--length] = '\0';
    }

    size_t start = 0;

    while (text[start] == ' ' || text[start] == '\t') {
        start++;
    }

    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

/* ========================================
   Memory Operand
   Example: 8(sp)
   ======================================== */

static int parse_memory_operand(
    const char *operand,
    const SymbolTable *symbols,
    int *immediate,
    int *base_register
)
{
    if (operand == NULL ||
        immediate == NULL ||
        base_register == NULL) {
        return -1;
    }

    const char *left = strchr(operand, '(');
    const char *right = strchr(operand, ')');

    if (left == NULL ||
        right == NULL ||
        right < left ||
        right[1] != '\0') {
        return -1;
    }

    size_t offset_length = (size_t)(left - operand);
    size_t register_length = (size_t)(right - left - 1);

    char offset_text[256];
    char register_text[64];

    if (offset_length >= sizeof(offset_text) ||
        register_length == 0 ||
        register_length >= sizeof(register_text)) {
        return -1;
    }

    memcpy(offset_text, operand, offset_length);
    offset_text[offset_length] = '\0';

    memcpy(register_text, left + 1, register_length);
    register_text[register_length] = '\0';

    trim_operand(offset_text);
    trim_operand(register_text);

    if (offset_text[0] == '\0') {
        *immediate = 0;
    }
    else if (evaluate_signed(
                 offset_text,
                 symbols,
                 immediate
             ) != 0) {
        return -1;
    }

    return get_register(register_text, base_register);
}

/* ========================================
   Branch / Jump Offset
   ======================================== */

static int get_target_offset(
    const char *operand,
    const SymbolTable *symbols,
    uint32_t current_address,
    int *offset
)
{
    uint32_t target_address;

    if (symbol_table_get(
            symbols,
            operand,
            &target_address
        ) == 0) {

        int64_t difference =
            (int64_t)target_address - (int64_t)current_address;

        if (difference < INT_MIN || difference > INT_MAX) {
            return -1;
        }

        *offset = (int)difference;

        return 0;
    }

    return evaluate_signed(operand, symbols, offset);
}

/* ========================================
   Instruction Classification
   ======================================== */

static int is_r_type(const char *mnemonic)
{
    static const char *const names[] = {
        "add", "sub", "sll", "slt", "sltu",
        "xor", "srl", "sra", "or", "and",
        "mul", "mulh", "mulhsu", "mulhu",
        "div", "divu", "rem", "remu"
    };

    return is_one_of(
        mnemonic,
        names,
        sizeof(names) / sizeof(names[0])
    );
}

static int is_i_arithmetic(const char *mnemonic)
{
    static const char *const names[] = {
        "addi", "slti", "sltiu", "xori",
        "ori", "andi", "slli", "srli", "srai"
    };

    return is_one_of(
        mnemonic,
        names,
        sizeof(names) / sizeof(names[0])
    );
}

static int is_load(const char *mnemonic)
{
    static const char *const names[] = {
        "lb", "lh", "lw", "lbu", "lhu"
    };

    return is_one_of(
        mnemonic,
        names,
        sizeof(names) / sizeof(names[0])
    );
}

static int is_store(const char *mnemonic)
{
    static const char *const names[] = {
        "sb", "sh", "sw"
    };

    return is_one_of(
        mnemonic,
        names,
        sizeof(names) / sizeof(names[0])
    );
}

static int is_branch(const char *mnemonic)
{
    static const char *const names[] = {
        "beq", "bne", "blt", "bge", "bltu", "bgeu"
    };

    return is_one_of(
        mnemonic,
        names,
        sizeof(names) / sizeof(names[0])
    );
}

/* ========================================
   CSR Instruction
   ======================================== */

static int encode_csr_instruction(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t *instruction
)
{
    const char *mnemonic = parsed_line->mnemonic;

    int rd;
    int rs1;
    int immediate;
    const char *csr_name;

    if (strcmp(mnemonic, "csrw") == 0 ||
        strcmp(mnemonic, "csrs") == 0 ||
        strcmp(mnemonic, "csrc") == 0) {

        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[1], &rs1) != 0) {
            return -1;
        }

        csr_name = parsed_line->operands[0];

        return encode_csr_type(
            mnemonic,
            0,
            csr_name,
            rs1,
            instruction
        );
    }

    if (strcmp(mnemonic, "csrwi") == 0 ||
        strcmp(mnemonic, "csrsi") == 0 ||
        strcmp(mnemonic, "csrci") == 0) {

        if (parsed_line->operand_count != 2 ||
            evaluate_signed(
                parsed_line->operands[1],
                symbols,
                &immediate
            ) != 0 ||
            immediate < 0 ||
            immediate > 31) {
            return -1;
        }

        csr_name = parsed_line->operands[0];

        return encode_csr_type(
            mnemonic,
            0,
            csr_name,
            immediate,
            instruction
        );
    }

    if (strcmp(mnemonic, "csrr") == 0) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0) {
            return -1;
        }

        return encode_csr_type(
            mnemonic,
            rd,
            parsed_line->operands[1],
            0,
            instruction
        );
    }

    if (parsed_line->operand_count != 3 ||
        get_register(parsed_line->operands[0], &rd) != 0) {
        return -1;
    }

    csr_name = parsed_line->operands[1];

    if (strcmp(mnemonic, "csrrwi") == 0 ||
        strcmp(mnemonic, "csrrsi") == 0 ||
        strcmp(mnemonic, "csrrci") == 0) {

        if (evaluate_signed(
                parsed_line->operands[2],
                symbols,
                &immediate
            ) != 0 ||
            immediate < 0 ||
            immediate > 31) {
            return -1;
        }

        rs1 = immediate;
    }
    else {
        if (get_register(parsed_line->operands[2], &rs1) != 0) {
            return -1;
        }
    }

    return encode_csr_type(
        mnemonic,
        rd,
        csr_name,
        rs1,
        instruction
    );
}

/* ========================================
   Base Instruction Encoder
   ======================================== */

static int encode_base_instruction(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t current_address,
    uint32_t *instruction
)

{
    const char *mnemonic = parsed_line->mnemonic;

    int rd;
    int rs1;
    int rs2;
    int immediate;

    /* R-type */

    if (is_r_type(mnemonic)) {
        if (parsed_line->operand_count != 3 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs1) != 0 ||
            get_register(parsed_line->operands[2], &rs2) != 0) {
            return -1;
        }

        return encode_r_type(
            mnemonic,
            rd,
            rs1,
            rs2,
            instruction
        );
    }

    /* I-type arithmetic */

    if (is_i_arithmetic(mnemonic)) {
        if (parsed_line->operand_count != 3 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs1) != 0 ||
            evaluate_signed(
                parsed_line->operands[2],
                symbols,
                &immediate
            ) != 0) {
            return -1;
        }

        return encode_i_type(
            mnemonic,
            rd,
            rs1,
            immediate,
            instruction
        );
    }

    /* Load instructions */

    if (is_load(mnemonic)) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            parse_memory_operand(
                parsed_line->operands[1],
                symbols,
                &immediate,
                &rs1
            ) != 0) {
            return -1;
        }

        return encode_i_type(
            mnemonic,
            rd,
            rs1,
            immediate,
            instruction
        );
    }

    /* Store instructions */

    if (is_store(mnemonic)) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rs2) != 0 ||
            parse_memory_operand(
                parsed_line->operands[1],
                symbols,
                &immediate,
                &rs1
            ) != 0) {
            return -1;
        }

        return encode_s_type(
            mnemonic,
            rs1,
            rs2,
            immediate,
            instruction
        );
    }

    /* B-type */

    if (is_branch(mnemonic)) {
        if (parsed_line->operand_count != 3 ||
            get_register(parsed_line->operands[0], &rs1) != 0 ||
            get_register(parsed_line->operands[1], &rs2) != 0 ||
            get_target_offset(
                parsed_line->operands[2],
                symbols,
                current_address,
                &immediate
            ) != 0) {
            return -1;
        }

        return encode_b_type(
            mnemonic,
            rs1,
            rs2,
            immediate,
            instruction
        );
    }

    /* U-type */

    if (strcmp(mnemonic, "lui") == 0 ||
        strcmp(mnemonic, "auipc") == 0) {

        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            evaluate_signed(
                parsed_line->operands[1],
                symbols,
                &immediate
            ) != 0) {
            return -1;
        }

        return encode_u_type(
            mnemonic,
            rd,
            immediate,
            instruction
        );
    }


    /* J-type: jal label or jal rd, label */

    if (strcmp(mnemonic, "jal") == 0) {
        const char *target_operand;

    if (parsed_line->operand_count == 1) {
        /* Default destination register: ra (x1) */
        rd = 1;
        target_operand = parsed_line->operands[0];
    }
        else if (parsed_line->operand_count == 2) {
            if (get_register(
                    parsed_line->operands[0],
                    &rd
                ) != 0) {
                return -1;
            }

            target_operand = parsed_line->operands[1];
        }
        else {
            return -1;
        }

        if (get_target_offset(
                target_operand,
                symbols,
                current_address,
                &immediate
            ) != 0) {
            return -1;
        }

        return encode_j_type(
            mnemonic,
            rd,
            immediate,
            instruction
        );
    }

    /* jalr: rd, offset(rs1) */

    if (strcmp(mnemonic, "jalr") == 0) {
        if (parsed_line->operand_count == 2) {
            if (get_register(parsed_line->operands[0], &rd) != 0 ||
                parse_memory_operand(
                    parsed_line->operands[1],
                    symbols,
                    &immediate,
                    &rs1
                ) != 0) {
                return -1;
            }
        }
        else if (parsed_line->operand_count == 3) {
            if (get_register(parsed_line->operands[0], &rd) != 0 ||
                get_register(parsed_line->operands[1], &rs1) != 0 ||
                evaluate_signed(
                    parsed_line->operands[2],
                    symbols,
                    &immediate
                ) != 0) {
                return -1;
            }
        }
        else {
            return -1;
        }

        return encode_i_type(
            mnemonic,
            rd,
            rs1,
            immediate,
            instruction
        );
    }

    /* CSR instructions */

    if (strncmp(mnemonic, "csr", 3) == 0) {
        return encode_csr_instruction(
            parsed_line,
            symbols,
            instruction
        );
    }

    /* System instructions */

    if (strcmp(mnemonic, "mret") == 0 ||
        strcmp(mnemonic, "ecall") == 0 ||
        strcmp(mnemonic, "ebreak") == 0 ||
        strcmp(mnemonic, "wfi") == 0 ||
        strcmp(mnemonic, "fence") == 0) {

        if (parsed_line->operand_count != 0) {
            return -1;
        }

        return encode_system_type(mnemonic, instruction);
    }

    return -1;
}


/* ========================================
   Pseudo-instruction Helpers
   ======================================== */

static int split_immediate(
    int64_t value,
    int *upper,
    int *lower
)
{
    if (upper == NULL || lower == NULL) {
        return -1;
    }

    int64_t adjusted = value + 0x800;
    int64_t high = adjusted / 4096;

    /* Convert truncation toward zero to floor division */
    if (adjusted < 0 && adjusted % 4096 != 0) {
        high--;
    }

    int64_t low = value - high * 4096;

    if (high < -524288 || high > 1048575 ||
        low < -2048 || low > 2047) {
        return -1;
    }

    *upper = (int)high;
    *lower = (int)low;

    return 0;
}

static int get_pc_relative_offset(
    const char *expression,
    const SymbolTable *symbols,
    uint32_t current_address,
    int *offset
)
{
    uint32_t target;

    if (evaluate_expression(
            expression,
            symbols,
            &target
        ) != 0) {
        return -1;
    }

    int64_t difference =
        (int64_t)(uint64_t)target -
        (int64_t)(uint64_t)current_address;

    if (difference < INT32_MIN ||
        difference > INT32_MAX) {
        return -1;
    }

    *offset = (int)difference;

    return 0;
}

/* ========================================
   Pseudo Branch Instructions
   ======================================== */

static int encode_pseudo_branch(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t current_address,
    uint32_t *instruction
)
{
    const char *mnemonic = parsed_line->mnemonic;
    const char *branch_mnemonic = NULL;
    const char *target_operand = NULL;

    int rs1 = -1;
    int rs2 = -1;

    if (strcmp(mnemonic, "beqz") == 0 ||
        strcmp(mnemonic, "bnez") == 0 ||
        strcmp(mnemonic, "bltz") == 0 ||
        strcmp(mnemonic, "bgez") == 0 ||
        strcmp(mnemonic, "blez") == 0 ||
        strcmp(mnemonic, "bgtz") == 0) {

        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rs1) != 0) {
            return -1;
        }

        rs2 = 0;
        target_operand = parsed_line->operands[1];

        if (strcmp(mnemonic, "beqz") == 0) {
            branch_mnemonic = "beq";
        }
        else if (strcmp(mnemonic, "bnez") == 0) {
            branch_mnemonic = "bne";
        }
        else if (strcmp(mnemonic, "bltz") == 0) {
            branch_mnemonic = "blt";
        }
        else if (strcmp(mnemonic, "bgez") == 0) {
            branch_mnemonic = "bge";
        }
        else if (strcmp(mnemonic, "blez") == 0) {
            branch_mnemonic = "bge";
            rs2 = rs1;
            rs1 = 0;
        }
        else {
            branch_mnemonic = "blt";
            rs2 = rs1;
            rs1 = 0;
        }
    }
    else {
        if (parsed_line->operand_count != 3 ||
            get_register(parsed_line->operands[0], &rs1) != 0 ||
            get_register(parsed_line->operands[1], &rs2) != 0) {
            return -1;
        }

        target_operand = parsed_line->operands[2];

        if (strcmp(mnemonic, "ble") == 0) {
            branch_mnemonic = "bge";
        }
        else if (strcmp(mnemonic, "bgt") == 0) {
            branch_mnemonic = "blt";
        }
        else if (strcmp(mnemonic, "bleu") == 0) {
            branch_mnemonic = "bgeu";
        }
        else if (strcmp(mnemonic, "bgtu") == 0) {
            branch_mnemonic = "bltu";
        }
        else {
            return 0;
        }

        /* Reverse operands for greater-than / less-or-equal */
        int temporary = rs1;
        rs1 = rs2;
        rs2 = temporary;
    }

    int offset;

    if (get_target_offset(
            target_operand,
            symbols,
            current_address,
            &offset
        ) != 0) {
        return -1;
    }

    return encode_b_type(
        branch_mnemonic,
        rs1,
        rs2,
        offset,
        instruction
    );
}

/* ========================================
   Pseudo-instruction Encoder
   Return values:
   1: handled successfully
   0: not a pseudo-instruction
  -1: invalid pseudo-instruction
   ======================================== */

static int encode_pseudo_instruction(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t current_address,
    uint32_t instructions[2],
    uint32_t *instruction_count
)
{
    if (parsed_line == NULL ||
        symbols == NULL ||
        instructions == NULL ||
        instruction_count == NULL) {
        return -1;
    }

    const char *mnemonic = parsed_line->mnemonic;

    int rd;
    int rs1;
    int rs2;
    int immediate;

    *instruction_count = 0;

    /* li: addi or lui + addi */

    if (strcmp(mnemonic, "li") == 0) {
        int64_t li_value;

        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            evaluate_li_immediate(
                parsed_line->operands[1],
                symbols,
                &li_value
            ) != 0) {
            return -1;
        }

        if (li_value >= -2048 && li_value <= 2047) {
            if (encode_i_type(
                    "addi",
                    rd,
                    0,
                    (int)li_value,
                    &instructions[0]
                ) != 0) {
                return -1;
            }

            *instruction_count = 1;
            return 1;
        }

        int upper;
        int lower;

        if (split_immediate(
                li_value,
                &upper,
                &lower
            ) != 0) {
            return -1;
        }

        if (encode_u_type(
                "lui",
                rd,
                upper,
                &instructions[0]
            ) != 0 ||
            encode_i_type(
                "addi",
                rd,
                rd,
                lower,
                &instructions[1]
            ) != 0) {
            return -1;
        }

        *instruction_count = 2;
        return 1;
    }

    /* la: auipc + addi */

    if (strcmp(mnemonic, "la") == 0) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_pc_relative_offset(
                parsed_line->operands[1],
                symbols,
                current_address,
                &immediate
            ) != 0) {
            return -1;
        }

        int upper;
        int lower;

        if (split_immediate(
                (int64_t)immediate,
                &upper,
                &lower
            ) != 0) {
            return -1;
        }

        if (encode_u_type(
                "auipc",
                rd,
                upper,
                &instructions[0]
            ) != 0 ||
            encode_i_type(
                "addi",
                rd,
                rd,
                lower,
                &instructions[1]
            ) != 0) {
            return -1;
        }

        *instruction_count = 2;
        return 1;
    }

    /* call: jal ra, target */

    if (strcmp(mnemonic, "call") == 0) {
        if (parsed_line->operand_count != 1 ||
            get_pc_relative_offset(
                parsed_line->operands[0],
                symbols,
                current_address,
                &immediate
            ) != 0) {
            return -1;
        }

        if (encode_j_type(
                "jal",
                1,
                immediate,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* tail: auipc + jalr */

    if (strcmp(mnemonic, "tail") == 0) {
        if (parsed_line->operand_count != 1 ||
            get_pc_relative_offset(
                parsed_line->operands[0],
                symbols,
                current_address,
                &immediate
            ) != 0) {
            return -1;
        }

        int upper;
        int lower;

        if (split_immediate(
                (int64_t)immediate,
                &upper,
                &lower
            ) != 0) {
            return -1;
        }

        if (encode_u_type(
                "auipc",
                6,
                upper,
                &instructions[0]
            ) != 0 ||
            encode_i_type(
                "jalr",
                0,
                6,
                lower,
                &instructions[1]
            ) != 0) {
            return -1;
        }

        *instruction_count = 2;
        return 1;
    }

    /* j: jal x0, target */

    if (strcmp(mnemonic, "j") == 0) {
        if (parsed_line->operand_count != 1 ||
            get_target_offset(
                parsed_line->operands[0],
                symbols,
                current_address,
                &immediate
            ) != 0) {
            return -1;
        }

        if (encode_j_type(
                "jal",
                0,
                immediate,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* jr: jalr x0, rs1, 0 */

    if (strcmp(mnemonic, "jr") == 0) {
        if (parsed_line->operand_count != 1 ||
            get_register(parsed_line->operands[0], &rs1) != 0) {
            return -1;
        }

        if (encode_i_type(
                "jalr",
                0,
                rs1,
                0,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* ret: jalr x0, ra, 0 */

    if (strcmp(mnemonic, "ret") == 0) {
        if (parsed_line->operand_count != 0 ||
            encode_i_type(
                "jalr",
                0,
                1,
                0,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* nop: addi x0, x0, 0 */

    if (strcmp(mnemonic, "nop") == 0) {
        if (parsed_line->operand_count != 0 ||
            encode_i_type(
                "addi",
                0,
                0,
                0,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* mv: addi rd, rs, 0 */

    if (strcmp(mnemonic, "mv") == 0) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs1) != 0) {
            return -1;
        }

        if (encode_i_type(
                "addi",
                rd,
                rs1,
                0,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* not: xori rd, rs, -1 */

    if (strcmp(mnemonic, "not") == 0) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs1) != 0) {
            return -1;
        }

        if (encode_i_type(
                "xori",
                rd,
                rs1,
                -1,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* neg: sub rd, x0, rs */

    if (strcmp(mnemonic, "neg") == 0) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs2) != 0) {
            return -1;
        }

        if (encode_r_type(
                "sub",
                rd,
                0,
                rs2,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* seqz: sltiu rd, rs, 1 */

    if (strcmp(mnemonic, "seqz") == 0) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs1) != 0) {
            return -1;
        }

        if (encode_i_type(
                "sltiu",
                rd,
                rs1,
                1,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* snez: sltu rd, x0, rs */

    if (strcmp(mnemonic, "snez") == 0) {
        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs2) != 0) {
            return -1;
        }

        if (encode_r_type(
                "sltu",
                rd,
                0,
                rs2,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* sltz / sgtz */

    if (strcmp(mnemonic, "sltz") == 0 ||
        strcmp(mnemonic, "sgtz") == 0) {

        if (parsed_line->operand_count != 2 ||
            get_register(parsed_line->operands[0], &rd) != 0 ||
            get_register(parsed_line->operands[1], &rs1) != 0) {
            return -1;
        }

        rs2 = 0;

        if (strcmp(mnemonic, "sgtz") == 0) {
            rs2 = rs1;
            rs1 = 0;
        }

        if (encode_r_type(
                "slt",
                rd,
                rs1,
                rs2,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    /* Branch pseudo-instructions */

    static const char *const branch_pseudos[] = {
        "beqz", "bnez", "bltz", "bgez",
        "blez", "bgtz", "ble", "bgt",
        "bleu", "bgtu"
    };

    if (is_one_of(
            mnemonic,
            branch_pseudos,
            sizeof(branch_pseudos) / sizeof(branch_pseudos[0])
        )) {

        if (encode_pseudo_branch(
                parsed_line,
                symbols,
                current_address,
                &instructions[0]
            ) != 0) {
            return -1;
        }

        *instruction_count = 1;
        return 1;
    }

    return 0;
}

/* ========================================
   Write Instruction in Little Endian
   ======================================== */

static int write_instruction(
    uint8_t *buffer,
    uint32_t buffer_size,
    uint32_t offset,
    uint32_t instruction
)
{
    if (buffer == NULL ||
        offset > buffer_size ||
        buffer_size - offset < 4) {
        return -1;
    }

    buffer[offset] = (uint8_t)(instruction & 0xFF);
    buffer[offset + 1] = (uint8_t)((instruction >> 8) & 0xFF);
    buffer[offset + 2] = (uint8_t)((instruction >> 16) & 0xFF);
    buffer[offset + 3] = (uint8_t)((instruction >> 24) & 0xFF);

    return 0;
}

/* ========================================
   Read and Parse Source Line
   ======================================== */

static int read_parsed_line(
    FILE *source,
    ParsedLine *parsed_line,
    uint32_t *line_number,
    char *source_line
)
{
    if (source == NULL ||
        parsed_line == NULL ||
        line_number == NULL ||
        source_line == NULL) {
        return -1;
    }

    char buffer[PARSER_MAX_LINE_LENGTH];

    if (fgets(buffer, sizeof(buffer), source) == NULL) {
        return ferror(source) ? -1 : 0;
    }

    (*line_number)++;

    size_t length = strlen(buffer);

    if (length > 0 &&
        buffer[length - 1] != '\n' &&
        !feof(source)) {

        int ch;

        while ((ch = fgetc(source)) != '\n' && ch != EOF) {
        }

        return -1;
    }

    /* Remove newline characters but preserve original formatting */

    while (length > 0 &&
           (buffer[length - 1] == '\n' ||
            buffer[length - 1] == '\r')) {
        buffer[--length] = '\0';
    }

    /* Preserve original source line */

    memcpy(source_line, buffer, length + 1);

    /* Parse the source line */

    if (parse_line(buffer, parsed_line) != 0) {
        fprintf(
            stderr,
            "Pass 2 error at line %u: parsing failed\n",
            *line_number
        );

        return -1;
    }

    return 1;
}

/* ========================================
   Append Listing Entry
   ======================================== */

static int append_listing_entry(
    Pass2Result *result,
    uint32_t address,
    uint32_t machine_code,
    uint8_t byte_count,
    const char *source_line
)
{
    if (result == NULL ||
        source_line == NULL ||
        byte_count == 0 ||
        byte_count > 4) {
        return -1;
    }

    /* Expand listing array when full */

    if (result->listing_count == result->listing_capacity) {
        size_t new_capacity;

        if (result->listing_capacity == 0) {
            new_capacity = 32;
        }
        else {
            new_capacity = result->listing_capacity * 2;

            if (new_capacity < result->listing_capacity) {
                return -1;
            }
        }

        if (new_capacity > (size_t)-1 / sizeof(Pass2ListingEntry)) {
            return -1;
        }

        Pass2ListingEntry *new_entries = realloc(
            result->listing_entries,
            new_capacity * sizeof(Pass2ListingEntry)
        );

        if (new_entries == NULL) {
            return -1;
        }

        result->listing_entries = new_entries;
        result->listing_capacity = new_capacity;
    }

    Pass2ListingEntry *entry =
        &result->listing_entries[result->listing_count];

    entry->address = address;
    entry->machine_code = machine_code;
    entry->byte_count = byte_count;

    int written = snprintf(
        entry->source_line,
        sizeof(entry->source_line),
        "%s",
        source_line
    );

    if (written < 0 ||
        (size_t)written >= sizeof(entry->source_line)) {
        return -1;
    }

    result->listing_count++;

    return 0;
}
/* ========================================
   Append Data Listing Entries
   ======================================== */

static int append_data_listing_entries(
    Pass2Result *result,
    uint32_t address,
    const uint8_t *bytes,
    uint32_t byte_count,
    const char *source_line
)
{
    if (result == NULL ||
        (byte_count > 0 && bytes == NULL) ||
        source_line == NULL) {
        return -1;
    }

    uint32_t offset = 0;

    while (offset < byte_count) {
        uint32_t chunk_size = byte_count - offset;

        if (chunk_size > 4) {
            chunk_size = 4;
        }

        uint32_t machine_code = 0;

        for (uint32_t i = 0; i < chunk_size; i++) {
            machine_code |=
                (uint32_t)bytes[offset + i] << (i * 8);
        }

        const char *entry_source =
            (offset == 0) ? source_line : "";

        if (append_listing_entry(
                result,
                address + offset,
                machine_code,
                (uint8_t)chunk_size,
                entry_source
            ) != 0) {
            return -1;
        }

        offset += chunk_size;
    }

    return 0;
}
/* ========================================
   Data Emission Helpers
   ======================================== */

static int write_data_byte(
    uint8_t *buffer,
    uint32_t buffer_size,
    uint32_t offset,
    uint8_t value
)
{
    if (buffer == NULL || offset >= buffer_size) {
        return -1;
    }

    buffer[offset] = value;

    return 0;
}

/* Write integer data in little-endian order */

static int write_data_value(
    uint8_t *buffer,
    uint32_t buffer_size,
    uint32_t offset,
    uint32_t value,
    uint32_t width
)
{
    if (buffer == NULL ||
        width == 0 ||
        width > 4 ||
        offset > buffer_size ||
        width > buffer_size - offset) {
        return -1;
    }

    for (uint32_t i = 0; i < width; i++) {
        buffer[offset + i] =
            (uint8_t)((value >> (i * 8)) & 0xFF);
    }

    return 0;
}

/* Check whether a value fits in .byte or .half */

static int check_data_value_range(
    int64_t value,
    uint32_t width
)
{
    if (width == 4) {
        return 0;
    }

    if (width != 1 && width != 2) {
        return -1;
    }

    uint32_t bit_count = width * 8u;
    int64_t signed_min = -((int64_t)1 << (bit_count - 1u));
    uint32_t unsigned_max = (1u << bit_count) - 1u;

    return value >= signed_min &&
           value <= (int64_t)unsigned_max ? 0 : -1;
}

/* Convert one hexadecimal digit */

static int hex_digit_value(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }

    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }

    return -1;
}

/* Decode and write a string literal */

static int write_string_literal(
    const char *text,
    uint8_t *buffer,
    uint32_t buffer_size,
    uint32_t offset,
    int add_null,
    uint32_t *written
)
{
    if (text == NULL ||
        buffer == NULL ||
        written == NULL) {
        return -1;
    }

    size_t length = strlen(text);

    if (length < 2 ||
        text[0] != '"' ||
        text[length - 1] != '"') {
        return -1;
    }

    *written = 0;

    for (size_t i = 1; i < length - 1; i++) {
        unsigned char ch = (unsigned char)text[i];

        if (ch == '"') {
            return -1;
        }

        if (ch == '\\') {
            i++;

            if (i >= length - 1) {
                return -1;
            }

            ch = (unsigned char)text[i];

            if (ch == 'x') {
                int digit;
                int digits = 0;
                unsigned int value = 0;

                while (i + 1 < length - 1 &&
                       (digit = hex_digit_value(text[i + 1])) >= 0) {
                    i++;
                    value = ((value << 4) | (unsigned int)digit) & 0xFFU;
                    digits++;
                }

                if (digits == 0) {
                    return -1;
                }

                ch = (unsigned char)value;
            }
            else if (ch >= '0' && ch <= '7') {
                unsigned int value = ch - '0';
                int digits = 1;

                while (digits < 3 &&
                       i + 1 < length - 1 &&
                       text[i + 1] >= '0' &&
                       text[i + 1] <= '7') {

                    i++;
                    value = (value << 3) |
                            (unsigned int)(text[i] - '0');
                    digits++;
                }

                ch = (unsigned char)value;
            }
            else {
                switch (ch) {
                    case 'a': ch = '\a'; break;
                    case 'b': ch = '\b'; break;
                    case 'f': ch = '\f'; break;
                    case 'n': ch = '\n'; break;
                    case 'r': ch = '\r'; break;
                    case 't': ch = '\t'; break;
                    case 'v': ch = '\v'; break;
                    case 'e': ch = 27; break;
                    case '\\': ch = '\\'; break;
                    case '"': ch = '"'; break;
                    case '\'': ch = '\''; break;
                    case '?': ch = '?'; break;

                    default:
                        return -1;
                }
            }
        }

        if (write_data_byte(
                buffer,
                buffer_size,
                offset + *written,
                (uint8_t)ch
            ) != 0) {
            return -1;
        }

        (*written)++;
    }

    if (add_null) {
        if (write_data_byte(
                buffer,
                buffer_size,
                offset + *written,
                0
            ) != 0) {
            return -1;
        }

        (*written)++;
    }

    return 0;
}

/* ========================================
   Emit Data Directives
   ======================================== */

static int emit_data_directive(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t current_address,
    uint8_t *buffer,
    uint32_t buffer_size,
    uint32_t output_offset,
    uint32_t *emitted_size
)
{
    if (parsed_line == NULL ||
        symbols == NULL ||
        buffer == NULL ||
        emitted_size == NULL) {
        return -1;
    }

    uint32_t expected_size = 0;

    if (get_directive_size(
            parsed_line,
            symbols,
            current_address,
            &expected_size
        ) != 1) {
        return -1;
    }

    if (output_offset > buffer_size ||
        expected_size > buffer_size - output_offset) {
        return -1;
    }

    const char *mnemonic = parsed_line->mnemonic;
    uint32_t cursor = 0;

    *emitted_size = 0;

    /* .byte, .half, .word */

    uint32_t width = 0;

    if (strcmp(mnemonic, ".byte") == 0) {
        width = 1;
    }
    else if (strcmp(mnemonic, ".half") == 0) {
        width = 2;
    }
    else if (strcmp(mnemonic, ".word") == 0) {
        width = 4;
    }

    if (width != 0) {
        for (int i = 0; i < parsed_line->operand_count; i++) {
            int64_t signed_value;

            if (evaluate_expression_signed(
                    parsed_line->operands[i],
                    symbols,
                    &signed_value
                ) != 0 ||
                check_data_value_range(
                signed_value,
                width
            ) != 0) {
                return -1;
            }

            uint32_t value = (uint32_t)signed_value;

            if (write_data_value(
                    buffer,
                    buffer_size,
                    output_offset + cursor,
                    value,
                    width
                ) != 0) {
                return -1;
            }

            cursor += width;
        }
    }

    /* .ascii, .asciz */

    else if (strcmp(mnemonic, ".ascii") == 0 ||
             strcmp(mnemonic, ".asciz") == 0) {

        int add_null = strcmp(mnemonic, ".asciz") == 0;

        for (int i = 0; i < parsed_line->operand_count; i++) {
            uint32_t string_size = 0;

            if (write_string_literal(
                    parsed_line->operands[i],
                    buffer,
                    buffer_size,
                    output_offset + cursor,
                    add_null,
                    &string_size
                ) != 0) {
                return -1;
            }

            cursor += string_size;
        }
    }

    /* .space */

    else if (strcmp(mnemonic, ".space") == 0) {
        uint32_t space_size;
        uint32_t fill_value = 0;

        if (evaluate_expression(
                parsed_line->operands[0],
                symbols,
                &space_size
            ) != 0) {
            return -1;
        }

        if (parsed_line->operand_count == 2) {
            if (evaluate_expression(
                    parsed_line->operands[1],
                    symbols,
                    &fill_value
                ) != 0) {
                return -1;
            }
        }

        for (uint32_t i = 0; i < space_size; i++) {
            if (write_data_byte(
                    buffer,
                    buffer_size,
                    output_offset + i,
                    (uint8_t)(fill_value & 0xFF)
                ) != 0) {
                return -1;
            }
        }

        cursor = space_size;
    }

    /* .align */

    else if (strcmp(mnemonic, ".align") == 0) {
        uint32_t fill_value = 0;

        if (parsed_line->operand_count == 2) {
            if (evaluate_expression(
                    parsed_line->operands[1],
                    symbols,
                    &fill_value
                ) != 0) {
                return -1;
            }
        }

        for (uint32_t i = 0; i < expected_size; i++) {
            if (write_data_byte(
                    buffer,
                    buffer_size,
                    output_offset + i,
                    (uint8_t)(fill_value & 0xFF)
                ) != 0) {
                return -1;
            }
        }

        cursor = expected_size;
    }
    else {
        return -1;
    }

    if (cursor != expected_size) {
        return -1;
    }

    *emitted_size = cursor;

    return 0;
}

/* ========================================
   Pass 2 Entry Point
   ======================================== */

int pass2_run(
    FILE *source,
    const SymbolTable *symbols,
    const Pass1Layout *layout,
    Pass2Result *result
)
{
    if (source == NULL ||
        symbols == NULL ||
        layout == NULL ||
        result == NULL) {
        return -1;
    }

    

    fpos_t original_position;

    if (fgetpos(source, &original_position) != 0) {
        return -1;
    }

    Pass2Result generated = {0};

    uint32_t data_start_offset = layout->text_size;

    if (layout->data_size != 0) {
        if (layout->data_base < layout->text_base) {
            return -1;
        }

        data_start_offset =
            layout->data_base - layout->text_base;

        if (data_start_offset < layout->text_size ||
            layout->data_size > UINT32_MAX - data_start_offset) {
            return -1;
        }
    }

    generated.size =
        data_start_offset + layout->data_size;

    generated.base_address = layout->text_base;

    generated.bytes = calloc(
        generated.size == 0 ? 1 : generated.size,
        sizeof(uint8_t)
    );

    if (generated.bytes == NULL) {
        return -1;
    }

    if (fseek(source, 0, SEEK_SET) != 0) {
        free(generated.bytes);
        fsetpos(source, &original_position);
        return -1;
    }

    uint32_t text_offset = 0;
    uint32_t data_offset = 0;
    uint32_t line_number = 0;

    int in_text_section = 1;
    int in_bss_section = 0;
    int data_padding_recorded = 0;
    int status;
    int success = 0;

    ParsedLine parsed_line;
    char source_line[PARSER_MAX_LINE_LENGTH];

    while (1) {
        status = read_parsed_line(
            source,
            &parsed_line,
            &line_number,
            source_line
        );

        if (status == 0) {
            break;
        }

        if (status < 0) {
            goto cleanup;
        }

        const char *mnemonic = parsed_line.mnemonic;

        if (strcmp(mnemonic, ".text") == 0) {
            in_text_section = 1;
            in_bss_section = 0;
            continue;
        }

        if (strcmp(mnemonic, ".data") == 0) {
            in_text_section = 0;
            in_bss_section = 0;

            if (!data_padding_recorded &&
                layout->data_size > 0) {

                uint32_t padding_size =
                    data_start_offset - layout->text_size;

                if (padding_size > 0) {
                    if (append_data_listing_entries(
                            &generated,
                            layout->text_base + layout->text_size,
                            &generated.bytes[layout->text_size],
                            padding_size,
                            "(padding)"
                        ) != 0) {
                        goto cleanup;
                    }
                }
            }

            data_padding_recorded = 1;
            continue;
        }

        if (strcmp(mnemonic, ".bss") == 0) {
            in_text_section = 0;
            in_bss_section = 1;
            continue;
        }

        if (strcmp(mnemonic, ".equ") == 0 ||
            strcmp(mnemonic, ".global") == 0 ||
            strcmp(mnemonic, ".globl") == 0 ||
            mnemonic[0] == '\0') {
            continue;
        }

        /* BSS occupies memory but is not stored in the binary */

        if (in_bss_section) {
            continue;
        }

        if (!in_text_section) {
            uint32_t current_address =
                layout->data_base + data_offset;

            uint32_t emitted_size = 0;

            if (emit_data_directive(
                    &parsed_line,
                    symbols,
                    current_address,
                    generated.bytes,
                    generated.size,
                    data_start_offset + data_offset,
                    &emitted_size
                ) != 0) {

                fprintf(
                    stderr,
                    "Pass 2 error at line %u: "
                    "cannot emit data directive '%s'\n",
                    line_number,
                    mnemonic
                );

                goto cleanup;
            }
            if (append_data_listing_entries(
                    &generated,
                    current_address,
                    &generated.bytes[data_start_offset + data_offset],
                    emitted_size,
                    source_line
                ) != 0) {
                goto cleanup;
            }

            data_offset += emitted_size;
            continue;
        }

        
    uint32_t current_address =
    layout->text_base + text_offset;

/* Handle .align in the text section */

if (strcmp(mnemonic, ".align") == 0) {
    uint32_t padding;

    if (get_directive_size(
            &parsed_line,
            symbols,
            current_address,
            &padding
        ) != 1 ||
        padding % 4 != 0) {

        fprintf(
            stderr,
            "Pass 2 error at line %u: invalid text alignment\n",
            line_number
        );

        goto cleanup;
    }

    for (uint32_t i = 0; i < padding; i += 4) {
        if (write_instruction(
                generated.bytes,
                generated.size,
                text_offset + i,
                0x00000000
            ) != 0) {
            goto cleanup;
        }

        const char *listing_source =
            (i == 0) ? source_line : "";

        if (append_listing_entry(
                &generated,
                current_address + i,
                0x00000000,
                4,
                listing_source
            ) != 0) {
            goto cleanup;
        }
    }

    text_offset += padding;
    continue;
}

/* Try pseudo-instruction expansion first */

uint32_t instructions[2];
uint32_t instruction_count = 0;

int pseudo_result = encode_pseudo_instruction(
    &parsed_line,
    symbols,
    current_address,
    instructions,
    &instruction_count
);

if (pseudo_result == 0) {
    /* Regular instruction */

    if (encode_base_instruction(
            &parsed_line,
            symbols,
            current_address,
            &instructions[0]
        ) != 0) {

        fprintf(
            stderr,
            "Pass 2 error at line %u: cannot encode '%s'\n",
            line_number,
            mnemonic
        );

        goto cleanup;
    }

    instruction_count = 1;
}
else if (pseudo_result < 0) {
    fprintf(
        stderr,
        "Pass 2 error at line %u: invalid pseudo-instruction '%s'\n",
        line_number,
        mnemonic
    );

    goto cleanup;
}

/* Write one or two machine instructions */
for (uint32_t i = 0; i < instruction_count; i++) {
    if (write_instruction(
            generated.bytes,
            generated.size,
            text_offset + i * 4,
            instructions[i]
        ) != 0) {
        goto cleanup;
    }

    const char *listing_source =
        (i == 0) ? source_line : "";

    if (append_listing_entry(
            &generated,
            current_address + i * 4,
            instructions[i],
            4,
            listing_source
        ) != 0) {
        goto cleanup;
    }
}

    text_offset += instruction_count * 4;
    }

    if (text_offset != layout->text_size) {
        fprintf(
            stderr,
            "Pass 2 error: generated text size (%u) "
            "does not match Pass 1 text size (%u)\n",
            text_offset,
            layout->text_size
        );

        goto cleanup;
    }

    if (data_offset != layout->data_size) {
        fprintf(
            stderr,
            "Pass 2 error: generated data size (%u) "
            "does not match Pass 1 data size (%u)\n",
            data_offset,
            layout->data_size
        );

        goto cleanup;
    }

    success = 1;

cleanup:
    if (fsetpos(source, &original_position) != 0) {
        success = 0;
    }

    if (!success) {
        free(generated.bytes);
        free(generated.listing_entries);
        return -1;
    }

    *result = generated;

    return 0;
}

/* ========================================
   Destroy Pass 2 Result
   ======================================== */

void pass2_destroy(Pass2Result *result)
{
    if (result == NULL) {
        return;
    }

    free(result->bytes);
    free(result->listing_entries);

    result->bytes = NULL;
    result->size = 0;
    result->base_address = 0;

    result->listing_entries = NULL;
    result->listing_count = 0;
    result->listing_capacity = 0;
}

/* ========================================
   Write Binary File
   ======================================== */

int pass2_write_binary(
    const Pass2Result *result,
    const char *filename
)
{
    if (result == NULL ||
        filename == NULL ||
        (result->size > 0 && result->bytes == NULL)) {
        return -1;
    }

    FILE *output = fopen(filename, "wb");

    if (output == NULL) {
        return -1;
    }

    size_t written = fwrite(
        result->bytes,
        sizeof(uint8_t),
        result->size,
        output
    );

    int close_result = fclose(output);

    if (written != result->size || close_result != 0) {
        return -1;
    }

    return 0;
}


/* ========================================
   Write Listing File
   ======================================== */

int pass2_write_listing(
    const Pass2Result *result,
    const SymbolTable *symbols,
    const char *filename
)
{
    if (result == NULL ||
        symbols == NULL ||
        filename == NULL ||
        (result->listing_count > 0 &&
         result->listing_entries == NULL) ||
        (symbols->count > 0 &&
         symbols->symbols == NULL)) {
        return -1;
    }

    FILE *output = fopen(filename, "w");

    if (output == NULL) {
        return -1;
    }

    int success = 1;

    /* Machine code and original source */

    for (size_t i = 0; i < result->listing_count; i++) {
        const Pass2ListingEntry *entry =
            &result->listing_entries[i];

        if (fprintf(
                output,
                "%08X  %08X  %s\n",
                (unsigned int)entry->address,
                (unsigned int)entry->machine_code,
                entry->source_line
            ) < 0) {
            success = 0;
            break;
        }
    }

    /* Symbol table */

    if (success) {
        if (fprintf(output, "\nSYMBOLS\n\n") < 0) {
            success = 0;
        }
    }

    for (size_t i = 0; success && i < symbols->count; i++) {
        const Symbol *symbol = &symbols->symbols[i];

        char display_type = symbol->type;

        /* Display absolute symbols as '=' */

        if (display_type == 'A') {
            display_type = '=';
        }

        if (fprintf(
                output,
                "%-16s %08X %c%s\n",
                symbol->name,
                (unsigned int)symbol->address,
                display_type,
                symbol->is_global ? " G" : ""
            ) < 0) {
            success = 0;
            break;
        }
    }

    if (fclose(output) != 0) {
        success = 0;
    }

    return success ? 0 : -1;
}
