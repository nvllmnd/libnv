#pragma once

#if defined(LIBNV_INTERNAL) && LIBNV_INTERNAL == 1
#error __FILE__ " is only intended to be included by external projects that depend on libnv"
#endif

#undef LIBNV_TOP_LEVEL_INCLUDE
#define LIBNV_TOP_LEVEL_INCLUDE 1


#include "nv/core.h"
#include "nv/memory.h"
#include "nv/iter.h"

#undef LIBNV_TOP_LEVEL_INCLUDE
#define LIBNV_TOP_LEVEL_INCLUDE 0

