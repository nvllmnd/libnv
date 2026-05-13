#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "attributes.h"
#include "intdefs.h"
#include "memory/error.h"

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define CONCAT3_(a, b, c) a##b##c
#define CONCAT3(a, b, c) CONCAT3_(a, b, c)



#define array(T, N)                                                    \
  /* conveinence for declaring static array of type (T) of size (N) */ \
  __typeof__(T[N])

#define ptr(T)                                                 \
  /* conveinence for declaring pointer types. bye, bye '*'! */ \
  __typeof__(T)*


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

#define is_ptr_ok(p)                                                           \
  /* check if given pointer (p) is good. (converts to a non-negative, non-zero \
   * integer) */                                                               \
  /* shorthand for [alloc_result] for checking pointers returned by            \
   * [Allocator] interface struct [AllocVTable] methods                        \
   */                                                                          \
  (((isize)(p)) > 0)

#define clamp(x, _min, _max)                                           \
  /* clamps a value values (x) to be between (min) and (max) */        \
  /* i.e.: 'clamp(-1, 0, 5) == 0;', or 'clamp(650, 0, 100) == 100;' */ \
  (max((_min), min((x), (_max))))

// NOTE: I kind of like these 2 macros in a guilty pleasure kind of way lmao...
//  i might one day use them, but idk its kinda ugg and seems too distant to C
//  for it to make any sense to other developers coming across it while reading
//  this codebase
//  #define deref(p) (*(p))
//  #define ref &
//

#define UNUSED(v) ((void)v)

#define make(T, ...) /* Conveinence macro for creating new structs on stack. its possible \
                        to pass a value instead of a type as the first parameter to this  \
                        macro. the type of the resulting struct will be the type of value \
                        given. Note that this does not do anything with the value, and    \
                        does not create a copy of the value passed in (if any)*/          \
  ((__typeof__(T)){__VA_ARGS__})

#define make_zeroed(T) /* Same as [make] macro, but initializes given type T's \
                          fields to all be set to 0. */                        \
  (make(T))

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

// static inline void* align_ptr(const void* ptr, isize align)
// WHERE(IS_POWER_OF_2(align)) {
//   const isize offset = ptr_align_offset(ptr, align);
//   const isize adjust = (offset == 0 ? 0 : align - offset);
//   const u64ptr p = cast(u64ptr, ptr);
//   return pcast(void, p + adjust);
// }

/// Checks if a given pointer is aligned to given alignment.
/// @param (align) MUST BE A POWER OF 2. If it is not this funciton returns
/// false
static inline bool ptr_is_aligned(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align)) {
  const auto addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;
  return (addr & mask) == 0;
}

/// Aligns pointer up to given alignment, or returns the same pointer if it
/// already is aligned
/// @param (align) MUST BE A POWER OF 2.  If it is not, then this function
/// returns the exact same pointer, doing no calulations and possiby causing
/// confusion if given pointer is misaligned
static inline const void* align_ptr(const void* ptr, isize align) WHERE(IS_POWER_OF_2(align)) {
  if (ptr_is_aligned(ptr, align)) {
    return ptr;
  }

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

  return pcast(void, aligned);
}
#define align_ptr(p, align) ((__typeof__(p))align_ptr(p, align))

/// Same as [align_ptr], but returns an error if any errors may occur
static inline ApiError try_align_ptr(const void** ptr_out, isize align) {
  if (!IS_POWER_OF_2(align)) {
    return ApiError__ParameterValueNotPowerOf2;
  }
  if (is_null(ptr_out)) {
    return ApiError__NullParameter;
  }

  const void* ptr = *ptr_out;

  const uintptr_t addr = cast(uintptr_t, ptr);
  const uintptr_t mask = align - 1;

  const uintptr_t aligned = (addr + mask) & (~mask);

  *ptr_out = cast(void*, aligned);
  return OK;
}

#define alias(T) /* conveinence macro for defining structs to avoid having to \
                    write out the struct name 3 times*/                       \
  typedef struct T T

#define tryerr(expr)                                                         \
  /* evaluates given expression that returns [error] (int), and returns from \
   * surrounding function with the error value if it is no equal to 0. This  \
   * macro can only be used inside functions that return [error](int) */     \
  do {                                                                       \
    const error _err = (expr);                                               \
    if (_err != 0) {                                                         \
      return _err;                                                           \
    }                                                                        \
  } while (0)

// #define tryerr_or(expr, orelse) do {\
//     const error _err = (expr); \
//     if (_er != 0) { (orelse); }\
// } while(0)

#define tryerr_or(expr, orelse)                                               \
  /* Same as [tryerr] macro, but instead of returning error value in the case \
   * it is not equal to 0, a given expression is ran. you can use this macro  \
   * anywhere in the case you want to handle an error dynamicaly inside a     \
   * function that does not return [error](int)*/                             \
  do {                                                                        \
    const error _err = (expr);                                                \
    if (_err != 0) {                                                          \
      (orelse);                                                               \
    }                                                                         \
  } while (0)

#define tryerr_or_cb(expr, cb, ...)                                            \
  /* Same as [tryerr_or], but instead of running a given expression in the     \
   * case where error is not equal to 0, a given callback function is called.  \
   * Callback function can have any signature, as long as it has at least a    \
   * parameter of type [error](int) as its first parameter. extra parameters   \
   * to this macro are forwarded to error handling callback function. For a    \
   * version of this macro that returns from the surrounding function with the \
   * return value of given callback, see: [tryerr_or_ret]*/                    \
  do {                                                                         \
    const error _err = (expr);                                                 \
    if (_err != 0) {                                                           \
      (cb)(_err, __VA_ARGS__);                                                 \
    }                                                                          \
  } while (0)

#define tryerr_or_ret(expr, cb, ...)                                   \
  /* Same as [tryerr_or_cb], but returns from the surrounding function \
     with the value returned by given callback function. Due to this,  \
     this macro can only be called inside functions with the same      \
     return type as the surrounding function. */                       \
  (tryerr_or_cb((expr), (cb), __VA_ARGS__))



#define tptr_new(p, enable) (__typeof((p)))(((addr)(p)) | ((enable) ? 1 : 0))
#define tptr_ptr(p) ((__typeof((p)))((addr)(p) & ~1UL))
#define tptr_tag(p) (((addr)(p)) & 1)



#define prefix_offset(ptr, T) (&((pcast(T, (ptr)))[-1]))


#define bitset(set, flag) ((set) |= (flag))

#define bitclear(set, flag) ((set) &= ~(flag))
#define bittoggle(set, flag) ((set) ^= (flag))
#define bithas(set, flag) (cast(bool, (set) & (flag)))

#define bithasall(set, flags) (((set) & (flags)) == (flags))
#define bithasany(set, flags) ((set) & (flags))
