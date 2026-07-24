// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "nv/core/ext.h"

// #define __USE_POSIX 1

// #define __USE_MISC 1
// #ifndef __USE_XOPEN_EXTENDED
// #define __USE_XOPEN_EXTENDED 1

// #endif
// #define _POSIX_C_SOURCE 1

#include <cassert>
#include <cerrno>
#include <cstdalign>
#include <cstdarg>
#include <stdatomic.h>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <strings.h>
#include <type_traits>
#include <algorithm>
#include <bit>
#include <concepts>

#include "nv/cxx/alloc.hpp"
#include "nv/cxx/opt.hpp"

#include "nv/cxx/result.hpp"
#include "nv/cxx/defer.hpp"
#include "nv/cxx/ptr.hpp"

#include "nv/cxx/alloc.hpp"
