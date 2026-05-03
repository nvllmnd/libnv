#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "attributes.h"
#include "intdefs.h"
#include "mimalloc.h"

#define array(T, N)                                                    \
  /* conveinence for declaring static array of type (T) of size (N) */ \
  __typeof__(T[N])

#define ptr(T)                                                 \
  /* conveinence for declaring pointer types. bye, bye '*'! */ \
  __typeof__(__typeof_unqual__(T)*)

#define const_ptr(T)                                                           \
  /* same as [ptr] macro, is more clear that this pointer is a constant one */ \
  __typeof__(const __typeof_unqual__(T)*)

#define type_eq(a, b)                                                         \
  /* compares an expression (a) with given type (b) to see if their types are \
   * the same */                                                              \
  _Generic((a), __typeof__(b): true, default: false)

#define assert_type_eq(a, b)                                               \
  /* same as [type_eq] macro, but fails a static assertion if types do not \
   * match */                                                              \
  static_assert(type_eq(a, b))

#define is_string(s)                                                      \
  /* checks to see if s is of type [const char*] */                       \
  /* Do not that this will return false if given string is a mutable char \
   * buffer (char*) */                                                    \
  type_eq((s), char*)

#define cast(T, _src)                                                          \
  /* casts expression _src to be of type T */                                  \
  /* for a version of this macro that is specialized for casting pointers, see \
   * [pcast]*/                                                                 \
  ((__typeof__(T))(_src))

#define pcast(T, _ptr)                                                    \
  /* casts a given pointer (_ptr) to be of type T */                      \
  /* for a version of this macro that just does general casts between any \
   * given type and an expresion; see: [cast]*/                           \
  (cast(__typeof__(T*), (_ptr)))

#define fmin(a, b)                     \
  (cast(__typeof__((a)), _Generic((a), \
            i8: fmin,                  \
            u8: fmin,                  \
            i16: fmin,                 \
            u16: fmin,                 \
            i32: fmin,                 \
            u32: fmin,                 \
            i64: fmin,                 \
            u64: fmin,                 \
            f32: fmin,                 \
            f64: fmin,                 \
            f128: fminl)(a, b)))

#define fmax(a, b)                     \
  (cast(__typeof__((a)), _Generic((a), \
            i8: fmax,                  \
            u8: fmax,                  \
            i16: fmax,                 \
            u16: fmax,                 \
            i32: fmax,                 \
            u32: fmax,                 \
            i64: fmax,                 \
            u64: fmax,                 \
            f32: fmax,                 \
            f64: fmax,                 \
            f128: fmaxl)(a, b)))

#define cmin(a, b)                          \
  ({                                        \
    constexpr const __typeof__(a) _a = (a); \
    constexpr const __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;                      \
  })

#define cmax(a, b)                          \
  ({                                        \
    constexpr const __typeof__(a) _a = (a); \
    constexpr const __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;                      \
  })

#define min(a, b)                 \
  ({                              \
    const __typeof__(a) _a = (a); \
    const __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;            \
  })

#define max(a, b)                 \
  ({                              \
    const __typeof__(a) _a = (a); \
    const __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;            \
  })

#define is_null(p)                                    \
  /* checks if given pointer (p) is equal to null. */ \
  (nullptr == (p))

#define is_not_null(p)                                   \
  /* checks if given pointer (p) is not equal to null */ \
  (!(is_null((p))))

#define is_ptr_ok(p)                                                                                                   \
  /* check if given pointer (p) is good. (converts to a non-negative, non-zero integer) */                             \
  /* shorthand for [alloc_result] for checking pointers returned by [Allocator] interface struct [AllocVTable] methods \
   */                                                                                                                  \
  (((isize)(p)) > 0)

#define fclamp(x, _min, _max)                                          \
  /* clamps a value values (x) to be between (min) and (max) */        \
  /* i.e.: 'clamp(-1, 0, 5) == 0;', or 'clamp(650, 0, 100) == 100;' */ \
  (max((_min), min((x), (_max))))

// NOTE: I kind of like these 2 macros in a guilty pleasure kind of way lmao...
//  i might one day use them, but idk its kinda ugg and seems too distant to C for
//  it to make any sense to other developers coming across it while reading
//  this codebase
//  #define deref(p) (*(p))
//  #define ref &
//

#define UNUSED(v) ((void)v)

#define make(T, ...) /* Conveinence macro for creating new structs on stack. its possible to pass a value instead of a \
                        type as the first parameter to this macro. the type of the resulting struct will be the type   \
                        of value given. Note that this does not do anything with the value, and does not create a copy \
                        of the value passed in (if any)*/                                                              \
  ((__typeof__(T)){__VA_ARGS__})

#define make_zeroed(T) /* Same as [make] macro, but initializes given type T's fields to all be set to 0. */ (make(T))

PARAMS_NONNULL(1)
static inline void* move(void** from) {
  void* tmp = *from;
  *from = nullptr;
  return tmp;
}
#define move(from) (move((void**)&from))

PARAMS_NONNULL(1, 2)
static inline void* move_into(void** from, void** to) {
  *to = move(*from);
  return *to;
}
#define move_into(from, to) (move_into((void**)&from, (void**)&to))

PARAMS_NONNULL(1, 2)
static inline void* move_exchange(void** obj, void** new_value) {
  void* tmp = *obj;
  *obj = *new_value;
  return tmp;
}
#define move_exchange(from, to) (move_exchange((void**)&from, (void**)&to))

/// Offsetof polyfill
#ifndef offsetof
#define offsetof(T, m) ((isize) & ((T*)0)->m)
#endif

#define IS_POWER_OF_2(n) ((n & (n - 1)) == 0)

CONST_FUNC
static inline bool is_power_of_2(isize n) { return IS_POWER_OF_2(n); }

static inline isize ptr_align_offset(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align)) {
  if LIKELY (IS_POWER_OF_2(align)) {
    const u64ptr mask = align - 1;
    return cast(isize, cast(u64ptr, ptr) & mask);
  }
  return 0;
}

// static inline void* align_ptr(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align)) {
//   const isize offset = ptr_align_offset(ptr, align);
//   const isize adjust = (offset == 0 ? 0 : align - offset);
//   const u64ptr p = cast(u64ptr, ptr);
//   return pcast(void, p + adjust);
// }

static inline bool ptr_is_aligned(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align)) {
  const auto addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;
  return (addr & mask) == 0;
}

static inline const void* align_ptr(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align)) {
  if (ptr_is_aligned(ptr, align)) {
    return ptr;
  }

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

  return cast(void*, aligned);
}

#define align_ptr(p, align) ((__typeof__(p))align_ptr(p, align))

#define alias(T) typedef struct T T

// Thanks mimalloc! :D
// .. and align within the allocation
// const uintptr_t align_mask = alignment - 1;  // for any x, `(x & align_mask) == (x % alignment)`
// const uintptr_t poffset = ((uintptr_t)p + offset) & align_mask;
// const uintptr_t adjust  = (poffset == 0 ? 0 : alignment - poffset);
// void* aligned_p = (void*)((uintptr_t)p + adjust);
