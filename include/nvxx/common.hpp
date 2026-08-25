#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iostream>
#include <new>
#include <type_traits>
#include <utility>

#if __has_builtin(__builtin_trap)
#ifndef EXIT_FATAL
#define EXIT_FATAL() __builtin_trap()
#endif  // ifndef EXIT_FATAL
#ifndef EXIT_FATAL

#include <exception>

#define EXIT_FATAL() std::terminate()

#endif  // ifndef EXIT_FATAL
#endif  // HEDLEY_HAS_BUILTIN(__builtin_trap)

#ifdef NDEBUG
#undef LIBNV_DEBUG
#define LIBNV_DEBUG 0

#ifndef LIBNV_VERBOSE_LOGGING
#define LIBNV_VERBOSE_LOGGING 0
#endif

#else

#undef LIBNV_DEBUG
#define LIBNV_DEBUG 1

#endif

#ifdef assert_debug
#undef assert_debug
#endif

#if LIBNV_DEBUG == 1
#define assert_debug(_expr, _fmt, ...)                                                                             \
  {                                                                                                                \
    if (!(_expr)) {                                                                                                \
      fprintf(stderr, "[%s:%d]::%s => " _fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__); \
      EXIT_FATAL();                                                                                                \
    }                                                                                                              \
  }

#else

#define assert_debug(_expr, _fmt, ...)

#endif

#ifdef __cplusplus

#ifndef ASSERT
#define ASSERT(_expr, _fmt, ...)                                                                                   \
  {                                                                                                                \
    if (!(_expr)) {                                                                                                \
      fprintf(stderr, "[%s:%d]::%s => " _fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(, ) __VA_ARGS__); \
      EXIT_FATAL();                                                                                                \
    }                                                                                                              \
  }
#endif

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
inline namespace nvty {

using u8 = uint8_t;
using i8 = int8_t;
using byte = std::byte;
using u16 = uint16_t;
using i16 = int16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
using usize = size_t;
using isize = ssize_t;

using error = i64;

static constexpr const i8 I8_MAX = INT8_MAX;
static constexpr const i8 I8_MIN = INT8_MIN;
static constexpr const u8 U8_MAX = UINT8_MAX;
static constexpr const i16 I16_MAX = INT16_MAX;
static constexpr const u16 U16_MAX = UINT16_MAX;

static constexpr const i32 I32_MIN = INT32_MIN;
static constexpr const i32 I32_MAX = INT32_MAX;
static constexpr const u32 U32_MAX = UINT32_MAX;
static constexpr const i64 I64_MIN = INT64_MIN;
static constexpr const i64 I64_MAX = INT64_MAX;
static constexpr const u64 U64_MAX = UINT64_MAX;

static constexpr const isize ISIZE_MIN = PTRDIFF_MIN;
static constexpr const isize ISIZE_MAX = PTRDIFF_MAX;

static constexpr const usize USIZE_MAX = SIZE_MAX;

#if defined(__has_include)
#if __has_include(<unistd.h>)

#include <unistd.h>
using isize = ssize_t;
#else
using isize = ptrdiff_t;
#endif
#else
using isize = ptrdiff_t;
#endif

}  // namespace nvty

namespace nv {

using Sview = std::string_view;
using Str = Sview;

template <class T, isize N>
using ArrayLvref = T (&)[N];

template <class T, isize N>
using ArrayRvref = T (&&)[N];

template <class T, isize N>
using ArrayPtr = T (*)[N];

template <class T>
using SlicePtr = T (*)[];

template <isize N>
using StringLiteral = ArrayLvref<const char, N>;

template <isize N>
using RvStringLiteral = ArrayRvref<const char, N>;

template <isize N>
using StringLiteralPtr = ArrayPtr<const char, N>;

template <isize N>
consteval Str static_string(StringLiteral<N> s) noexcept {
  static_assert(N >= 0, "template arguement N must be >= 0!");
  return Str{s, N};
}

template <class... Args>
constexpr void println(Str fmt, Args&&... args) noexcept {
  std::cout << std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)) << "\n";
}

template <class... Args>
constexpr void print(Str fmt, Args&&... args) noexcept {
  std::cout << std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
}

