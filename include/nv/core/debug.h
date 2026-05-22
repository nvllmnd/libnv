#pragma once

#include <assert.h>
#include "nv/core_types.h"

#ifndef LIBNV_DEBUG

#ifdef NDEBUG
#define LIBNV_DEBUG 0
#else
#define LIBNV_DEBUG 1
#endif

#endif

#if LIBNV_DEBUG == 1
#define IF_DEBUG(x) x
#define IF_RELEASE(x)
#define ASSERT_PTR(_p) assert((_p))

#else
#define IF_DEBUG(x)
#define IF_RELEASE(x) x
#define ASSERT_PTR(_p) punwrap(_p)
#endif


