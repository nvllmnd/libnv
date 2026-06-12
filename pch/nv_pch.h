// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif


// #define __USE_POSIX 1

// #define __USE_MISC 1
// #ifndef __USE_XOPEN_EXTENDED
// #define __USE_XOPEN_EXTENDED 1

// #endif
// #define _POSIX_C_SOURCE 1


#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

#include "nv/core/log.h"
#include "nv/core/sslice.h"
#include "nv/core/attributes.h"
#include "nv/core/constants.h"
#include "nv/core/debug.h"
#include "nv/core/intdefs.h"
#include "nv/core/core_types.h"
#include "nv/iter/array.h"
#include "nv/memory/error.h"

#include "nv/core/algo.h"
#include "nv/memory/layout.h"