inline namespace typeops {

template <class T>
using RemoveRef = std::remove_reference_t<T>;

template <class T>
using RemovePtr = std::remove_pointer_t<T>;

/// @brief alias for [std::remove_cvref_t]
template <class T>
using RemoveCvref = std::remove_cvref_t<T>;

template <class T>
using RemoveCv = std::remove_cv_t<T>;

template <class T>
using RemoveConst = std::remove_const_t<T>;

template <class T>
using RemoveCvptr = RemovePtr<RemoveCvref<T>>;

template <class T>
using Decay = std::decay_t<T>;

template <class Func, class... Args>
using ReturnType = std::invoke_result_t<Func, Args...>;

namespace priv {

template <class T>
struct RemovePtrRecImpl {
  using Type = RemovePtr<RemoveCv<T>>;
};

template <class T>
struct RemovePtrRecImpl<T*> {
  using Type = typename RemovePtrRecImpl<T>::Type;
};

template <class T>
struct RemoveCvrefRecImpl {
  using Type = typename RemovePtrRecImpl<RemoveCvref<T>>::Type;
};

template <class T>
struct RemoveCvrefRecImpl<T*> {
  using Type = typename RemoveCvrefRecImpl<T>::Type;
};

template <class T>
struct RemoveCvrefRecImpl<const T*> {
  using Type = typename RemoveCvrefRecImpl<T>::Type;
};

template <class T>
struct RemoveCvrefRecImpl<T&> {
  using Type = typename RemoveCvrefRecImpl<T>::Type;
};

template <class T>
struct RemoveCvrefRecImpl<const T&> {
  using Type = typename RemoveCvrefRecImpl<T>::Type;
};

template <class T>
struct RemoveCvrefRecImpl<T&&> {
  using Type = typename RemoveCvrefRecImpl<T>::Type;
};

template <class T>
struct RemoveCvrefRecImpl<const T&&> {
  using Type = typename RemoveCvrefRecImpl<T>::Type;
};

template <class T>
struct UnwrapImpl {
  using Type = RemoveCvptr<T>;
};

template <class T>
struct UnwrapImpl<T*> {
  using Type = typename UnwrapImpl<RemoveCvptr<T>>::Type;
};

template <class T>
struct UnwrapImpl<T&> {
  using Type = typename UnwrapImpl<RemoveCvptr<T>>::Type;
};

template <class T>
struct UnwrapImpl<T&&> {
  using Type = typename UnwrapImpl<RemoveCvptr<T>>::Type;
};

template <class T, isize N>
struct UnwrapImpl<T[N]> {
  using Type = typename UnwrapImpl<std::remove_all_extents_t<T>>::Type;
};

}  // namespace priv

template <class T>
using Unwrap = typename priv::UnwrapImpl<T>::Type;

template <class T>
using ValType = Unwrap<T>;

template <class T>
using RemvoeCvrefRef = typename priv::RemoveCvrefRecImpl<T>::Type;

template <class T>
using RemovePtrRec = typename priv::RemovePtrRecImpl<T>::Type;

// template <class T>
// using Trim = std::remove_all_extents<RemovePtr<RemoveCvref<T>>>;
//
// template <class T>
// using ValType = Atomize<T>;
//
// template <class T>
// using Melt = Decay<std::remove_all_extents_t<RemovePtr<RemoveCvref<T>>>>;
//
// template <class T>
// using Nuke = Atomize<Melt<Decay<RemoveCvref<T>>>>;
//
}  // namespace typeops

/// @brief alias for [RemoveCvref] ([std::remove_cvref_t])

template <class T>
using ptr = std::add_pointer_t<T>;

template <class T>
using const_ptr = ptr<const T>;

using void_ptr = ptr<void>;

template <class T>
[[gnu::pure]]
constexpr bool is_null(const T* ptr) noexcept {
  return nullptr == ptr;
}

template <class T>
[[gnu::pure]]
constexpr bool is_not_null(const T* ptr) noexcept {
  return nullptr != ptr;
}

template <class T>
using lvalue = std::add_lvalue_reference_t<T>;

template <class T>
using const_lvalue = std::add_lvalue_reference_t<std::add_const_t<T>>;

template <class T>
using rvalue = std::add_rvalue_reference_t<T>;

template <class T>
constexpr auto IsRef = std::is_reference_v<T>;

template <class T>
constexpr auto IsLvref = std::is_lvalue_reference_v<T>;

template <class T>
constexpr auto IsRvref = std::is_rvalue_reference_v<T>;

template <class T>
constexpr auto IsPtr = std::is_pointer_v<T>;

/// @details std::launders and reinterpret_casts byte pointer to a standard layout compatible type
/// given pointer must not be null
template <class T>
  requires(std::is_standard_layout_v<T>)
[[gnu::nonnull, gnu::returns_nonnull]]
constexpr ptr<const T> byte_cast(ptr<const byte> p) noexcept {
  assert_debug(is_not_null(p), "Cannot byte_cast a nullptr!");
  return std::launder(reinterpret_cast<nv::ptr<const T>>(p));
}

}  // namespace nv

namespace nv::priv_impl {
template <class Func>
struct Defer {
  Func f;
  bool is_valid = true;

  constexpr Defer(const Defer&) noexcept = delete;
  constexpr Defer(Defer&& other) noexcept : f(std::move(other.f)) { is_valid = false; }
  constexpr Defer(const Func&& f) noexcept : f(std::forward<const Func>(f)) {}

  constexpr ~Defer() noexcept {
    if (is_valid) f();
  }
};

inline constexpr struct {
  template <class Func>
  constexpr Defer<Func> operator<<(const Func&& f) const noexcept {
    return Defer<Func>(std::forward<const Func>(f));
  }
} defer_helper;

}  // namespace nv::priv_impl

#define DEFER_CONCAT(x, y) x##y
#define DEFER_CONCAT_HELPER(x, y) (DEFER_CONCAT(x, y))
#define DEFER_VAR_NAME DEFER_CONCAT_HELPER(s_defer_, __LINE__)

