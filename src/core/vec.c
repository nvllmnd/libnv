#include "nv/iter/vec.h"

#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/core/log.h"
#include "nv/memory/alloc.h"
#include "nv/memory/layout.h"

struct VecMeta {
  i32 len;
  i32 capacity;
  i32 elem_size;
};
alias(VecMeta);

struct Vector {
  /// @brief aligned top
  u8* start;

  /// @brief capacity by element count

  i32 elem_size;

  i32 capacity;

  /// @brief count of elements
  i32 len;
  /// @brief full-size, in bytes
  i32 size;

  ATTR_COUNTED_BY(size)
  u8 data[];
};
alias(Vector);

// METHOD
// PURE_FUNC
// static inline void* vdata(Vector* self) {
//   assert(self);
//   return ptr_alignup(&self->data[0],  self->elayout.align);
// }

// METHOD
// PURE_FUNC
// static inline const void* vdata_const(const Vector* self) {
//   assert(self);
//   return ptr_alignup((void*)&self->data[0],  self->elayout.align);
// }

PARAMS_NONNULL(1)
RETURNS_NON_NULL
PURE_FUNC
static inline Vector* asvec(VecAny s) { return prefix_offset(s, Vector); }

PARAMS_NONNULL(1)
RETURNS_NON_NULL
PURE_FUNC
static inline const Vector* asvecc(const VecAny s) { return prefix_offset(s, Vector); }

static inline void* vindex(Vector* self, i32 index) {
  assert(self);
  assert(index >= 0 && index < self->len);
  const i32 i = index * self->elem_size;
  return &self->start[i];
}

METHOD
static inline void* vectop(Vector* self) {
  assert(self);
  if (self->len <= self->capacity) {
    return vindex(self, self->len - 1);
  }
  return nullptr;
}

// #define VecHeader(T) struct { VecMeta meta; T data[]; }

// static_assert(sizeof(VecHeader(i32)) == sizeof(VecMeta));

VecAny vec_new_(MemLayout elayout, i32 capacity, Allocator alloc) {
  assert(capacity > 0);
  Vector* self = allocator_allocate(alloc, mlayout_extend_with(elayout, capacity, mlayout_new(Vector)));
  assert(self);
  self->capacity = capacity;
  self->len = 0;
  self->size = capacity * elayout.size;
  self->start = ptr_alignup(&self->data[0], elayout.align);
  LOG("data: %p start: %p", &self->data[0], self->start);

  self->elem_size = elayout.size;

  assert(self->capacity == (self->size / self->elem_size));
  return self;
}

METHOD
void* vec_insert_back(VecAny s) {
  assert(s);
  Vector* self = asvec(s);
  const i32 avail = vec_avail_bytes(self);
  if (avail < self->elem_size) {
    return nullptr;
  }
  void* ptr = vectop(self);

  self->len += 1;
  return ptr;
}

PURE_FUNC
METHOD
i32 vec_len(const VecAny s) {
  assert(s);
  return asvecc(s)->len;
}

PURE_FUNC
METHOD
i32 vec_capacity(const VecAny s) {
  assert(s);
  return asvecc(s)->capacity;
}

METHOD
PURE_FUNC
const void* vec_cend_(const VecAny self) {
  assert(self);
  const Vector* v = asvecc(self);
  return (&v->data[v->size]);
}

METHOD
PURE_FUNC
void* vec_end_(VecAny self) {
  assert(self);
  Vector* v = asvec(self);
  return (&v->data[v->size]);
}

METHOD
i32 vec_set_len(VecAny s, i32 new_len) {
  Vector* self = asvec(s);

  if (self->len == new_len) {
    return self->len;
  }

  self->len = clamp(new_len, 0, self->capacity);
  return self->len;
}

i32 vec_grow_to_cap(VecAny s) {
  Vector* self = asvec(s);
  self->len = self->capacity;
  return self->len;
}

/// @brief returns number of free elements before reaching capacity
PURE_FUNC
METHOD
i32 vec_available(const VecAny s) {
  assert(s);
  const Vector* self = asvecc(s);
  return self->capacity - self->len;
}

/// @brief same as [vec_available], but returns size in bytes
PURE_FUNC
METHOD
i32 vec_avail_bytes(const VecAny s) {
  assert(s);
  const Vector* self = asvecc(s);

  return vec_available(s) * self->elem_size;
}

PURE_FUNC
METHOD
bool vec_is_full(const VecAny s) {
  assert(s);
  const Vector* self = asvecc(s);
  return self->len >= self->capacity;
}

PURE_FUNC
METHOD
bool vec_is_empty(const VecAny s) {
  assert(s);
  const Vector* self = asvecc(s);

  return self->len <= 0;
}

PURE_FUNC
METHOD
f32 vec_load_factor(const VecAny s) {
  assert(s);
  const Vector* self = asvecc(s);

  const f32 denom = self->len == 0 ? 1. : (f32)self->len;

  return self->capacity / denom;
}

METHOD
VecAny vec_resize_(VecAny s, i32 new_capacity, Allocator alloc) {
  assert(s);
  assert(new_capacity > 0);
  Vector* self = asvec(s);

  if (new_capacity == self->capacity) {
    return s;
  }
  if (new_capacity < self->capacity) {
    self->capacity = new_capacity;

    if (self->len > new_capacity) {
      self->len = new_capacity;
    }
    return s;
  }

  const auto old = mlayout_fma(Vector, self->size);
  const auto nlayout = mlayout_fma(Vector, self->size);
  if (vtmask_has_expand(alloc.vtable->mask)) {
    if (allocator_expand(alloc, self, old, nlayout)) {
      return &self->start[0];
    }
  }

  if (vtmask_has_realloc(alloc.vtable->mask)) {
    self = allocator_reallocate(alloc, self, old, nlayout);
    if LIKELY (is_not_null(self)) {
      return &self->start[0];
    }
    return s;
  } else {
    Vector* next = allocator_allocate(alloc, nlayout);
    if LIKELY (is_not_null(next)) {
      memcpy(next, self, nlayout.size);

      next->start = ptr_alignup(&next->data[0], nlayout.align);
      allocator_free(alloc, self);
      return next;
    }
    LOG_FATAL("Failed to reallocate vector with elem size: %d bytes, of len: %d, cap: %d. with unsupported Allocator!",
              self->elem_size, self->len, self->capacity);
  }
}

void vec_destroy(VecAny self, Allocator alloc) {
  if (is_not_null(self)) {
    allocator_free(alloc, self);
    self = nullptr;
  }
}

void vec_clear(VecAny s) {
  assert(s);
  Vector* self = asvec(s);
  self->len = 0;
}
void vec_clear_zeroed(VecAny s) {

  assert(s);
  Vector* self = asvec(s);  
  self->len = 0;
  memset(&self->data[0], 0, self->size);
}
