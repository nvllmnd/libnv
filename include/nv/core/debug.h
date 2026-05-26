#pragma once



#ifndef LIBNV_DEBUG

#ifdef NDEBUG
#define LIBNV_DEBUG 0
#else
#define LIBNV_DEBUG 1
#endif

#endif

#if LIBNV_DEBUG == 1

#include <assert.h>

#define IF_DEBUG(x) x
#define IF_RELEASE(x)
#define ASSERT_PTR(_p) assert((_p))

#else

#include "nv/core_types.h"

#define IF_DEBUG(x)
#define IF_RELEASE(x) x
#define ASSERT_PTR(_p) punwrap(_p)
#endif
