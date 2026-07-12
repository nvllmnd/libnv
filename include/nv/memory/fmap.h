#pragma once

#include <string.h>

#include "nv/common.h"
#include "nv/core/attributes.h"

BEGIN_C_DECLS

struct FileOffset {
  /// Entry number of this file
  i64 file_id;
  i64 start;
  i64 end;
};
alias(FileOffset);

/// @brief Information for a memory mapped file.
/// @details This data type supports loading multiple files, which then get
/// loadedd as such: (FO = FileOffset)
///
/// |###################################################################################|
/// |    |    |    |    |    |              |             |               |             |
/// |.FO.|.FO.|.FO.|.FO.|.FO.|...File Data..|..File Data..|....File Data..|..File Data..|
/// |    |    |    |    |    |              |             |               |             |
/// |###################################################################################|
///
///
/// As such the only extra data allocated (besides the loaded files themselves) are the FileOffset structs at the start
/// of the resulting memory mapped file,
///
struct FileMap {
  /// @brief pointer to start of file mapped memory
  /// @details uesd for munmap
  void* base;
  char* data;

  FileOffset* offsets;
  /// @brief number of files loaded into this FileMap
  i64 offset_len;
  i64 data_size;
  /// @brief byte offset to the start of loaded file data
  /// @details data at this offset will be just the file data if only
  /// one file is loaded, otherwise it will contain FileOffsets, one for each entry, followed by each file data,
  /// delimited by
  //
  i64 data_start;

  /// @brief byte offset to the end of this filemap
  i64 data_end;
};
alias(FileMap);

static constexpr const FileMap FMAP_NONE = {};

PURE_FUNC
static inline bool fmap_is_none(const FileMap* self) { return self && memcmp(self, &FMAP_NONE, sizeof(FileMap)) == 0; }

/// @brief was there only a single file loaded into this [FileMap]?
/// @details does it conatain offsets?
PURE_FUNC
static inline bool fmap_is_single(const FileMap* self) {
  return self && self->offset_len <= 0 && is_null(self->offsets) && (char*)self->base == self->data;
}

static inline bool fmap_is_multi(const FileMap* self) { return !fmap_is_single(self); }

static constexpr const i32 FMAP_FILE_COUNT_MAX = 1024;

NvError fmap_load_all_init_(FileMap* self, const char* files[], i32 file_count);

#define fmap_load_all_init(_fm, ...) \
  (fmap_load_all_init_((_fm), (const char*[]){__VA_ARGS__}, VA_ARGS_LEN(__VA_ARGS__)))

#ifndef __cplusplus
sslice fmap_file_data(const FileMap* self, i64 fileid) METHOD PURE_FUNC;

sslice fmap_as_string(const FileMap* self) METHOD PURE_FUNC;
#endif

void fmap_destroy(FileMap* self);

END_C_DECLS
