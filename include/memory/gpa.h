#pragma once


#include "attributes.h"
#include "intdefs.h"


/// Returns a pointer to the main arena used for general allocations
PURE_FUNC
struct ArenaHeap* gpa_main(void);

/// Arena used for allocating strings. each entry is null-terminated
PURE_FUNC
struct ArenaHeap* gpa_string(void);


void memory_init(isize initial_capacity);

void memory_free(void);

void* arena_calloc(isize size, isize count, isize align);
