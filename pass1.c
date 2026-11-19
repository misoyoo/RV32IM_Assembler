
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pass1.h"
#include "parser.h"
#include "directives.h"
#include "symbol_table.h"

typedef enum {
    SECTION_NONE,
    SECTION_TEXT,
    SECTION_DATA
} Section;

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

static int is_data_directive(const char *mnemonic)
{
    static const char *const directives[] = {
        ".byte",
        ".half",
        ".word",
        ".ascii",
        ".asciz",
        ".space"
    };

    return is_one_of(
        mnemonic,
        directives,
        sizeof(directives) / sizeof(directives[0])
    );
}

static int is_global_directive(const char *mnemonic)
{
    return strcmp(mnemonic, ".global") == 0 ||
           strcmp(mnemonic, ".globl") == 0;
}

static int add_offset(uint32_t *offset, uint32_t size)
{
    if (offset == NULL || size > UINT32_MAX - *offset) {
        return -1;
    }

    *offset += size;
    return 0;
}

static int make_address(
    uint32_t base,
    uint32_t offset,
    uint32_t *address
)
{
    if (address == NULL || offset > UINT32_MAX - base) {
        return -1;
    }

    *address = base + offset;
    return 0;
}

/* ========================================
   Source Line Reading
   ======================================== */

static int read_source_line(
    FILE *source,
    char *buffer,
    size_t capacity,
    uint32_t *line_number
)
{
    if (fgets(buffer, (int)capacity, source) == NULL) {
        return ferror(source) ? -1 : 0;
    }

    (*line_number)++;

    size_t length = strlen(buffer);

    if (length > 0 &&
        buffer[length - 1] != '\n' &&
        !feof(source)) {

        int ch = fgetc(source);

        if (ch != EOF && ch != '\n') {
            while (ch != '\n' && ch != EOF) {
                ch = fgetc(source);
            }

            return -1;
        }

        if (ch == EOF && ferror(source)) {
            return -1;
        }
    }

    return 1;
}

static int read_parsed_line(
    FILE *source,
    ParsedLine *parsed_line,
    uint32_t *line_number
)
{
    char buffer[PARSER_MAX_LINE_LENGTH];

    int status = read_source_line(
        source,
        buffer,
        sizeof(buffer),
        line_number
    );

    if (status <= 0) {
        if (status < 0) {
            fprintf(
                stderr,
                "Pass 1 error at line %u: cannot read source line\n",
                *line_number
            );
        }

        return status;
    }

    if (parse_line(buffer, parsed_line) != 0) {
        fprintf(
            stderr,
            "Pass 1 error at line %u: parsing failed\n",
            *line_number
        );

        return -1;
    }

    return 1;
}

/* ========================================
   Instruction Size Calculation
   ======================================== */

