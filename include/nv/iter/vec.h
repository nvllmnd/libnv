#pragma once

#include "nv/core/intdefs.h"
#include "nv/core_types.h"
#include "nv/memory/alloc.h"

#define Vec(T) __typeof(T)*

/// @brief main typedef for Vec.
typedef void* VecAny;

VecAny vec_new_(MemLayout tlayout, i32 capacity, Allocator alloc);
#define vec_new(T, _cap, _alloc) ((__typeof(T)*)vec_new_(mlayout_new(T), (_cap), (_alloc)))

METHOD
void* vec_insert_back(VecAny self);

#define vec_push(_self, _val)                                          \
  do {                                                                 \
    const auto _v = (_val);                                     \
    static_assert(sizeof(__typeof(*(_self))) == sizeof(__typeof(_v))); \
    auto _elem = vec_insert_back((_self));                      \
    if (_elem) {                                                       \
      memcpy(_elem, &_v, sizeof(__typeof(_v)));                        \
    }                                                                  \
  } while (0)

PURE_FUNC
METHOD
i32 vec_len(const VecAny self);

PURE_FUNC
METHOD
i32 vec_capacity(const VecAny self);

METHOD
PURE_FUNC
const void* vec_cend_(const VecAny self);
#define vec_cend(_self) ((__typeof(*(_self))*)vec_cend_((_self)))

METHOD
PURE_FUNC
void* vec_end_(VecAny self);
#define vec_end(_self) ((__typeof(*(_self))*)vec_end_((_self)))

METHOD
i32 vec_set_len(VecAny self, i32 new_len);

METHOD
i32 vec_grow_to_cap(VecAny self);

/// @brief returns number of free elements before reaching capacity
PURE_FUNC
METHOD
i32 vec_available(const VecAny self);

/// @brief same as [vec_available], but returns size in bytes
PURE_FUNC
METHOD
i32 vec_avail_bytes(const VecAny self);

PURE_FUNC
METHOD
bool vec_is_full(const VecAny self);

PURE_FUNC
METHOD
bool vec_is_empty(const VecAny self);

PURE_FUNC
METHOD
f32 vec_load_factor(const VecAny self);


METHOD
VecAny vec_resize_(VecAny self, i32 new_capacity, Allocator alloc);
#define vec_resize(_self, _new_cap, _alloc) ((__typeof(*(_self))*)vec_resize_((_self), (_new_cap), (_alloc)))

void vec_destroy(VecAny self, Allocator alloc);

void vec_clear(VecAny self);
void vec_clear_zeroed(VecAny self);

#define vec_for_i(_self_, _index_name_) /* convienence macro for iterating over a vec. second parameter is just \
the loop index variable name. [vec_foreach] uses i' by default */                                               \
  for (int _index_name_ = 0; _index_name_ < vec_len((_self_)); _index_name_++)

#define vec_for(_self_) /* same as [vec_for_i] macro, but sets _index_name_ = i*/ vec_for_i(_self_, i)

#define vec_foreach_iter(_self_, _iter_name_) \
  for (__typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_end((_self_)); _iter_name_++)

#define vec_foreach(_self_) vec_foreach_iter(_self_, iter)

#define vec_foreach_iter_const(_self_, _iter_name_) \
  for (const __typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_cend((_self_)); _iter_name_++)

#define vec_foreach_const(_self_) vec_foreach_iter_const(_self_, iter)
