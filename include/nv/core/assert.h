#pragma once

#ifdef __cplusplus
#define ASSERT()                                                                                                   \
  do {                                                                                                             \
    if (!(_expr)) {                                                                                                \
      fprintf(stderr, "[%s:%d]::%s => " _fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__); \
      std::terminate();                                                                                            \
    }                                                                                                              \
  } while (0)

#endif

#ifndef __cplusplus

#define ASSERT(_expr, _fmt, ...)                                                                   \
  do {                                                                                             \
    if (!(_expr)) {                                                                                \
      log_fatal("[%s:%d]::%s => " _fmt, __FILE__, __LINE__, __func__, __VA_OPT__(, ) __VA_ARGS__); \
      EXIT_FATAL();                                                                                \
    }                                                                                              \
  } while (0)

#endif
