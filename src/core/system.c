#include "nv/core/system.h"
#include "nv/core/algo.h"
#include "nv/core/stb_sprintf.h"
#include "nv/core/ext.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

NvError open_file(i32* out, i64* size, const char* path) {
  assert(out);
  assert(path);
  assert(size);

  i32 fd = open(path, O_RDONLY);

  if (fd == -1) {
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

  struct stat file_stat = {};
  if (fstat(fd, &file_stat) != 0) {
    DERR("Failed invokation of fstat");
    return Error__FileIO;
  }

  *size = file_stat.st_size;
  *out = fd;
  LOG("File Size; %li FD: %d", *size, *out);

  return OK;
}

const char* get_cwd(char* out, i32* out_len) {
  char cwd[PATH_MAX] = {};
  const char* full_cwd = getcwd(cwd, PATH_MAX);
  if UNLIKELY (is_null(full_cwd)) {
    return nullptr;
  }

  const isize len = stringlen(full_cwd) + 1;

  const auto n = stbsp_snprintf(out, len, "%s", cwd) >= 0;
  if (n < 0 ) {
    LOG_ERROR("stbsp_snprintf error! Returned -1!");
    return nullptr;
  }
  if (out_len) {
    *out_len = len - 1;
  }
  return out;
}

/// @brief gets current working directory and caches the path string
/// @details This function calls [getcwd] and caches the result in a static buffer of size [PATH_MAX],
/// to always get the active working directory, @see [get_cwd]
const char* get_pwd(void) {
  static char cwd[PATH_MAX] = {};
  static bool init = false;
  if UNLIKELY (!init) {
    init = true;
    return getcwd(cwd, PATH_MAX);
  }
  return cwd;
}

/// @brief calls [getcwd] and puts the result in given Allocator
const char* get_cwd_in(Allocator alloc) {
  char cwd[PATH_MAX] = {};
  const char* path = getcwd(cwd, PATH_MAX);
  const i64 n = stringlen(path) + 1;
  char* res = allocator_allocate(alloc, mlayout_bytes(n));
  assert(stbsp_snprintf(res, n, "%s", path) >= 0);
  return res;
}

NvError close_file(i32 fd) {
  if (close(fd) == -1) {
    DERR("Failed to close file with descriptor value: %d", fd);
    return Error__FileIO;
  }
  return OK;
}

NvError file_open(File* self, const char* path) {
  assert(self);
  i32 fd = -1;
  i64 size = 0;
  const NvError err = open_file(&fd, &size, path);
  if (err != OK) {
    DERR("Failed to open file with path: %s", path);
    return err | Error__FileIO;
  }
  assert(fd != -1);
  assert(size > 0);

  self->fd = fd;
  self->size = size;

  return OK;
}

NvError file_close(File* self) {
  const NvError err = close_file(self->fd);
  if (err != 0) {
    return err | Error__FileIO;
  }
  memset(self, 0, sizeof(File));
  return OK;
}
