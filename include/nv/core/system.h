#pragma once

#include <stdio.h>
#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/memory/alloc.h"
#include "nv/memory/error.h"

BEGIN_C_DECLS

struct File {
  i64 fd;
  i64 size;
};
alias(File);

struct FILE;

NvError open_file(i32* out, i64* size, const char* path);

NvError close_file(i32 fd);

NvError file_open(File* self, const char* path);
NvError file_close(File* self);

/// @brief caches the result of [getcwd]
const char* get_pwd(void);

/// @brief calls [getcwd] and puts the result in given Allocator
const char* get_cwd_in(Allocator alloc);

const char* get_cwd(char* out, i32* out_len);

END_C_DECLS
