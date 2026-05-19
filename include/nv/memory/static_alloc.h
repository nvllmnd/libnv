#pragma once

#include "nv/core/attributes.h"
#include "nv/core/constants.h"
#include "nv/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/memory/layout.h"
#include "nv/core/algo.h"

typedef struct StaticAlloc StaticAlloc;

#define TStaticAlloc(N) \
  struct {              \
    u8 storage[N];      \
  }

typedef TStaticAlloc(KILOBYTES(4)) StaticAlloc4k;

typedef TStaticAlloc(KILOBYTES(8)) StaticAlloc8k;

typedef TStaticAlloc(KILOBYTES(12)) StaticAlloc12k;
typedef TStaticAlloc(KILOBYTES(16)) StaticAlloc16k;

typedef TStaticAlloc(KILOBYTES(24)) StaticAlloc24k;

typedef TStaticAlloc(KILOBYTES(32)) StaticAlloc32k;

typedef TStaticAlloc(KILOBYTES(64)) StaticAlloc64k;
typedef TStaticAlloc(KILOBYTES(128)) StaticAlloc128k;

typedef TStaticAlloc(KILOBYTES(255)) StaticAlloc255k;
typedef TStaticAlloc(KILOBYTES(512)) StaticAlloc512k;

typedef TStaticAlloc(MEGABYTES(1)) StaticAlloc1mb;
typedef TStaticAlloc(MEGABYTES(2)) StaticAlloc2mb;

typedef TStaticAlloc(MEGABYTES(4)) StaticAlloc4mb;
typedef TStaticAlloc(MEGABYTES(8)) StaticAlloc8mb;

typedef TStaticAlloc(MEGABYTES(16)) StaticAlloc16mb;
typedef TStaticAlloc(MEGABYTES(24)) StaticAlloc24mb;

#define salloc_capacity(_self) (sizeof(__typeof((_self).storage)))
#define salloc_begin(_self) (&(_self).storage[0])
#define salloc_end(_self) (salloc_begin(_self) + salloc_capacity(_self))

#define salloc_ref(_self) ((StaticAlloc*)salloc_begin(_self))

#define salloc_cref(_self) ((const StaticAlloc*)salloc_begin(_self))
// #define salloc_used(_self) ((_self).top - salloc_begin(_self))
// #define salloc_ref(_self) (make(StaticAllocRef, .top = &(_self.top), .storage = { .start = &(_self).storage[0], .end
// = &(_self).storage[sizeof(__typeof((_self.storage)))] })

#define salloc_new(N) (make_zeroed(TStaticAlloc(N)))

METHOD
void salloc_init_(StaticAlloc* self, i32 size_bytes);
#define salloc_init(_self) (salloc_init_(salloc_ref(_self), salloc_capacity(_self)))

METHOD
PURE_FUNC
i32 salloc_used(const StaticAlloc* self);

METHOD
void* salloc_allocate(StaticAlloc* self, MemLayout layout) WHERE(IS_POWER_OF_2(layout.align));

METHOD
void* salloc_zallocate(StaticAlloc* self, MemLayout layout) WHERE(IS_POWER_OF_2(layout.align));

PARAMS_NONNULL(1, 2)
void* salloc_expand(StaticAlloc* self, void* ptr, MemLayout old_layout, MemLayout new_layout)
    WHERE(IS_POWER_OF_2(old_layout.align) && IS_POWER_OF_2(new_layout.align));

PARAMS_NONNULL(1, 2)
void* salloc_reallocate(StaticAlloc* self, void* ptr, MemLayout old_layout, MemLayout new_layout)
    WHERE(IS_POWER_OF_2(old_layout.align) && IS_POWER_OF_2(new_layout.align));


PARAMS_NONNULL(1,2)
char* salloc_strndup(StaticAlloc* self, const char* string, i32 n) WHERE(n > 0);

CONST_FUNC
i32 salloc_metadata_size(void);


