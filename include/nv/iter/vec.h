#pragma once

#include "nv/iter/buff.h"

#define Vec(T) ptr(T)

#define vec_new(T, _init_capacity_, alloc) ((Vec(T))buff_sized_new(sizeof(T) ,(_init_capacity_), (alloc)))

#define vec_append(self) ((Vec(__typeof(*(self))))buff_append((Buff*)(self), mlayout_new(__typeof(*(self)))))

#define vec_push(SELF, VAL) do {\
  static_assert(sizeof(__typeof(*(SELF))) == sizeof(__typeof((VAL)))); \
  __typeof_unqual((VAL))* _elem = vec_append(SELF); \
  if (_elem) {\
      *_elem = (VAL); \
  }\
} while(0) 


#define vec_len(self) (buff_len(pcast(u8, (self))))
#define vec_capacity(self) (buff_capacity(pcast(u8, (self))))
#define vec_cend(self) ((const __typeof(self))(buff_cend(pcast(const Buff, (self)))))
#define vec_end(self) ((__typeof(self))(buff_end(pcast(Buff, (self)))))

#define vec_set_len(_self_, _new_len_) (buff_set_len((_self_), _new_len_ * sizeof(__typeof(*(_self_)))))

#define vec_grow_to_cap(_self_) (buff_grow_to_cap((Buff*)(_self_)))

#define vec_available(self) (buff_available(pcast(const Buff, (self))) / (i32)sizeof(__typeof(*(self))))

#define vec_needs_resize_for(self, T, n) \
  (buff_needs_space_for(pcast(const Buff, (self)), make(MemLayout, .size = sizeof(T) * (n), .align = alignof(T))))

#define vec_has_space_for(self, T, n) \
  (buff_has_space_for(pcast(const Buff, (self)), make(MemLayout, .size = sizeof(T) * (n), .align = alignof(T))))

#define vec_destroy(self, alloc) (buff_destroy(pcast(Buff, (self)), (alloc)))

#define vec_clear_zeroed(self) (buff_clear_zeroed(pcast(Buff, (self))))

#define vec_clear_zeroed_cap(_self_) (buff_clear_zeroed_cap(pcast(Buff, (_self_))))

#define vec_clear(self) (buff_clear(pcast(Buff, (self))))

#define vec_index(_self_, _i_)                                                                                 \
  ({                                                                                                           \
    const i32 _len_ = vec_len((_self_));                                                                       \
    const i32 _index_ = (_i_);                                                                                 \
    if (_index_ < 0 || _index_ >= _len_) {                                                                     \
      buff_fatal_error((Buff*)(_self_),                                                                        \
                       "Attempted to index vec of length: %d with an index that is out of bounds!: %d", _len_, \
                       _index_);                                                                               \
    }                                                                                                          \
    &(_self_)[_index_];                                                                                        \
  })

#define vec_write(self, out, out_len) \
  (buff_write(pcast(Buff, (self)), (out), (out_len) * (i32)sizeof(__typeof(*(self)))))
#define vec_resize(self, new_capacity, alloc) \
  ((__typeof(*(self))*)(buff_resize(pcast(Buff, (self)), (new_capacity), (alloc))))

#define vec_from_mem(start, end) (buff_from_mem(pcast(u8, (start)), pcast(u8, (end)))
#define vec_is_full(self) (buff_is_full(pcast(const Buff, (self))))

#define vec_is_empty(self) (buff_is_empty(pcast(const Buff, (self))))
#define vec_min_size buff_min_size

#define vec_for_i(_self_, _index_name_) /* convienence macro for iterating over a vec. second parameter is just \
the loop index variable name. [vec_foreach] uses i' by default */                                               \
  for (int _index_name_ = 0; _index_name_ < vec_len((_self_)); _index_name_++)

#define vec_for(_self_) /* same as [vec_for_i] macro, but sets _index_name_ = i*/ vec_for_i(_self_, i)

#define vec_foreach_iter(_self_, _iter_name_) \
  for (__typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_end((_self_)); _iter_name_++)

#define vec_foreach(_self_) vec_foreach_iter(_self_, iter)

#define vec_foreach_iter_const(_self_, _iter_name_) \
  for (const __typeof(*(_self_))* _iter_name_ = _self_; _iter_name_ < vec_end((_self_)); _iter_name_++)

#define vec_foreach_const(_self_) vec_foreach_iter_const(_self_, iter)

