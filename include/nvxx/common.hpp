#pragma once

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

template <isize N>
consteval Str static_string(const char (&s)[N]) noexcept {
  static_assert(N >= 0, "template arguement N must be >= 0!");
  return Str{s, N};
}

template <class... Args>
constexpr void println(Str fmt, Args&&... args) noexcept {
  std::cout << std::vformat(fmt, std::make_format_args(args...)) << "\n";
}

template <class... Args>
constexpr void print(Str fmt, Args&&... args) noexcept {
  std::cout << std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
}

template <class T>
using RemoveRef = std::remove_reference_t<T>;

template <class T>
using RemoveCvref = std::remove_cvref_t<T>;

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

#if defined(__cpp_concepts)
namespace nv {

template <class Func, class... Args>
using ReturnType = std::invoke_result_t<Func, Args...>;

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
  typename T::FlexMemberType;
  typename T::FlexValueType;
} && std::is_unbounded_array_v<typename T::FlexMemberType>;

template <class T>
concept StdLayout = std::is_standard_layout_v<T>;

}  // namespace nv

#endif

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
