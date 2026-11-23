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


/* ========================================
   Symbol Name Validation
   ======================================== */

int symbol_table_is_valid_name(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    size_t length = 0;

    unsigned char first = (unsigned char)name[0];

    int valid_first =
        (first >= 'A' && first <= 'Z') ||
        (first >= 'a' && first <= 'z') ||
        first == '_' ||
        first == '.';

    if (!valid_first) {
        return 0;
    }

    for (size_t i = 0; name[i] != '\0'; i++) {
        unsigned char c = (unsigned char)name[i];

        int valid_char =
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_' ||
            c == '.';

        if (!valid_char) {
            return 0;
        }

        length++;

        if (length >= SYMBOL_NAME_MAX) {
            return 0;
        }
    }

    return 1;
}


/* ========================================
   Find Symbol
   ======================================== */

static Symbol *find_symbol(
    SymbolTable *table,
    const char *name
)
{
    if (table == NULL || name == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }

    return NULL;
}

/* ========================================
   Expand Symbol Table
   ======================================== */

static int expand_symbol_table(SymbolTable *table)
{
    if (table->count < table->capacity) {
        return 0;
    }

    size_t new_capacity;

    if (table->capacity == 0) {
        new_capacity = 16;
    }
    else {
        new_capacity = table->capacity * 2;

        if (new_capacity < table->capacity) {
            return -1;
        }
    }

    if (new_capacity > (size_t)-1 / sizeof(Symbol)) {
        return -1;
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

    return 0;
}

/* ========================================
   Add Typed Symbol
   ======================================== */

int symbol_table_add_typed(
    SymbolTable *table,
    const char *name,
    uint32_t address,
    char type
)
{
    if (table == NULL ||
        !symbol_table_is_valid_name(name)) {
        return -1;
    }

    if (type != 'T' &&
        type != 'D' &&
        type != 'B' &&
        type != 'A') {
        return -1;
    }

    Symbol *existing = find_symbol(table, name);

    /* Fill a previously declared global symbol */

    if (existing != NULL) {
        if (existing->defined || !existing->is_global) {
            return -1;
        }

        existing->address = address;
        existing->type = type;
        existing->defined = 1;

        return 0;
    }

    if (expand_symbol_table(table) != 0) {
        return -1;
    }

    Symbol *new_symbol = &table->symbols[table->count];

    strcpy(new_symbol->name, name);

    new_symbol->address = address;
    new_symbol->type = type;
    new_symbol->is_global = 0;
    new_symbol->defined = 1;

    table->count++;

    return 0;
}

/* ========================================
   Add Symbol
   Default type: Absolute
   ======================================== */

int symbol_table_add(
    SymbolTable *table,
    const char *name,
    uint32_t address
)
{
    return symbol_table_add_typed(
        table,
        name,
        address,
        'A'
    );
}
int symbol_table_update_address(
    SymbolTable *table,
    const char *name,
    uint32_t address
)
{
    if (table == NULL || name == NULL) {
        return -1;
    }

    Symbol *symbol = find_symbol(table, name);

    if (symbol == NULL || !symbol->defined) {
        return -1;
    }

    symbol->address = address;

    return 0;
}

/* ========================================
   Declare Global Symbol
   ======================================== */

int symbol_table_declare_global(
    SymbolTable *table,
    const char *name
)
{
    if (table == NULL ||
        !symbol_table_is_valid_name(name)) {
        return -1;
    }

    Symbol *existing = find_symbol(table, name);

    if (existing != NULL) {
        existing->is_global = 1;
        return 0;
    }

    if (expand_symbol_table(table) != 0) {
        return -1;
    }

    Symbol *new_symbol = &table->symbols[table->count];

    strcpy(new_symbol->name, name);

    new_symbol->address = 0;
    new_symbol->type = 'U';
    new_symbol->is_global = 1;
    new_symbol->defined = 0;

    table->count++;

    return 0;
}

/* ========================================
   Get Symbol Address
   ======================================== */

int symbol_table_get(
    const SymbolTable *table,
    const char *name,
    uint32_t *address
)
{
    if (table == NULL ||
        name == NULL ||
        address == NULL) {
        return -1;
    }

    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            if (!table->symbols[i].defined) {
                return -1;
            }

            *address = table->symbols[i].address;
            return 0;
        }
    }

    return -1;
}
