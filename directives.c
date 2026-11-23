#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>

#include "directives.h"

// ========================================
// Expression Parser
// ========================================

typedef struct {
    const char *cursor;
    const SymbolTable *symbols;
} ExprParser;

// ========================================
// Utility Functions
// ========================================

static void skip_spaces(ExprParser *parser)
{
    while (isspace((unsigned char)*parser->cursor)) {
        parser->cursor++;
    }
}

static int is_ascii_alpha(char ch)
{
    unsigned char c = (unsigned char)ch;

    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z');
}

static int is_ascii_digit(char ch)
{
    return ch >= '0' && ch <= '9';
}

static int is_identifier_start(char ch)
{
    return is_ascii_alpha(ch) ||
           ch == '_' || ch == '.';
}

static int is_identifier_char(char ch)
{
    return is_ascii_alpha(ch) ||
           is_ascii_digit(ch) ||
           ch == '_' || ch == '.';
}

// ========================================
// Character Literal
// ========================================

static int parse_character(ExprParser *parser, int64_t *value)
{
    if (*parser->cursor != '\'') {
        return -1;
    }

    parser->cursor++;

    unsigned char ch = (unsigned char)*parser->cursor;

    if (ch == '\0') {
        return -1;
    }

    if (ch == '\\') {
        parser->cursor++;

        switch (*parser->cursor) {
            case 'n':
                ch = '\n';
                break;
            case 'r':
                ch = '\r';
                break;
            case 't':
                ch = '\t';
                break;
            case 'b':
                ch = '\b';
                break;
            case '0':
                ch = '\0';
                break;
            case '\\':
                ch = '\\';
                break;
            case '\'':
                ch = '\'';
                break;
            case '"':
                ch = '"';
                break;
            default:
                return -1;
        }
    }

    parser->cursor++;

    if (*parser->cursor != '\'') {
        return -1;
    }

    parser->cursor++;

    *value = (int64_t)ch;

    return 0;
}

// ========================================
// Primary Expression
// ========================================

static int parse_expression_or(
    ExprParser *parser,
    int64_t *value
);

static int parse_primary(
    ExprParser *parser,
    int64_t *value
)
{
    skip_spaces(parser);

    // Parentheses
    if (*parser->cursor == '(') {
        parser->cursor++;

        if (parse_expression_or(parser, value) != 0) {
            return -1;
        }

        skip_spaces(parser);

        if (*parser->cursor != ')') {
            return -1;
        }

        parser->cursor++;

        return 0;
    }

    // Character literal
    if (*parser->cursor == '\'') {
        return parse_character(parser, value);
    }

    // Number literal
    if (isdigit((unsigned char)*parser->cursor)) {

        // Binary literal: 0b... or 0B...
        if (parser->cursor[0] == '0' &&
            (parser->cursor[1] == 'b' ||
             parser->cursor[1] == 'B')) {

            const char *end = parser->cursor + 2;
            uint64_t number = 0;

            if (*end != '0' && *end != '1') {
                return -1;
            }

            while (*end == '0' || *end == '1') {
                number = number * 2u +
                         (uint64_t)(*end - '0');

                if (number > UINT32_MAX) {
                    return -1;
                }

                end++;
            }

            // Reject malformed adjacent characters such as 0b102.
            if (is_identifier_char(*end)) {
                return -1;
            }

            parser->cursor = end;
            *value = (int64_t)number;

            return 0;
        }

        // Decimal and hexadecimal literals
        int base = 10;

        // Use base 16 only for explicit hexadecimal prefixes.
        if (parser->cursor[0] == '0' &&
            (parser->cursor[1] == 'x' ||
            parser->cursor[1] == 'X')) {
            base = 16;
        }

        errno = 0;

        char *end;
        unsigned long long number = strtoull(
        parser->cursor,
            &end,
            base
        );

        if (end == parser->cursor ||
            errno == ERANGE ||
            number > UINT32_MAX) {
            return -1;
        }

        parser->cursor = end;
        *value = (int64_t)number;

        return 0;
    }

    // Symbol
    if (is_identifier_start(*parser->cursor)) {
        char name[SYMBOL_NAME_MAX];
        size_t length = 0;

        while (is_identifier_char(*parser->cursor)) {
            if (length >= sizeof(name) - 1) {
                return -1;
            }

            name[length++] = *parser->cursor;
            parser->cursor++;
        }

        name[length] = '\0';

        if (symbol_table_get_expression_value(
                parser->symbols,
                name,
                value
            ) != 0) {
            return -1;
        }

        return 0;
    }

    return -1;
}

