#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "symbol_table.h"

// ========================================
// Utility functions
// ========================================

static void trim(char *text)
{
    char *start = text;

    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (start != text) {
        memmove(text, start, strlen(start) + 1);
    }

    size_t length = strlen(text);

    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[length - 1] = '\0';
        length--;
    }
}

static int copy_field(
    char *destination,
    size_t capacity,
    const char *source,
    size_t length
)
{
    if (length >= capacity) {
        return -1;
    }

    memcpy(destination, source, length);
    destination[length] = '\0';

    return 0;
}
// ========================================
// Remove comments
// ========================================

static void remove_comments(char *line)
{
    int in_string = 0;
    int escaped = 0;

    for (int i = 0; line[i] != '\0'; i++) {
        char ch = line[i];

        if (in_string) {
            if (escaped) {
                escaped = 0;
            }
            else if (ch == '\\') {
                escaped = 1;
            }
            else if (ch == '"') {
                in_string = 0;
            }
        }
        else {
            if (ch == '"') {
                in_string = 1;
            }
            else if (ch == '#') {
                line[i] = '\0';
                break;
            }
            else if (ch == '/' && line[i + 1] == '/') {
                line[i] = '\0';
                break;
            }
        }
    }
}

// ========================================
// Parse operands
// ========================================

static int parse_operands(
    char *text,
    ParsedLine *parsed_line
)
{
    char *start = text;

    int parenthesis_depth = 0;
    int bracket_depth = 0;
    int brace_depth = 0;

    int in_string = 0;
    int escaped = 0;

    for (char *cursor = text; ; cursor++) {
        char ch = *cursor;

        if (in_string) {
            if (escaped) {
                escaped = 0;
            }
            else if (ch == '\\') {
                escaped = 1;
            }
            else if (ch == '"') {
                in_string = 0;
            }
        }
        else {
            if (ch == '"') {
                in_string = 1;
            }
            else if (ch == '(') {
                parenthesis_depth++;
            }
            else if (ch == ')') {
                parenthesis_depth--;

                if (parenthesis_depth < 0) {
                    return -1;
                }
            }
            else if (ch == '[') {
                bracket_depth++;
            }
            else if (ch == ']') {
                bracket_depth--;

                if (bracket_depth < 0) {
                    return -1;
                }
            }
            else if (ch == '{') {
                brace_depth++;
            }
            else if (ch == '}') {
                brace_depth--;

                if (brace_depth < 0) {
                    return -1;
                }
            }
        }

        int delimiter = 0;

        if (ch == '\0') {
            if (in_string ||
                parenthesis_depth != 0 ||
                bracket_depth != 0 ||
                brace_depth != 0) {
                return -1;
            }

            delimiter = 1;
        }
        else if (ch == ',' &&
                 !in_string &&
                 parenthesis_depth == 0 &&
                 bracket_depth == 0 &&
                 brace_depth == 0) {
            delimiter = 1;
        }

        if (delimiter) {
            size_t length = (size_t)(cursor - start);

            char operand[PARSER_MAX_OPERAND_LENGTH];

            if (copy_field(
                    operand,
                    sizeof(operand),
                    start,
                    length
                ) != 0) {
                return -1;
            }

            trim(operand);

            if (operand[0] == '\0') {
                return -1;
            }

            if (parsed_line->operand_count >= PARSER_MAX_OPERANDS) {
                return -1;
            }

            strcpy(
                parsed_line->operands[parsed_line->operand_count],
                operand
            );

            parsed_line->operand_count++;

            if (ch == '\0') {
                break;
            }

            start = cursor + 1;
        }

        if (ch == '\0') {
            break;
        }
    }

    return 0;
}

// ========================================
// Parse one assembly line
// ========================================

int parse_line(
    const char *line,
    ParsedLine *parsed_line
)
{
    if (line == NULL || parsed_line == NULL) {
        return -1;
    }

    memset(parsed_line, 0, sizeof(ParsedLine));

    char buffer[PARSER_MAX_LINE_LENGTH];

    size_t length = strlen(line);

    if (length >= sizeof(buffer)) {
        return -1;
    }

    strcpy(buffer, line);

    remove_comments(buffer);
    trim(buffer);

    // Empty line or comment line
    if (buffer[0] == '\0') {
        return 0;
    }

    char *position = buffer;

    // ------------------------------------
    // Parse label
    // ------------------------------------

    char *token_end = position;

    while (*token_end != '\0' &&
           !isspace((unsigned char)*token_end)) {
        token_end++;
    }

    char *colon = memchr(
        position,
        ':',
        (size_t)(token_end - position)
    );

    if (colon != NULL) {
        size_t label_length = (size_t)(colon - position);

        if (label_length == 0) {
            return -1;
        }

        if (copy_field(
                parsed_line->label,
                sizeof(parsed_line->label),
                position,
                label_length
            ) != 0) {
            return -1;
        }

        if (!symbol_table_is_valid_name(parsed_line->label)) {
            return -1;
        }

        position = colon + 1;

        while (isspace((unsigned char)*position)) {
            position++;
        }

        // Label only
        if (*position == '\0') {
            return 0;
        }
    }

    // ------------------------------------
    // Parse mnemonic
    // ------------------------------------

    token_end = position;

    while (*token_end != '\0' &&
           !isspace((unsigned char)*token_end)) {
        token_end++;
    }

    size_t mnemonic_length = (size_t)(token_end - position);

    if (mnemonic_length == 0) {
        return 0;
    }

    if (copy_field(
            parsed_line->mnemonic,
            sizeof(parsed_line->mnemonic),
            position,
            mnemonic_length
        ) != 0) {
        return -1;
    }

    position = token_end;

    while (isspace((unsigned char)*position)) {
        position++;
    }

    // Instruction or directive without operands
    if (*position == '\0') {
        return 0;
    }

    // Parse operands
    if (parse_operands(position, parsed_line) != 0) {
        return -1;
    }

    return 0;
}