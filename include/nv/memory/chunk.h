#pragma once



#include <string.h>
#include "nv/core/attributes.h"
#include "nv/core/core_types.h"
#include "nv/core/intdefs.h"
#include "nv/core/log.h"

struct Chunk {
  byte* begin;
  byte* end;
};
alias(Chunk);

static constexpr const Chunk CHUNK_NONE = (Chunk){};

#define Chunk(...) (Chunk){__VA_ARGS__}

PURE_FUNC
static inline bool chunk_is_ok(Chunk self) { return memcmp(&self, &CHUNK_NONE, sizeof(Chunk)) != 0;
}

PURE_FUNC
PARAMS_NONNULL(1)
static inline Chunk chunk_new(byte* begin, i64 size_bytes) {
  if (is_null(begin) || size_bytes <= 0) {
    LOG_ERROR("Tried to create a new Chunk of negative size (%li) or with nullptr!", size_bytes);
    return CHUNK_NONE;
  }
  byte* end = begin + size_bytes; 

  return Chunk(begin = begin, .end = end);

}


