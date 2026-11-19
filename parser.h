#ifndef PARSER_H
#define PARSER_H

#define PARSER_MAX_LINE_LENGTH 1024
#define PARSER_MAX_LABEL_LENGTH 128
#define PARSER_MAX_MNEMONIC_LENGTH 64
#define PARSER_MAX_OPERANDS 16
#define PARSER_MAX_OPERAND_LENGTH 256

typedef struct {
    char label[PARSER_MAX_LABEL_LENGTH];
    char mnemonic[PARSER_MAX_MNEMONIC_LENGTH];

    int operand_count;

    char operands[PARSER_MAX_OPERANDS][PARSER_MAX_OPERAND_LENGTH];
} ParsedLine;

int parse_line(const char *line, ParsedLine *parsed_line);

#endif