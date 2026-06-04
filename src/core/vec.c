#include "nv/iter/vec.h"

#include "nv/core/algo.h"
#include "nv/core/attributes.h"
#include "nv/core/log.h"
#include "nv/memory/alloc.h"
#include "nv/memory/layout.h"
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

PARAMS_NONNULL(1)
RETURNS_NON_NULL
static inline Vector* asvec(VecAny s) { return prefix_offset(s, Vector); }

PARAMS_NONNULL(1)
RETURNS_NON_NULL
PURE_FUNC
static inline const Vector* asvecc(const VecAny s) { return prefix_offset(s, Vector); }

METHOD
PURE_FUNC
static inline i32 vec_full_size(const Vector* self) {
  assert(self);
  return self->size + sizeof(Vector);
}

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
    return vindex(self, self->len);
  }
  return nullptr;
}

CONST_FUNC
static inline i32 vec_data_size(i32 capacity, i32 elem_size) {
  return capacity * elem_size + 1;  // +1, last byte is used for end iterator
}

static inline u8* vec_iter_end(Vector* self) {
  Vector* s = asvec(self);
  return &s->data[s->size];
}

CONST_FUNC
static inline MemLayout vec_layout(i32 capacity, i32 elem_size) {
  return mlayout_fma(Vector, vec_data_size(capacity, elem_size));
}

// #define VecHeader(T) struct { VecMeta meta; T data[]; }

// static_assert(sizeof(VecHeader(i32)) == sizeof(VecMeta));

VecAny vec_new_(MemLayout elayout, i32 capacity, Allocator alloc) {
  assert(capacity > 0);
  Vector* self = allocator_allocate(alloc, vec_layout(capacity, elayout.size));
  assert(self);
  self->capacity = capacity;
  self->len = 0;
  self->size = capacity * elayout.size;
  self->start = ptr_alignup(&self->data[0], elayout.align);

  self->elem_size = elayout.size;

  assert(self->capacity == (self->size / self->elem_size));
  return self->start;
}

void* vec_insert_back(VecAny s) {
  assert(s);
  Vector* self = asvec(s);

  LOG("1 Vec => elem_size: %d, capacity: %d, len: %d, size: %d, start: %p, data: %p, ", self->elem_size, self->capacity,
      self->len, self->size, self->start, self->data);

  const i32 avail = vec_avail_bytes(s);

  if (avail < self->elem_size) {
    LOG_DBG("Failed to insert to back of Vector of elem size: %d bytes, only %d bytes available!", self->elem_size,
            avail);
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
  return vec_iter_end(v);
}

i32 vec_set_len(VecAny s, i32 new_len) {
  Vector* self = asvec(s);

  self->len = new_len;
  return new_len;
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

  LOG("2 Vec => elem_size: %d, capacity: %d, len: %d, size: %d, start: %p, data: %p, ", self->elem_size, self->capacity,
      self->len, self->size, self->start, self->data);

  const i32 avail = vec_available(s);

  return avail * self->elem_size;
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
  const i32 new_size = vec_data_size(new_capacity, self->elem_size);
  const auto nlayout = vec_layout(new_capacity, self->elem_size);

  VecAny next = nullptr;

  if (vtmask_has_realloc(alloc.vtable->mask)) {
    next = allocator_reallocate(alloc, self, old, nlayout);
    if UNLIKELY (is_null(next)) {
      LOG_DBG("Failed to resize Vec of element size: %d bytes with len: %d and capacity: %d to new capacity: %d",
              self->elem_size, vec_len(self), vec_capacity(self), new_capacity);
      return nullptr;
    }
    self->size = new_size;
    self->capacity = new_capacity;
    next = self->start;

    // also try  to expand if this allocator doesn't have realloc method (and if it has an expand method!)
  } else if (vtmask_has_expand(alloc.vtable->mask)) {
    if (allocator_expand(alloc, self, old, nlayout)) {
      self->size = new_size;
      self->capacity = new_capacity;
      next = self->start;
    }
  }

  if (is_null(next)) {
    Vector* ptr = allocator_allocate(alloc, nlayout);
    if LIKELY (is_not_null(ptr)) {
      memcpy(ptr, self, vec_full_size(self));

      ptr->size = new_size;
      ptr->start = ptr_alignup(&ptr->data[0], nlayout.align);
      ptr->capacity = new_capacity;
      allocator_free(alloc, self);
      self = nullptr;

      next = ptr->start;
    } else {
      LOG_FATAL(
          "Failed to reallocate vector with elem size: %d bytes, of len: %d, cap: %d. with unsupported Allocator!",
          self->elem_size, self->len, self->capacity);
    }
  }

  return next;
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


void* vec_index_(VecAny s, i32 index) {
  assert(s);

  if UNLIKELY (index < 0 || index >= vec_len(s)) {
    const i32 n = asvecc(s)->size;
    return &pcast(u8, s)[n];
  }

  return &pcast(u8, s)[index];
}