static int get_instruction_size(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t *size
)
{
    if (parsed_line == NULL ||
        symbols == NULL ||
        size == NULL) {
        return -1;
    }

    const char *mnemonic = parsed_line->mnemonic;

    /* li: one or two instructions */

    if (strcmp(mnemonic, "li") == 0) {
        if (parsed_line->operand_count != 2) {
            return -1;
        }

        uint32_t immediate;

        if (evaluate_expression(
                parsed_line->operands[1],
                symbols,
                &immediate
            ) != 0) {
            return -1;
        }

        if (immediate <= 0x7FFU ||
            immediate >= 0xFFFFF800U) {
            *size = 4;
        }
        else {
            *size = 8;
        }

        return 0;
    }

    /* la: auipc + addi */

    if (strcmp(mnemonic, "la") == 0) {
        if (parsed_line->operand_count != 2) {
            return -1;
        }

        *size = 8;
        return 0;
    }

    /* call and tail: two instructions */

    if (strcmp(mnemonic, "call") == 0 ||
        strcmp(mnemonic, "tail") == 0) {
        *size = 8;
        return 0;
    }

    /* RV32I R-type and RV32M */

    static const char *const r_type[] = {
        "add", "sub", "sll", "slt", "sltu",
        "xor", "srl", "sra", "or", "and",
        "mul", "mulh", "mulhsu", "mulhu",
        "div", "divu", "rem", "remu"
    };

    /* RV32I I-type */

    static const char *const i_type[] = {
        "addi", "slti", "sltiu", "xori",
        "ori", "andi", "slli", "srli", "srai",
        "lb", "lh", "lw", "lbu", "lhu",
        "jalr"
    };

    /* RV32I S-type */

    static const char *const s_type[] = {
        "sb", "sh", "sw"
    };

    /* RV32I B-type */

    static const char *const b_type[] = {
        "beq", "bne", "blt", "bge", "bltu", "bgeu"
    };

    /* RV32I U-type and J-type */

    static const char *const uj_type[] = {
        "lui", "auipc", "jal"
    };

    /* CSR instructions and aliases */

    static const char *const csr_type[] = {
        "csrrw", "csrrs", "csrrc",
        "csrrwi", "csrrsi", "csrrci",
        "csrr", "csrw", "csrs", "csrc",
        "csrwi", "csrsi", "csrci"
    };

    /* System instructions */

    static const char *const system_type[] = {
        "ecall", "ebreak", "mret", "sret",
        "uret", "wfi", "fence", "fence.i"
    };

    /* One-instruction pseudo-instructions */

    static const char *const pseudo_type[] = {
        "j", "jr", "ret", "mv", "nop",
        "not", "neg", "seqz", "snez",
        "sltz", "sgtz",
        "beqz", "bnez", "bltz", "bgez",
        "blez", "bgtz", "ble", "bgt",
        "bleu", "bgtu"
    };

    if (is_one_of(
            mnemonic,
            r_type,
            sizeof(r_type) / sizeof(r_type[0])
        ) ||
        is_one_of(
            mnemonic,
            i_type,
            sizeof(i_type) / sizeof(i_type[0])
        ) ||
        is_one_of(
            mnemonic,
            s_type,
            sizeof(s_type) / sizeof(s_type[0])
        ) ||
        is_one_of(
            mnemonic,
            b_type,
            sizeof(b_type) / sizeof(b_type[0])
        ) ||
        is_one_of(
            mnemonic,
            uj_type,
            sizeof(uj_type) / sizeof(uj_type[0])
        ) ||
        is_one_of(
            mnemonic,
            csr_type,
            sizeof(csr_type) / sizeof(csr_type[0])
        ) ||
        is_one_of(
            mnemonic,
            system_type,
            sizeof(system_type) / sizeof(system_type[0])
        ) ||
        is_one_of(
            mnemonic,
            pseudo_type,
            sizeof(pseudo_type) / sizeof(pseudo_type[0])
        )) {

        *size = 4;
        return 0;
    }

    return -1;
}

/* ========================================
   Pass 1 - First Scan
   Calculate text section size and
   register .equ constants
   ======================================== */

