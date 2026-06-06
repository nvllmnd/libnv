#include "nv/memory/alloc.h"

#include <asm-generic/errno.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "nv/core/constants.h"
#include "nv/core_types.h"

// void* vtable_alloc_no_impl(void*, MemLayout){ return NO_IMPL_METHOD_RESULT; }
void* vtable_realloc_no_impl(void*, void*, MemLayout, MemLayout) { return nullptr; }
void* vtable_zalloc_no_impl(void*, MemLayout) { return nullptr; }
void* vtable_expand_no_impl(void*, void*, MemLayout, MemLayout) { return nullptr; }
// void vtable_free_no_impl(void*, void*) {}
