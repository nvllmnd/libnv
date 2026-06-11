
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif




#include "nv/core/fs.h"

#include <stdlib.h>
#include <linux/limits.h>

#include <limits.h>
#include <stdio.h>


#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


#include "nv/core/log.h"
#include "nv/memory/alloc.h"
#include "nv/memory/layout.h"

// TODO: Need to write tests for all this!

struct FileInfo {
  FILE* fd;
  i64 size;
};
alias(FileInfo);

static inline i64 fsize(FILE* f) {
  fseek(f, 0L, SEEK_END);
  const i64 size = ftell(f);
  fseek(f, 0L, SEEK_SET);
  return size;
}

CONST_FUNC
static inline i64 fmap_full_size(i64 size, i64 entry_count) {
  return size + sizeof(FileMap) + (sizeof(FileEntry) * entry_count) + (FMAP_ENTRY_DELIM_SIZE * entry_count);
}

CONST_FUNC
[[maybe_unused]]
static inline i64 fent_full_size(i64 size) {
  return sizeof(FileEntry) + size + (FMAP_ENTRY_DELIM_SIZE * sizeof(char));
}

static inline FileMap* load_from_info(const char* path, FileInfo info) {
  assert(info.fd);
  assert(info.size > 0);
  const i64 size = info.size;
  const auto fd = fileno(info.fd);

  FileMap* ptr = mmap(0, size + sizeof(FileMap), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED) {
    LOG_FATAL("Failed to memory-map file at path %s of size: %li", path, size);
  }

  ptr->size = size;
  ptr->id = FID__FileMap;
  ptr->entry_count = 1;

  return ptr;
}

FileEntry* file_read_mem(const char* filepath, Allocator alloc) {
  static char REALPATH[PATH_MAX] = {};
  const char* rp = realpath(filepath, REALPATH);
  if (is_null(rp)) {
    LOG_FATAL("Failed to expand path: %s => %s", filepath, strerror(errno));
  }



  LOG_INFO("about to open file: %s", rp);
  FILE* f = fopen(rp, "r");
  if (is_null(f)) {
    LOG_ERROR("Failed to open file: %s => %s", rp, strerror(errno));
    return nullptr;
  }
  const i64 size = fsize(f);
  LOG_INFO("Opened file: %s of size: %li", rp, size);

  if (size >= FMAP_MMAP_MIN_SIZE) {
    LOG_ERROR(
        "Size %li bytes is too large for simply reading into memory, (is over the %li byte limit) use a FileMap "
        "instead, which is backed by mmap",
        size, FMAP_MMAP_MIN_SIZE);
    return nullptr;
  }

  const MemLayout layout = mlayout_fma(FileEntry, size);
  File* entry = allocator_allocate(alloc, layout);
  if (is_null(entry)) {
    fclose(f);
    LOG_ERROR(
        "Failed to allocate enough space with given allocator. Not enough space for %li bytes! which is required to "
        "load %s into memory!",
        layout.size, rp);
    return nullptr;
  }

  const auto fd = fileno(f);

  entry->size = size;
  entry->id = -fd;

  const i32 err = read(fd, &entry->data[0], entry->size);
  if (err == -1) {
    fclose(f);
    LOG_ERROR("Failed to read file at path: %s into memory! syscall read failed for FD: %d. ERRNO: %s", rp, fd,
              strerror(errno));
    return nullptr;
  }

  return entry;
}

i64 file_read_into(const char* filepath, char* buff, i32 buff_count) {
  FILE* f = fopen(filepath, "r");
  const auto fd = fileno(f);
  const i32 count = read(fd, buff, buff_count);
  if (count == -1) {
    fclose(f);
    LOG_ERROR("Failed to read file at path: %s into memory! syscall read failed for FD: %d. ERRNO: %s", filepath, fd,
              strerror(errno));
  }

  return count;
}

FileMap* fmap_load(const char* path) {
  FILE* f = fopen(path, "r");
  if (is_null(f)) {
    LOG_FATAL("Failed to open file at filepath: %s", path);
  }
  const i64 size = fsize(f);

  const FileInfo fi = (FileInfo){.fd = f, .size = size};
  return load_from_info(path, fi);
}

FileMap* fmap_load_many(const char* paths[], i32 path_count) {
  assert(paths);
  assert(path_count > 0);

  if UNLIKELY (path_count >= FMAP_FILE_LOAD_MAX) {
    LOG_FATAL(
        "Maximum number of files reached! You can not load more than %li files at a time, however you tried to load %d "
        "files at one time, which is over that limit!",
        FMAP_FILE_LOAD_MAX, path_count);
  }

  FileInfo files[path_count] = {};
  i64 total = 0;

  for (i32 i = 0; i < path_count; i++) {
    const char* fp = paths[i];
    FILE* f = fopen(fp, "r");
    if (is_null(f)) {
      LOG_ERROR("Failed to open file at path: %s", fp);
      return nullptr;
    }
    const i64 size = fsize(f);
    files[i] = (FileInfo){.fd = f, .size = size};
    total += files[i].size;
  }

  FileMap* fm = mmap(0, fmap_full_size(total, path_count), PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (fm == MAP_FAILED) {
    LOG_ERROR("Failed to concat many files, totaling %li bytes", total);
    return nullptr;
  }
  fm->size = total;
  fm->entry_count = path_count;

  char* top = (char*)&fm->storage[0];
  for (i32 i = 0; i < path_count; i++) {
    const auto fd = fileno(files[i].fd);
    const i64 size = files[i].size;
    const i64 fullsize = fent_full_size(size);
    FileEntry* entry = mmap(top, fullsize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, fd, 0);
    if (entry == MAP_FAILED) {
      LOG_ERROR("Failed to MAP_FIXED for file of size %li and offset: %li", size, top - ((char*)&fm->storage[0]));
      return nullptr;
    }

    entry->size = size;
    entry->id = i;
    top += fullsize;
    /// Dont put file delim on last mapping
    if LIKELY (i + 1 < path_count) {
      entry->data[entry->size - 1] = FMAP_ENTRY_DELIM[0];
      entry->data[entry->size] = 0;
    }
    fclose(files[i].fd);

    files[i] = (FileInfo){};
  }

  return fm;
}

void fmap_unmap(FileMap* self) {
  if (is_not_null(self)) {
    const i64 size = self->size;
    const i32 err = munmap(self, size);
    if (err == -1) {
      LOG_FATAL("Failed to unmap memory mapped file of size: %li", size);
    }
  }
}
