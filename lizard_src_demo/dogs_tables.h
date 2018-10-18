
// init table
static void (* const dog_init_table[DOG_COUNT])(uint8) = {
#define TABLE_ENTRY(d) dog_init_ ## d,
#include "dogs_table.h"
#undef TABLE_ENTRY
};

// tick table
static void (* const dog_tick_table[DOG_COUNT])(uint8) = {
#define TABLE_ENTRY(d) dog_tick_ ## d,
#include "dogs_table.h"
#undef TABLE_ENTRY
};

// draw table
static void (* const dog_draw_table[DOG_COUNT])(uint8) = {
#define TABLE_ENTRY(d) dog_draw_ ## d,
#include "dogs_table.h"
#undef TABLE_ENTRY
};

// end of file