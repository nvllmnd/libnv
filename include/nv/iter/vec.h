// SPDX-FileCopyrightText: 2026 Matthew McDade <nvllmnd@pm.me>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "nv/core/attributes.h"
#include "nv/memory/alloc.h"

BEGIN_C_DECLS

#define Vec(T) __typeof(T)*

/// @brief main typedef for Vec.
typedef void* VecAny;
typedef const void* ConstVecAny;

VecAny vec_new_(Layout tlayout, i32 capacity, Allocator alloc);
#define vec_new(T, _cap, _alloc) ((__typeof(T)*)vec_new_(mlayout_new(T), (_cap), (_alloc)))

METHOD
void* vec_insert_back(VecAny self);

// #define vec_push(_self, _val)                                          \
//   do {                                                                 \
//     const auto _v = (_val);                                     \
//     static_assert(sizeof(__typeof(*(_self))) == sizeof(__typeof(_v))); \
//     auto _elem = vec_insert_back((_self));                      \
//     if (_elem) {                                                       \
//       memcpy(_elem, &_v, sizeof(__typeof(_v)));                        \
//     }                                                                  \
//   } while (0)

METHOD
RETURNS_NON_NULL
void* vec_index_(VecAny self, i32 index);
#define vec_index(_self, _index) ((__typeof(*(_self))*)vec_index_((_self), (_index)))

PURE_FUNC
METHOD
i32 vec_len(ConstVecAny s);

PURE_FUNC
METHOD
i32 vec_capacity(ConstVecAny self);

METHOD
PURE_FUNC
RETURNS_NON_NULL
const void* vec_cend_(ConstVecAny self);
#define vec_cend(_self) ((__typeof(*(_self))*)vec_cend_((_self)))

METHOD
RETURNS_NON_NULL
void* vec_end_(VecAny self);
#define vec_end(_self) ((__typeof(*(_self))*)vec_end_((_self)))

METHOD
i32 vec_set_len(VecAny self, i32 new_len);

METHOD
i32 vec_grow_to_cap(VecAny self);

/// @brief returns number of free elements before reaching capacity
PURE_FUNC
METHOD
i32 vec_available(ConstVecAny self);

/// @brief same as [vec_available], but returns size in bytes
PURE_FUNC
METHOD
i32 vec_avail_bytes(ConstVecAny self);

PURE_FUNC
METHOD
bool vec_is_full(ConstVecAny self);

PURE_FUNC
METHOD
bool vec_is_empty(ConstVecAny self);

PURE_FUNC
METHOD
f32 vec_load_factor(ConstVecAny self);

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

#define vec_push(_self, _val)                                          \
  do {                                                                 \
    const auto _v = (_val);                                            \
    static_assert(sizeof(__typeof(*(_self))) == sizeof(__typeof(_v))); \
    if (!vec_is_full((_self))) {                                       \
      const i32 i = vec_len((_self));                                  \
      (_self)[i] = _v;                                                 \
      vec_set_len((_self), i + 1);                                     \
    }                                                                  \
  } while (0)

/// #####################################################
/// ################# Auto Resize Vec ###################
/// #####################################################

#define VecDyn(T)    \
  struct {           \
    Vec(T) data;     \
    Allocator alloc; \
  }

#define dvec_new(T, _cap, _alloc)                    \
  ({                                                 \
    const auto _capacity = (_cap);                   \
    auto _allocator = (_alloc);                      \
    auto _self = vec_new(T, _capacity, _allocator);  \
    (VecDyn(T)){.data = _self, .alloc = _allocator}; \
  })

#define dvec_index(_self, _index) (vec_index((_self).data, (_index)))
#define dvec_len(_self) (vec_len((_self).data))
#define dvec_capacity(_self) (vec_capacity((_self).data))
#define dvec_cend(_self) (vec_cend((_self).data))

#define dvec_end(_self) (vec_end((_self).data))
#define dvec_set_len(_self, _new_len) (vec_set_len((_self).data, (_new_len)))
#define dvec_grow_to_cap(_self) (vec_grow_to_cap((_self).data))
#define dvec_available(_self) (vec_available((_self).data))
#define dvec_avail_bytes(_self) (vec_avail_bytes((_self).data))
#define dvec_is_full(_self) (vec_is_full((_self).data))
#define dvec_is_empty(_self) (vec_is_empty((_self).data))
#define dvec_load_factor(_self) (vec_load_factor((_self).data))
#define dvec_resize(_self, _new) (vec_resize((_self).data, (_new), (_self).alloc))

#define dvec_push(_self, _val)          \
  do {                                  \
    auto _s = (_self).data;             \
    auto _alloc = (_self).alloc;        \
    const auto _len = vec_len(_s);      \
    if (_len >= vec_capacity(_s)) {     \
      vec_resize(_s, _len * 3, _alloc); \
    }                                   \
    vec_push(_s, _val);                 \
  } while (0)

#define dvec_destroy(_self) (vec_destroy((_self).data, (_self).alloc))
#define dvec_clear(_self) (vec_clear((_self).data))
#define dvec_clear_zeroed(_self) (vec_clear_zeroed((_self).data))

#define dvec_for_i(_self, _index_name) vec_for_i((_self).data, _index_name)
#define dvec_for(_self) vec_for((_self).data)
#define dvec_foreach_iter(_self, _iter_name) vec_foreach_iter((_self).data, _iter_name)
#define dvec_foreach(_self) vec_foreach((_self).data)

#define dvec_foreach_iter_const(_self, _iter_name) vec_foreach_iter_const((_self).data, _iter_name)
#define dvec_foreach_const(_self) vec_foreach_const((_self).data)

END_C_DECLS

#ifdef __cplusplus

template <typename T, typename A>
struct Vec {
  Vec(T) inner;
  A alloc;
};

#endif
