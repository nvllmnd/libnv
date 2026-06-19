
#include "nv/core/ext.h"
#include "nv/memory/fmap.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/mman.h>
#include <unistd.h>

#include "nv/core/attributes.h"
#include "nv/core/log.h"
#include "nv/core/system.h"
#include "nv/memory/error.h"
#include "nv/core/stb_sprintf.h"

PARAMS_NONNULL(1)
static inline i64 file_size(FILE* self) {
  fseek(self, 0, SEEK_END);
  const i64 n = ftell(self);

  rewind(self);
  return n;
}

NvError fmap_load_all_init_(FileMap* self, const char* files[], i32 file_count) {
  LOG("LOADING %d files", file_count);
  if UNLIKELY (is_null(self)) {
    return Error__ParamInvalidMethod;
  }

  if UNLIKELY (file_count >= FMAP_FILE_COUNT_MAX) {
    return Error__TooManyFiles;
  }

  if UNLIKELY (!fmap_is_none(self)) {
    return Error__FileMapAlreadyInitialized;
  }

  NvError err = NVOK;
  FILE* fds[file_count] = {};
  i64 file_sizes[file_count] = {};
  i64 total = 0;

  for (i32 i = 0; i < file_count; i++) {
    const char* path = files[i];
    // i32 file = -1;

    FILE* file = fopen(path, "r");
    if (is_null(file)) {
      DERR("Failed to open file: %s", path);
      goto cleanup;
    }

    i64 size = file_size(file);

    fds[i] = file;
    total += size;
    file_sizes[i] = size;
  }

  const i64 map_size = total + (sizeof(FileOffset) * file_count);

  char* map = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (map == MAP_FAILED) {
    DERR("failed to initialize memory mapped file of size: %li", map_size);
    err |= Error__FailedMemMap;
    goto cleanup;
  }

  const i64 offsets_size = (sizeof(FileOffset) * file_count);

  char* cursor = map + offsets_size;
  FileOffset* offsets = (FileOffset*)map;

  for (i32 i = 0; i < file_count; i++) {
    FILE* f = fds[i];
    const i64 size = file_sizes[i];

    const i64 start = cursor - map;
    assert(start >= 0);

    const usize bytes_read = fread(cursor, 1, size, f);
    if (bytes_read != (u64)size) {
      DERR(
          "Initializing Memory Mapped File of size: %li succeeded, but mapping file: %s into the memory mapped region"
          "@ offset: %li "
          "failed!",
          size, files[i], start);
      err |= Error__FileMapFailedToLoad;
      goto cleanup;
    }

    fclose(f);
    fds[i] = nullptr;

    cursor += size;

    offsets[i] = (FileOffset){
        .file_id = i,
        .start = start,
        .end = start + size,
    };
  }

  self->base = map;
  self->data = map + offsets_size;
  self->data_size = total;
  self->data_end = map_size;
  self->offsets = offsets;
  self->offset_len = file_count;

  // sanity check
  assert((map_size - total) == offsets_size);

  return NVOK;

cleanup:
  for (i32 i = 0; i < file_count; i++) {
    FILE* f = fds[i];
    if (is_not_null(f)) {
      fclose(f);
    }
  }
  if (is_not_null(map)) {
    assert(munmap(map, map_size) == 0);
  }

  return err | Error__FailedMemMap;
}

sslice fmap_file_data(const FileMap* self, i64 fileid) {
  if UNLIKELY (is_null(self) || is_null(self->offsets) || fileid < 0 || fileid >= self->offset_len) {
    return sslice_empty();
  }
  const FileOffset* fo = &self->offsets[fileid];
  const char* s = &self->data[fo->start];
  const i32 size = fo->end - fo->start;
  return sslice_new(.begin = s, .len = size);
}

sslice fmap_as_string(const FileMap* self) {
  assert(self);
  const char* str = &self->data[self->data_start];
  const i32 size = self->data_size;
  return sslice_new(.begin = str, .len = size);
}

void fmap_destroy(FileMap* self) {
  if UNLIKELY (is_null(self)) {
    return;
  }
  if (is_not_null(self->base)) {
    const error err = munmap(self->base, self->data_end);
    if UNLIKELY (err != 0) {
      LOG_FATAL("Failed to unmap memory mapped file: ERRNO(%d) %s", errno, strerror(errno));
    }
    memset(self, 0, sizeof(FileMap));
  }
}
