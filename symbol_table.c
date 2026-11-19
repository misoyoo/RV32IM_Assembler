#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"

// ========================================
// Initialize Symbol Table
// ========================================

int symbol_table_init(SymbolTable *table)
{
    if (table == NULL) {
        return -1;
    }

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;

    return 0;
}

// ========================================
// Destroy Symbol Table
// ========================================

void symbol_table_destroy(SymbolTable *table)
{
    if (table == NULL) {
        return;
    }

    free(table->symbols);

    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

// ========================================
// Add Symbol
// ========================================

int symbol_table_add(
    SymbolTable *table,
    const char *name,
    uint32_t address
)
{
    if (table == NULL || name == NULL || name[0] == '\0') {
        return -1;
    }

    if (strlen(name) >= SYMBOL_NAME_MAX) {
        return -1;
    }

    uint32_t existing_address;

    if (symbol_table_get(table, name, &existing_address) == 0) {
        return -1;
    }

    // Expand capacity when full
    if (table->count == table->capacity) {
        size_t new_capacity;

        if (table->capacity == 0) {
            new_capacity = 16;
        }
        else {
            new_capacity = table->capacity * 2;
        }

        Symbol *new_symbols = realloc(
            table->symbols,
            new_capacity * sizeof(Symbol)
        );

        if (new_symbols == NULL) {
            return -1;
        }

        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }

    // Store new symbol
    strcpy(
        table->symbols[table->count].name,
        name
    );

    table->symbols[table->count].address = address;

    table->count++;

    return 0;
}

// ========================================
// Get Symbol Address
// ========================================

int symbol_table_get(
    const SymbolTable *table,
    const char *name,
    uint32_t *address
)
{
    if (table == NULL || name == NULL || address == NULL) {
        return -1;
    }

    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            *address = table->symbols[i].address;
            return 0;
        }
    }

    return -1;
}