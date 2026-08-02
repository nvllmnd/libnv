#pragma once

#ifdef __cplusplus

namespace nv {}

#include "alloc.hpp"
#include "ptr.hpp"
#include "convert.hpp"

#include "defer.hpp"

#endif

#ifndef __cplusplus

#error "libnv cxx module can only be included from C++!"

#endif
