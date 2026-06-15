
#include "nv/core/ext.h"
#include "nv/memory/fmap.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include <unistd.h>

#include "nv/core/attributes.h"
#include "nv/core/log.h"
#include "nv/memory/error.h"

PARAMS_NONNULL(1)
static inline i64 file_size(FILE* f) {
  assert(f);
  fseek(f, 0, SEEK_END);
  const i64 size = ftell(f);
  assert(size >= 0);
  rewind(f);
  return size;
}

// NvError fmap_load_init(FileMap*, const char* path) {}

PARAMS_NONNULL(1, 2, 3)
static inline NvError open_file(FILE** file, i64* size, const char* path) {
  assert(file);
  assert(path);
  assert(size);
  assert(*file == nullptr);
  char cwd[PATH_MAX] = {};

  const char* full_cwd = getcwd(cwd, PATH_MAX);
  if (is_null(full_cwd)) {
    LOG_FATAL("Could not get current working directory!");
  }

  FILE* f = fopen(path, "r");

  *file = f;
  if (is_null(f)) {
    DERR("Failed to open file: %s", path);
    const i64 eno = errno;

    switch (eno) {
      case EACCES: {
        return Error__FilePermissionDenied;
      } break;
      case ENOTDIR: {
        return Error__FileNotADirectory;
      } break;
      case ENOENT: {
        return Error__FileNotFound;
      } break;
      case EFBIG: {
        return Error__FileTooBig;
      } break;
      case ENAMETOOLONG: {
        return Error__FilePathTooLong;
      } break;
      case ENOMEM: {
        return Error__OOM;
      } break;
      default: {
        return Error__FileMapFailedToLoad;
      } break;
    }
  }

  *size = file_size(f);
  return OK;
}

NvError fmap_load_init(FileMap* self, const char* path) {
  if UNLIKELY (is_null(self)) {
    return Error__ParamInvalidMethod;
  }

  if UNLIKELY (!fmap_is_none(self)) {
    return Error__FileMapAlreadyInitialized;
  }

  FILE* file = nullptr;
  i64 size = 0;
  const NvError err = open_file(&file, &size, path);
  if (err != OK) {
    return err;
  }

  char* map = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, fileno(file), 0);
  if UNLIKELY (map == MAP_FAILED) {
    DERR("failed to initialize virtual memory for file: %s of size %li bytes", path, size);
    return Error__FailedMemMap;
  }

  fclose(file);
  self->base = map;
  self->data_size = size;
  self->data = map;
  self->offset_len = 0;
  self->offsets = nullptr;
  self->data_start = 0;
  self->data_end = size;
  return OK;
}

/// FileMap fmap_load_directory(const char* path, bool recursive)

TODO_FN(NvError, fmap_load_directory_init, FileMap*, const char*, bool)

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

  NvError err = OK;
  FILE* fds[file_count] = {};
  i64 file_sizes[file_count] = {};
  i64 total = 0;

  for (i32 i = 0; i < file_count; i++) {
    const char* path = files[i];
    FILE* file = nullptr;

    i64 size = 0;
    err |= open_file(&file, &size, path);
    if (err != OK) {
      DERR("Failed to open file: %s", path);
      goto cleanup;
    }

    fds[i] = file;
    total += size;
    file_sizes[i] = size;
  }

  const i64 map_size = total + (sizeof(FileOffset) * file_count);

  char* map = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
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

    // const i64 fd = fileno(f);

    const i64 start = cursor - map;

    const i64 nn = fread(cursor, 1, size, f);
    // LOG("READ %.*s", (i32)size, cursor);

    if (nn == -1) {
      DERR(
          "Initializing Memory Mapped File of size: %li succeeded, but mapping file: %s into the memory mapped region "
          "@ offset: %li "
          "failed!",
          size, files[i], start);
      err |= Error__FileMapFailedToLoad;
      goto cleanup;
    }
    // const char* fmap = mmap(cursor, size, PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);
    // }

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

  return OK;

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
  LOG("IN CLEANUP");
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
    const error err = munmap(self->base, self->data_size);
    if UNLIKELY (err != 0) {
      LOG_FATAL("Failed to unmap memory mapped file: ERRNO(%d) %s", errno, strerror(errno));
    }
    memset(self, 0, sizeof(FileMap));
  }
}
