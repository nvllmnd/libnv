#pragma once

#include <assert.h>

#ifndef ZEAL_DEBUG

#ifdef NDEBUG
#define ZEAL_DEBUG 0
#else
#define ZEAL_DEBUG 1
#endif

#endif

#if ZEAL_DEBUG == 1
#define IF_DEBUG(x) x
#define IF_RELEASE(x)
#else
#define IF_DEBUG(x)
#define IF_RELEASE(x) x
#endif