#ifndef defer
#define defer const auto DEFER_VAR_NAME = nv::priv_impl::defer_helper << [&]() noexcept
#endif

#undef DEFER_CONCAT
#undef DEFER_CONCAT_HELPER
#undef DEFER_VAR_NAME

namespace nv {

// template <class T>
// using Atom = std::conditional_t < IsRef << T >>>
//     ;
//
// static_assert(std::is_same_v<i32, ValType<const int*&>>);

#if defined(__cpp_concepts)

// template <class T, class D>
// concept DisplayOut = requires(T a, D* out) {
//   { a.display(out) } noexcept -> std::convertible_to<result::Result<void, typename T::DisplayErrorType>>;
// };
//
// template <class T>
// concept Display = DisplayOut<T, decltype(std::cout)>;
//
// template <class T>
//   requires(Display<T>)
// constexpr Str as_string(const T& v) noexcept {
//   return v.display();
// }

template <class T>
concept IsCvref = std::is_const_v<T> || std::is_volatile_v<T> || std::is_reference_v<T>;

// template <class T>
// concept Iterator = requires(T v) {
//   v++;
//   v--;
//   { *v } -> std::convertible_to<ValType<typename T::ElemType>>;
//   v == v;
//   v != v;
//   v < v;
//   v <= v;
//   v > v;
//   v >= v;
//   // { v.begin() } -> std::convertible_to<ptr<RemoveCvref<typename T::TargetType>>>;
//   // { v.end() } -> std::convertible_to<ptr<RemoveCvref<typename T::TargetType>>>;
// } || std::is_pointer_v<RemoveCvref<T>>;
//
// template <class T>
// concept Iterable = requires(T v) {
//
// };
//
template <class Func, class... Args>
concept Callable = std::is_invocable_r_v<ReturnType<Func, Args...>, Func, Args...>;

/// @details Similar to [Pod], but only needs to satisfy [std::is_trivially_destructible_v] and
/// [std::is_standard_layout_v], as opposed to
// [Pod], which requires [std::is_trivial_v] as well as [std::standard_layout_v]
template <class T>
concept PodLike = std::is_standard_layout_v<T> && std::is_trivially_destructible_v<T>;

/// @brief Standard Layout and Trivial Type
template <class T>
concept Pod = std::is_standard_layout_v<T> && std::is_trivial_v<T>;

/// @brief concept for dynamically sized types for structs with a flexible array member
template <class T>
concept Dst = requires() {
  std::is_unbounded_array_v<typename T::FlexMemberType>;
  typename T::FlexValueType;
};

template <class T>
concept StdLayout = std::is_standard_layout_v<T>;

#else

template <class Func, class... Args>
constexpr bool Callable = std::is_invocable_r_v<ReturnType<Func, Args...>, Func, Args...>;

#endif

}  // namespace nv

#define CONTAINER_TEMPLATE_TYPES_AS(Prefix, T)                              \
  using Prefix##Type = T;                                                   \
  using Prefix##ValueType = std::remove_cvref_t<T>;                         \
  using Prefix##Pointer = std::add_pointer_t<Prefix##ValueType>;            \
  using Prefix##ConstPointer = std::add_pointer_t<const Prefix##ValueType>; \
  using Prefix##Reference = std::add_lvalue_reference_t<Prefix##ValueType>; \
  using Prefix##ConstReference = std::add_lvalue_reference_t<const Prefix##ValueType>;

#define CONTAINER_TEMPLATE_TYPES(T)                         \
  using Type = T;                                           \
  using ValueType = std::remove_cvref_t<T>;                 \
  using Pointer = std::add_pointer_t<ValueType>;            \
  using ConstPointer = std::add_pointer_t<const ValueType>; \
  using Reference = std::add_lvalue_reference_t<ValueType>; \
  using ConstReference = std::add_lvalue_reference_t<const ValueType>;

#define CHECK_AS_IMPL(_expr, _val) \
  if (const auto _opt = (_expr))   \
    for (auto [_val, _running] = std::tuple{_opt.unwrap(), true}; _running; _running = false)

#define CHECK_IMPL(_expr) CHECK_AS_IMPL(_expr, val)

#ifndef checkas

#define checkas(_expr, _val) CHECK_AS_IMPL(_expr, _val)

#else

#define CHECKAS(_expr, _val) CHECK_AS_IMPL(_expr, _val)

#endif

#ifndef check
#define check(_expr) CHECK_IMPL(_expr)
#else
#define CHECK(_expr) CHECK_IMPL(_expr)
#endif

#ifndef check_ensure

#define check_ensure(_expr)                                  \
  if (const auto _opt = (_expr); !static_cast<bool>(_opt)) { \
    return _opt;                                             \
  } else

#endif

#ifndef checkval
#define checkval(_expr)                                       \
  ({                                                          \
    auto _val = decltype((_expr).unwrap()){};                 \
    if (const auto _res = (_expr); static_cast<bool>(_opt)) { \
      _val = _res.unwrap();                                   \
    } else {                                                  \
      return _res;                                            \
    }                                                         \
    _val;                                                     \
  })
#endif