// ========================================
// Unary Expression
// ========================================

static int parse_unary(
    ExprParser *parser,
    int64_t *value
)
{
    skip_spaces(parser);

    if (*parser->cursor == '+') {
        parser->cursor++;
        return parse_unary(parser, value);
    }

    if (*parser->cursor == '-') {
        parser->cursor++;

        if (parse_unary(parser, value) != 0 ||
            *value == INT64_MIN) {
            return -1;
        }

        *value = -(*value);
        return 0;
    }

    if (*parser->cursor == '~') {
        parser->cursor++;

        if (parse_unary(parser, value) != 0) {
            return -1;
        }

        *value = (int64_t)(uint32_t)(~(uint32_t)*value);
        return 0;
    }

    return parse_primary(parser, value);
}

// ========================================
// Multiplicative Expression
// * / %
// ========================================

static int checked_add_int64(int64_t a, int64_t b, int64_t *result)
{
    if ((b > 0 && a > INT64_MAX - b) ||
        (b < 0 && a < INT64_MIN - b)) {
        return -1;
    }

    *result = a + b;
    return 0;
}

static int checked_sub_int64(int64_t a, int64_t b, int64_t *result)
{
    if ((b < 0 && a > INT64_MAX + b) ||
        (b > 0 && a < INT64_MIN + b)) {
        return -1;
    }

    *result = a - b;
    return 0;
}

static int checked_mul_int64(int64_t a, int64_t b, int64_t *result)
{
    if (a == 0 || b == 0) {
        *result = 0;
        return 0;
    }

    if (a == -1 && b == INT64_MIN) return -1;
    if (b == -1 && a == INT64_MIN) return -1;

    if (a > 0) {
        if (b > 0 && a > INT64_MAX / b) return -1;
        if (b < 0 && b < INT64_MIN / a) return -1;
    }
    else {
        if (b > 0 && a < INT64_MIN / b) return -1;
        if (b < 0 && a < INT64_MAX / b) return -1;
    }

    *result = a * b;
    return 0;
}

static int parse_multiplicative(
    ExprParser *parser,
    int64_t *value
)
{
    if (parse_unary(parser, value) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(parser);

        char op = *parser->cursor;

        if (op != '*' && op != '/' && op != '%') {
            break;
        }

        parser->cursor++;

        int64_t right;

        if (parse_unary(parser, &right) != 0) {
            return -1;
        }

        if ((op == '/' || op == '%') && right == 0) {
            return -1;
        }

        if ((op == '/' || op == '%') &&
            *value == INT64_MIN && right == -1) {
            return -1;
        }

        if (op == '*') {
            if (checked_mul_int64(*value, right, value) != 0) {
                return -1;
            }
        }
        else if (op == '/') {
            *value /= right;
        }
        else {
            *value %= right;
        }
    }

    return 0;
}

// ========================================
// Additive Expression
// + -
// ========================================

