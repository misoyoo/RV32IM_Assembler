#include <stdio.h>
#include <stdint.h>

#include "symbol_table.h"

int main(void)
{
    SymbolTable table;
    uint32_t address;

    if (symbol_table_init(&table) != 0) {
        printf("Symbol Table initialization failed!\n");
        return 1;
    }

    printf("===== Symbol Table Test =====\n");

    // Add symbols
    symbol_table_add(&table, "_start", 0x1000);
    symbol_table_add(&table, "LOOP", 0x1010);
    symbol_table_add(&table, "WAIT", 0x1020);

    // Search existing symbols
    if (symbol_table_get(&table, "_start", &address) == 0) {
        printf("_start = 0x%08X\n", (unsigned int)address);
    }

    if (symbol_table_get(&table, "LOOP", &address) == 0) {
        printf("LOOP = 0x%08X\n", (unsigned int)address);
    }

    if (symbol_table_get(&table, "WAIT", &address) == 0) {
        printf("WAIT = 0x%08X\n", (unsigned int)address);
    }

    // Duplicate symbol test
    if (symbol_table_add(&table, "LOOP", 0x2000) != 0) {
        printf("Duplicate symbol detected!\n");
    }

    // Non-existent symbol test
    if (symbol_table_get(&table, "UNKNOWN", &address) != 0) {
        printf("Unknown symbol detected!\n");
    }

    symbol_table_destroy(&table);

    return 0;
}