static int scan_text_size(
    FILE *source,
    SymbolTable *symbols,
    uint32_t *text_size
)
{
    Section section = SECTION_NONE;
    uint32_t text_offset = 0;
    uint32_t line_number = 0;

    ParsedLine parsed_line;

    while (1) {
        int status = read_parsed_line(
            source,
            &parsed_line,
            &line_number
        );

        if (status == 0) {
            break;
        }

        if (status < 0) {
            return -1;
        }

        const char *mnemonic = parsed_line.mnemonic;

        if (strcmp(mnemonic, ".text") == 0) {
            if (parsed_line.operand_count != 0) {
                return -1;
            }

            section = SECTION_TEXT;
            continue;
        }

        if (strcmp(mnemonic, ".data") == 0) {
            if (parsed_line.operand_count != 0) {
                return -1;
            }

            section = SECTION_DATA;
            continue;
        }

        if (strcmp(mnemonic, ".equ") == 0) {
            if (process_equ_directive(
                    &parsed_line,
                    symbols
                ) != 1) {
                fprintf(
                    stderr,
                    "Pass 1 error at line %u: invalid .equ\n",
                    line_number
                );

                return -1;
            }

            continue;
        }

        if (is_global_directive(mnemonic)) {
            if (parsed_line.operand_count < 1) {
                return -1;
            }

            continue;
        }

        if (mnemonic[0] == '\0') {
            continue;
        }

        if (section == SECTION_DATA) {
            if (is_data_directive(mnemonic) ||
                strcmp(mnemonic, ".align") == 0) {
                continue;
            }

            fprintf(
                stderr,
                "Pass 1 error at line %u: invalid directive in .data\n",
                line_number
            );

            return -1;
        }

        if (section != SECTION_TEXT) {
            fprintf(
                stderr,
                "Pass 1 error at line %u: instruction outside a section\n",
                line_number
            );

            return -1;
        }

        uint32_t current_address;
        uint32_t size;

        if (make_address(
                DEFAULT_TEXT_BASE,
                text_offset,
                &current_address
            ) != 0) {
            return -1;
        }

        if (strcmp(mnemonic, ".align") == 0) {
            int result = get_directive_size(
                &parsed_line,
                symbols,
                current_address,
                &size
            );

            if (result != 1) {
                fprintf(
                    stderr,
                    "Pass 1 error at line %u: invalid .align\n",
                    line_number
                );

                return -1;
            }
        }
        else {
            if (is_data_directive(mnemonic) ||
                get_instruction_size(
                    &parsed_line,
                    symbols,
                    &size
                ) != 0) {
                fprintf(
                    stderr,
                    "Pass 1 error at line %u: unknown or invalid instruction '%s'\n",
                    line_number,
                    mnemonic
                );

                return -1;
            }
        }

        if (add_offset(&text_offset, size) != 0) {
            return -1;
        }
    }

    *text_size = text_offset;

    return 0;
}

/* ========================================
   Pass 1 - Second Scan
   Register labels and calculate data size
   ======================================== */

static int scan_symbols(
    FILE *source,
    SymbolTable *symbols,
    Pass1Layout *layout
)
{
    Section section = SECTION_NONE;

    uint32_t text_offset = 0;
    uint32_t data_offset = 0;
    uint32_t line_number = 0;

    ParsedLine parsed_line;

    while (1) {
        int status = read_parsed_line(
            source,
            &parsed_line,
            &line_number
        );

        if (status == 0) {
            break;
        }

        if (status < 0) {
            return -1;
        }

        const char *mnemonic = parsed_line.mnemonic;

        /* Register label before processing its instruction */

        if (parsed_line.label[0] != '\0') {
            uint32_t label_address;

            if (section == SECTION_TEXT) {
                if (make_address(
                        layout->text_base,
                        text_offset,
                        &label_address
                    ) != 0) {
                    return -1;
                }
            }
            else if (section == SECTION_DATA) {
                if (make_address(
                        layout->data_base,
                        data_offset,
                        &label_address
                    ) != 0) {
                    return -1;
                }
            }
            else {
                fprintf(
                    stderr,
                    "Pass 1 error at line %u: label outside a section\n",
                    line_number
                );

                return -1;
            }

            if (symbol_table_add(
                    symbols,
                    parsed_line.label,
                    label_address
                ) != 0) {
                fprintf(
                    stderr,
                    "Pass 1 error at line %u: duplicate or invalid label '%s'\n",
                    line_number,
                    parsed_line.label
                );

                return -1;
            }
        }

        if (strcmp(mnemonic, ".text") == 0) {
            section = SECTION_TEXT;
            continue;
        }

        if (strcmp(mnemonic, ".data") == 0) {
            section = SECTION_DATA;
            continue;
        }

        if (strcmp(mnemonic, ".equ") == 0 ||
            is_global_directive(mnemonic) ||
            mnemonic[0] == '\0') {
            continue;
        }

        uint32_t current_address;
        uint32_t size;

        if (section == SECTION_TEXT) {
            if (make_address(
                    layout->text_base,
                    text_offset,
                    &current_address
                ) != 0) {
                return -1;
            }

            if (strcmp(mnemonic, ".align") == 0) {
                if (get_directive_size(
                        &parsed_line,
                        symbols,
                        current_address,
                        &size
                    ) != 1) {
                    return -1;
                }
            }
            else {
                if (is_data_directive(mnemonic) ||
                    get_instruction_size(
                        &parsed_line,
                        symbols,
                        &size
                    ) != 0) {
                    fprintf(
                        stderr,
                        "Pass 1 error at line %u: invalid .text statement\n",
                        line_number
                    );

                    return -1;
                }
            }

            if (add_offset(&text_offset, size) != 0) {
                return -1;
            }
        }
        else if (section == SECTION_DATA) {
            if (make_address(
                    layout->data_base,
                    data_offset,
                    &current_address
                ) != 0) {
                return -1;
            }

            int result = get_directive_size(
                &parsed_line,
                symbols,
                current_address,
                &size
            );

            if (result != 1) {
                fprintf(
                    stderr,
                    "Pass 1 error at line %u: invalid .data directive '%s'\n",
                    line_number,
                    mnemonic
                );

                return -1;
            }

            if (add_offset(&data_offset, size) != 0) {
                return -1;
            }
        }
        else {
            fprintf(
                stderr,
                "Pass 1 error at line %u: statement outside a section\n",
                line_number
            );

            return -1;
        }
    }

    if (text_offset != layout->text_size) {
        fprintf(
            stderr,
            "Pass 1 error: text size mismatch\n"
        );

        return -1;
    }

    layout->data_size = data_offset;

    return 0;
}

