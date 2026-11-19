#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stddef.h>
#include <stdint.h>

#define SYMBOL_NAME_MAX 128

typedef struct {
    char name[SYMBOL_NAME_MAX];
    uint32_t address;
} Symbol;

typedef struct {
    Symbol *symbols;
    size_t count;
    size_t capacity;
} SymbolTable;

int symbol_table_init(SymbolTable *table);

void symbol_table_destroy(SymbolTable *table);

int symbol_table_add(
    SymbolTable *table,
    const char *name,
    uint32_t address
);

int symbol_table_get(
    const SymbolTable *table,
    const char *name,
    uint32_t *address
);

#endif