#pragma once


#define __USE_POSIX 1
#define _POSIX_C_SOURCE 1

#include "nv/memory/alloc.h"

struct FileEntry {
  i64 size; 
  /// @brief the value of this field depends on how this file was loaded, if its a positive value, it
  /// is the index position of the FileEntry in a larger FileMap, if its negative, its most likely a negated file descritor value
  i64 id;
  ATTR_COUNTED_BY(size)
  char data[];
};
alias(FileEntry);

typedef FileEntry File;


File* read_file(const char* filepath, Allocator alloc);
i64 fread_into(const char* filepath, char* buff, i32 buff_count);


#ifndef LIBNV_FILEMAP_DELIM
#define LIBNV_FILEMAP_DELIM INT8_MIN
#endif

static constexpr const i64 FMAP_MMAP_MIN_SIZE = 4096;

static constexpr const i32 FMAP_ENTRY_DELIM_SIZE = 2;
static constexpr const char FMAP_ENTRY_PREFIX = LIBNV_FILEMAP_DELIM;
static constexpr const char FMAP_ENTRY_DELIM[FMAP_ENTRY_DELIM_SIZE] = { FMAP_ENTRY_PREFIX, 0};

/// @brief based off the fact we are using a static dynamicaly sized array in the [fmap_load_many] impl
static constexpr const i64 FMAP_FILE_LOAD_MAX = UINT16_MAX;


/// @brief Memory mapped file.
/// @details if many files were concatenated together into a single resulting memory mapped file,
/// each file is separated by a [LIBNV_FILEMAP_DELIM] value. folllowed by a null character.I
///
///
struct FileMap {
  i64 size;

  i64 entry_count;

  ATTR_COUNTED_BY(size)
  char storage[];
};
alias(FileMap);

FileMap* fmap_load(const char* path);
FileMap* fmap_load_many(const char* paths[], i32 path_count);

#define fmap_load_files(...) ({\
  static const char* _FILES[] = { __VA_ARGS__ }; \
  static constexpr const i64 _FILES_LEN = sizeof(_FILES) / sizeof(char*); \
  fmap_load_many(_FILES, _FILES_LEN); \
})

void fmap_unmap(FileMap* self);

