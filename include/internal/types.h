#pragma once

#include "nv/core/constants.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"
#if defined(LIBNV_INTERNAL) && LIBNV_INTERNAL == 1

struct VirtMem;

typedef struct VirtMem NvAlloc;

static constexpr const i64 DEFAULT_NVALLOC_SIZE = GB(2);

NvError nvalloc_init(i64 size);

NvAlloc* nvallocator(void);

#else
#error __FILE__ "is for libnv internal use only! Remove its #include!"

#endif