static int parse_additive(
    ExprParser *parser,
    int64_t *value
)
{
    if (parse_multiplicative(parser, value) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(parser);

        char op = *parser->cursor;

        if (op != '+' && op != '-') {
            break;
        }

        parser->cursor++;

        int64_t right;

        if (parse_multiplicative(parser, &right) != 0) {
            return -1;
        }

        if (op == '+') {
            if (checked_add_int64(*value, right, value) != 0) {
                return -1;
            }
        }
        else {
            if (checked_sub_int64(*value, right, value) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

// ========================================
// Shift Expression
// << >>
// ========================================

static int parse_shift(
    ExprParser *parser,
    int64_t *value
)
{
    if (parse_additive(parser, value) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(parser);

        int is_left = 0;

        if (strncmp(parser->cursor, "<<", 2) == 0) {
            is_left = 1;
        }
        else if (strncmp(parser->cursor, ">>", 2) == 0) {
            is_left = 0;
        }
        else {
            break;
        }

        parser->cursor += 2;

        int64_t right;

        if (parse_additive(parser, &right) != 0 ||
            right < 0 || right >= 32) {
            return -1;
        }

        uint32_t bits = (uint32_t)*value;

        if (is_left) {
            bits <<= (unsigned int)right;
        }
        else {
            bits >>= (unsigned int)right;
        }

        *value = (int64_t)bits;
    }

    return 0;
}

// ========================================
// Bitwise AND
// ========================================

static int parse_expression_and(
    ExprParser *parser,
    int64_t *value
)
{
    if (parse_shift(parser, value) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(parser);

        if (*parser->cursor != '&' ||
            parser->cursor[1] == '&') {
            break;
        }

        parser->cursor++;

        int64_t right;

        if (parse_shift(parser, &right) != 0) {
            return -1;
        }

        *value = (int64_t)(
            (uint32_t)*value & (uint32_t)right
        );
    }

    return 0;
}

// ========================================
// Bitwise XOR
// ========================================

static int parse_expression_xor(
    ExprParser *parser,
    int64_t *value
)
{
    if (parse_expression_and(parser, value) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(parser);

        if (*parser->cursor != '^') {
            break;
        }

        parser->cursor++;

        int64_t right;

        if (parse_expression_and(parser, &right) != 0) {
            return -1;
        }

        *value = (int64_t)(
            (uint32_t)*value ^ (uint32_t)right
        );
    }

    return 0;
}

// ========================================
// Bitwise OR
// ========================================

static int parse_expression_or(
    ExprParser *parser,
    int64_t *value
)
{
    if (parse_expression_xor(parser, value) != 0) {
        return -1;
    }

    while (1) {
        skip_spaces(parser);

        if (*parser->cursor != '|' ||
            parser->cursor[1] == '|') {
            break;
        }

        parser->cursor++;

        int64_t right;

        if (parse_expression_xor(parser, &right) != 0) {
            return -1;
        }

        *value = (int64_t)(
            (uint32_t)*value | (uint32_t)right
        );
    }

    return 0;
}

// ========================================
// Evaluate Expression
// ========================================

int evaluate_expression_signed(
    const char *expression,
    const SymbolTable *symbols,
    int64_t *value
)
{
    if (expression == NULL ||
        symbols == NULL ||
        value == NULL) {
        return -1;
    }

    ExprParser parser = {
        .cursor = expression,
        .symbols = symbols
    };

    if (parse_expression_or(&parser, value) != 0) {
        return -1;
    }

    skip_spaces(&parser);

    if (*parser.cursor != '\0') {
        return -1;
    }

    return 0;
}

int evaluate_li_immediate(
    const char *expression,
    const SymbolTable *symbols,
    int64_t *value
)
{
    if (value == NULL) {
        return -1;
    }

    int64_t immediate;

    if (evaluate_expression_signed(
            expression,
            symbols,
            &immediate
        ) != 0 ||
        immediate < INT32_MIN ||
        immediate > (int64_t)UINT32_MAX) {
        return -1;
    }

    /* li accepts a 32-bit bit pattern as well as a signed integer. */
    if (immediate > INT32_MAX) {
        immediate -= (int64_t)UINT32_MAX + 1;
    }

    *value = immediate;
    return 0;
}

int evaluate_expression(
    const char *expression,
    const SymbolTable *symbols,
    uint32_t *value
)
{
    if (value == NULL) {
        return -1;
    }

    int64_t signed_value;

    if (evaluate_expression_signed(
            expression,
            symbols,
            &signed_value
        ) != 0) {
        return -1;
    }

    *value = (uint32_t)signed_value;

    return 0;
}

// ========================================
// Process .equ Directive
// ========================================

static int is_valid_symbol_name(const char *name)
{
    return symbol_table_is_valid_name(name);
}

int process_equ_directive(
    const ParsedLine *parsed_line,
    SymbolTable *symbols
)
{
    if (parsed_line == NULL || symbols == NULL) {
        return -1;
    }

    if (strcmp(parsed_line->mnemonic, ".equ") != 0) {
        return 0;
    }

    if (parsed_line->operand_count != 2) {
        return -1;
    }

    const char *name = parsed_line->operands[0];
    const char *expression = parsed_line->operands[1];

    if (!is_valid_symbol_name(name)) {
        return -1;
    }

    int64_t value;

    if (evaluate_expression_signed(
            expression,
            symbols,
            &value
        ) != 0) {
        return -1;
    }

    if (symbol_table_add_expression_value(
            symbols,
            name,
            value
        ) != 0) {
        return -1;
    }

    return 1;
}

// ========================================
// String Literal Length
// ========================================

static int get_string_length(
    const char *text,
    uint32_t *length
)
{
    if (text == NULL || length == NULL) {
        return -1;
    }

    size_t total_length = strlen(text);

    if (total_length < 2 ||
        text[0] != '"' ||
        text[total_length - 1] != '"') {
        return -1;
    }

    uint32_t count = 0;

    for (size_t i = 1; i < total_length - 1; i++) {
        unsigned char ch = (unsigned char)text[i];

        if (ch == '"') {
            return -1;
        }

        if (ch == '\\') {
            i++;

            if (i >= total_length - 1) {
                return -1;
            }

            ch = (unsigned char)text[i];

            // Hexadecimal escape
            if (ch == 'x') {
                int digits = 0;

                while (i + 1 < total_length - 1 &&
                       isxdigit((unsigned char)text[i + 1])) {
                    i++;
                    digits++;
                }

                if (digits == 0) {
                    return -1;
                }
            }
            // Octal escape
            else if (ch >= '0' && ch <= '7') {
                int digits = 1;

                while (digits < 3 &&
                       i + 1 < total_length - 1 &&
                       text[i + 1] >= '0' &&
                       text[i + 1] <= '7') {
                    i++;
                    digits++;
                }
            }
            // Ordinary escape
            else if (strchr(
                         "abfnrtv\\\"'?e",
                         ch
                     ) == NULL) {
                return -1;
            }
        }

        count++;
    }

    *length = count;

    return 0;
}

// ========================================
// Data Directive Size
// ========================================

int get_directive_size(
    const ParsedLine *parsed_line,
    const SymbolTable *symbols,
    uint32_t current_address,
    uint32_t *size
)
{
    if (parsed_line == NULL ||
        symbols == NULL ||
        size == NULL) {
        return -1;
    }

    *size = 0;

    const char *mnemonic = parsed_line->mnemonic;

    
    // ------------------------------------
    // .byte, .half, .word
    // ------------------------------------

    int element_size = 0;

    if (strcmp(mnemonic, ".byte") == 0) {
        element_size = 1;
    }
    else if (strcmp(mnemonic, ".half") == 0) {
        element_size = 2;
    }
    else if (strcmp(mnemonic, ".word") == 0) {
        element_size = 4;
    }

    if (element_size != 0) {
        if (parsed_line->operand_count == 0) {
            return -1;
        }

        /* .half must start at a 2-byte aligned address */
        if (element_size == 2 && (current_address & 1u) != 0) {
            return -1;
        }

        /*
         * .word values are evaluated in Pass 2,
         * after all labels have been registered.
         */
        if (element_size != 4) {
            for (int i = 0; i < parsed_line->operand_count; i++) {
                uint32_t value;

                if (evaluate_expression(
                        parsed_line->operands[i],
                        symbols,
                        &value
                    ) != 0) {
                    return -1;
                }
            }
        }

        *size = (uint32_t)parsed_line->operand_count *
                (uint32_t)element_size;

        return 1;
    }


    // ------------------------------------
    // .ascii, .asciz
    // ------------------------------------

    if (strcmp(mnemonic, ".ascii") == 0 ||
        strcmp(mnemonic, ".asciz") == 0) {

        if (parsed_line->operand_count == 0) {
            return -1;
        }

        int add_null = strcmp(mnemonic, ".asciz") == 0;

        for (int i = 0; i < parsed_line->operand_count; i++) {
            uint32_t string_length;

            if (get_string_length(
                    parsed_line->operands[i],
                    &string_length
                ) != 0) {
                return -1;
            }

            *size += string_length;

            if (add_null) {
                (*size)++;
            }
        }

        return 1;
    }

    // ------------------------------------
    // .space
    // ------------------------------------

    if (strcmp(mnemonic, ".space") == 0) {
        if (parsed_line->operand_count < 1 ||
            parsed_line->operand_count > 2) {
            return -1;
        }

        uint32_t space_size;

        if (evaluate_expression(
                parsed_line->operands[0],
                symbols,
                &space_size
            ) != 0) {
            return -1;
        }

        // Validate optional fill value
        if (parsed_line->operand_count == 2) {
            uint32_t fill_value;

            if (evaluate_expression(
                    parsed_line->operands[1],
                    symbols,
                    &fill_value
                ) != 0) {
                return -1;
            }
        }

        *size = space_size;

        return 1;
    }

    // ------------------------------------
    // .align
    // ------------------------------------

    if (strcmp(mnemonic, ".align") == 0) {
        if (parsed_line->operand_count < 1 ||
            parsed_line->operand_count > 2) {
            return -1;
        }

        uint32_t exponent;

        if (evaluate_expression(
                parsed_line->operands[0],
                symbols,
                &exponent
            ) != 0) {
            return -1;
        }

        if (exponent > 31) {
            return -1;
        }

        // Validate optional fill value
        if (parsed_line->operand_count == 2) {
            uint32_t fill_value;

            if (evaluate_expression(
                    parsed_line->operands[1],
                    symbols,
                    &fill_value
                ) != 0) {
                return -1;
            }
        }

        uint32_t alignment = 1U << exponent;
        uint32_t mask = alignment - 1;

        *size = (0U - current_address) & mask;

        return 1;
    }

    // Not a data-size directive
    return 0;
}