/* ========================================
   Pass 1 Entry Point
   ======================================== */

int pass1_run(
    FILE *source,
    SymbolTable *symbols,
    Pass1Layout *layout
)
{
    if (source == NULL ||
        symbols == NULL ||
        layout == NULL) {
        return -1;
    }

    /*
     * The caller must initialize an empty SymbolTable
     * before calling pass1_run().
     */

    if (symbols->symbols != NULL ||
        symbols->count != 0 ||
        symbols->capacity != 0) {
        return -1;
    }

    fpos_t original_position;

    if (fgetpos(source, &original_position) != 0) {
        return -1;
    }

    SymbolTable work_symbols;

    if (symbol_table_init(&work_symbols) != 0) {
        return -1;
    }

    if (symbol_table_add(
        &work_symbols,
        "_stack_top",
        0x04000000u
    ) != 0) {

        symbol_table_destroy(&work_symbols);
        return -1;
    }

    Pass1Layout result_layout = {
        .text_base = DEFAULT_TEXT_BASE,
        .text_size = 0,
        .data_base = 0,
        .data_size = 0
    };

    int result = -1;

    /* First scan: calculate text size and process .equ */

    if (fseek(source, 0, SEEK_SET) != 0) {
        goto cleanup;
    }

    if (scan_text_size(
            source,
            &work_symbols,
            &result_layout.text_size
        ) != 0) {
        goto cleanup;
    }

    /* Data section starts immediately after text section */

    if (make_address(
            result_layout.text_base,
            result_layout.text_size,
            &result_layout.data_base
        ) != 0) {
        goto cleanup;
    }

    /* Second scan: register labels and calculate data size */

    if (fseek(source, 0, SEEK_SET) != 0) {
        goto cleanup;
    }

    if (scan_symbols(
            source,
            &work_symbols,
            &result_layout
        ) != 0) {
        goto cleanup;
    }

    if (fsetpos(source, &original_position) != 0) {
        goto cleanup;
    }

    *symbols = work_symbols;
    *layout = result_layout;

    return 0;

cleanup:
    symbol_table_destroy(&work_symbols);
    fsetpos(source, &original_position);

    return result;
}
