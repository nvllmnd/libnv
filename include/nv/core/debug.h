#pragma once



#ifndef LIBNV_DEBUG

#ifdef NDEBUG 
#define LIBNV_DEBUG 0
#else
#define LIBNV_DEBUG 1
#endif //  NDEBUG  
#endif // LIBNV_DEBUG

#if LIBNV_DEBUG == 1

#include <assert.h>

#define IF_DEBUG(x) x
#define IF_RELEASE(x)
#define ASSERT_PTR(_p) assert((_p))

#else

#include "nv/core/core_types.h"

#define IF_DEBUG(x)
#define IF_RELEASE(x) x
#define ASSERT_PTR(_p) punwrap(_p)

#endif // if LIBNV_DEBUG == 1

#include "hedley.h"

#if HEDLEY_HAS_BUILTIN(__builtin_trap) 
#ifndef EXIT_FATAL
#define EXIT_FATAL() __builtin_trap()
#endif // ifndef EXIT_FATAL
#ifndef EXIT_FATAL
#define EXIT_FATAL() abort()
#endif // ifndef EXIT_FATAL
#endif // HEDLEY_HAS_BUILTIN(__builtin_trap)

