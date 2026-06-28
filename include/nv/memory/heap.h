#pragma once

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/memory/vmem.h"

/// @brief first-fit, size-segragated block allocator
typedef struct Heap Heap;

Heap* heap_new(isize vmem_size);

PARAMS_NONNULL(1)
Heap* heap_from_vmem(VMem* vm);

[[gnu::alloc_size(2)]] [[gnu::alloc_align(3)]]
void* heap_alloc(Heap* self, isize size, isize align) METHOD;

[[gnu::alloc_size(2)]] [[gnu::alloc_align(3)]]
void* heap_zalloc(Heap* self, isize size, isize align) METHOD;

[[gnu::alloc_size(3)]]
void* heap_realloc(Heap* self, void* ptr, isize new_size) METHOD;

void* heap_reallocate(Heap* self, Layout old, Layout new) METHOD;

bool heap_resize(Heap* self, void* ptr, Layout old, Layout new) METHOD;

void heap_free(Heap* self, void* ptr) METHOD;

static inline void* heap_allocate(Heap* self, Layout layout) { return heap_alloc(self, layout.size, layout.align); }

void heap_destroy(Heap* self);
