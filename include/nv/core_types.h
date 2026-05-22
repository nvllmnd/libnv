#pragma once

#include <math.h>
#include <stdint.h>

#include "nv/core/attributes.h"
#include "nv/core/intdefs.h"
#include "nv/memory/error.h"

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

#define typeof_ptr(_p) __typeof(*(_p))*

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

#define size_of(_e) /* Same as the standard keyword 'sizeof', but may be a little more 'correct' and 'type-safe'(ish), \
                       as it wraps the target of sizeof in __typeof first */                                           \
  sizeof(__typeof((_e)))

#define align_of(_e) /*Same as the standard keyword 'alignof', but may be a little more 'correct' and \
                        'type-safe'(ish), as it wraps the target of alignof in __typeof first  */     \
  alignof(__typeof((_e)))

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

#define cmin(_a_, _b_)                          \
  ({                                            \
    constexpr const __typeof__(_a_) _a = (_a_); \
    constexpr const __typeof__(_b_) _b = (_b_); \
    _a < _b ? _a : _b;                          \
  })

#define cmax(_a_, _b_)                          \
  ({                                            \
    constexpr const __typeof__(_a_) _a = (_a_); \
    constexpr const __typeof__(_b_) _b = (_b_); \
    _a > _b ? _a : _b;                          \
  })

#define min(_a_, _b_)                 \
  ({                                  \
    const __typeof__(_a_) _a = (_a_); \
    const __typeof__(_b_) _b = (_b_); \
    _a < _b ? _a : _b;                \
  })

#define max(_a_, _b_)                 \
  ({                                  \
    const __typeof__(_a_) _a = (_a_); \
    const __typeof__(_b_) _b = (_b_); \
    _a > _b ? _a : _b;                \
  })

#define is_null(_p)                                   \
  /* checks if given pointer (p) is equal to null. */ \
  (nullptr == (_p))

#define is_not_null(_p)                                  \
  /* checks if given pointer (p) is not equal to null */ \
  (!(is_null((_p))))

#define clamp(_x, _min, _max)                                          \
  /* clamps a value values (x) to be between (min) and (max) */        \
  /* i.e.: 'clamp(-1, 0, 5) == 0;', or 'clamp(650, 0, 100) == 100;' */ \
  (max((_min), min((_x), (_max))))

// NOTE: I kind of like these 2 macros in a guilty pleasure kind of way lmao...
//  i might one day use them, but idk its kinda ugg and seems too distant to C
//  for it to make any sense to other developers coming across it while reading
//  this codebase
//  #define deref(p) (*(p))
//  #define ref &
//

#define UNUSED(_v) ((void)_v)

#define make(T, ...) /* Conveinence macro for creating new structs on stack. its possible \
                        to pass a value instead of a type as the first parameter to this  \
                        macro. the type of the resulting struct will be the type of value \
                        given. Note that this does not do anything with the value, and    \
                        does not create a copy of the value passed in (if any)*/          \
  ((__typeof__(T)){__VA_ARGS__})

#define make_zeroed(T) /* Same as [make] macro, but initializes given type T's \
                          fields to all be set to 0. */                        \
  (make(T))

/// Offsetof polyfill
#ifndef offsetof
#define offsetof(T, _m) ((isize) & ((T*)0)->_m)
#endif

#define sizeof_field(T, _name_) (sizeof(__typeof(make_zeroed(T)._name_)))

#define alias(T) /* conveinence macro for defining structs to avoid having to \
                    write out the struct name 3 times*/                       \
  typedef struct T T

#define tryerr(_expr)                                                        \
  /* evaluates given expression that returns [error] (int), and returns from \
   * surrounding function with the error value if it is no equal to 0. This  \
   * macro can only be used inside functions that return [error](int) */     \
  do {                                                                       \
    const error _err = (_expr);                                              \
    if (_err != 0) {                                                         \
      return _err;                                                           \
    }                                                                        \
  } while (0)

// #define tryerr_or(expr, orelse) do {\
//     const error _err = (expr); \
//     if (_er != 0) { (orelse); }\
// } while(0)

#define tryerr_or(_expr, _orelse)                                             \
  /* Same as [tryerr] macro, but instead of returning error value in the case \
   * it is not equal to 0, a given expression is ran. you can use this macro  \
   * anywhere in the case you want to handle an error dynamicaly inside a     \
   * function that does not return [error](int)*/                             \
  do {                                                                        \
    const error _err = (_expr);                                               \
    if (_err != 0) {                                                          \
      (_orelse);                                                              \
    }                                                                         \
  } while (0)

#define tryerr_or_cb(_expr, _cb, ...)                                          \
  /* Same as [tryerr_or], but instead of running a given expression in the     \
   * case where error is not equal to 0, a given callback function is called.  \
   * Callback function can have any signature, as long as it has at least a    \
   * parameter of type [error](int) as its first parameter. extra parameters   \
   * to this macro are forwarded to error handling callback function. For a    \
   * version of this macro that returns from the surrounding function with the \
   * return value of given callback, see: [tryerr_or_ret]*/                    \
  do {                                                                         \
    const error _err = (_expr);                                                \
    if (_err != 0) {                                                           \
      (_cb)(_err, __VA_ARGS__);                                                \
    }                                                                          \
  } while (0)

#define tryerr_or_ret(_expr, _cb, ...)                                 \
  /* Same as [tryerr_or_cb], but returns from the surrounding function \
     with the value returned by given callback function. Due to this,  \
     this macro can only be called inside functions with the same      \
     return type as the surrounding function. */                       \
  (tryerr_or_cb((_expr), (_cb), __VA_ARGS__))

// #define tryerr_orelse(_try_expr, _else_expr, _def) ({ (_try_expr) != OK ? (_else_expr) : (_def); })

#define tptr_new(p, enable) (__typeof((p)))(((addr)(p)) | ((enable) ? 1 : 0))
#define tptr_ptr(p) ((__typeof((p)))((addr)(p) & ~1UL))
#define tptr_tag(p) (((addr)(p)) & 1)

#define prefix_offset(_ptr, T) (&((pcast(T, (_ptr)))[-1]))

#define bitset(_set, _flag) ((_set) |= (_flag))

#define bitclear(_set, _flag) ((_set) &= ~(_flag))
#define bittoggle(_set, _flag) ((_set) ^= (_flag))
#define bithas(_set, _flag) (cast(bool, (_set) & (_flag)))

#define bithasall(_set, _flags) (((_set) & (_flags)) == (_flags))
#define bithasany(_set, _flags) ((_set) & (_flags))
