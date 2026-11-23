#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stddef.h>
#include <stdint.h>

#define SYMBOL_NAME_MAX 128

typedef struct {
    char name[SYMBOL_NAME_MAX];
    uint32_t address;
    int64_t expression_value;

    char type;
    int is_global;
    int defined;
} Symbol;

typedef struct {
    Symbol *symbols;
    size_t count;
    size_t capacity;
} SymbolTable;

int symbol_table_init(SymbolTable *table);

int symbol_table_is_valid_name(const char *name);

void symbol_table_destroy(SymbolTable *table);

int symbol_table_add_expression_value(
    SymbolTable *table,
    const char *name,
    int64_t value
);

int symbol_table_get_expression_value(
    const SymbolTable *table,
    const char *name,
    int64_t *value
);

int symbol_table_add(
    SymbolTable *table,
    const char *name,
    uint32_t address
);
int symbol_table_update_address(
    SymbolTable *table,
    const char *name,
    uint32_t address
);
int symbol_table_add_typed(
    SymbolTable *table,
    const char *name,
    uint32_t address,
    char type
);

int symbol_table_declare_global(
    SymbolTable *table,
    const char *name
);

int symbol_table_get(
    const SymbolTable *table,
    const char *name,
    uint32_t *address
);

#endif