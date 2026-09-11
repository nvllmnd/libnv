#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <new>
#include <print>
#include <string_view>
#include <type_traits>
#include <utility>

#define REQ_METHOD(T, x) /* conveinence macro for inline requires to test if T has a method*/ (requires(T a) { a.x(); })

#define REQ_METHOD_RET(T, x, ret) /* same as [REQ_METHOD], but checks that method has a specified return type*/ \
  (requires(T a) {                                                                                              \
    { a.x(); } noexcept -> ret                                                                                  \
  })

#define REQ_TNAME(T, x) /*convience macro for inline requires to test if T has a  nested typename*/ \
  requires { typename T::x; })

#ifdef println
#undef println
#endif

#ifdef print
#undef print
#endif

#ifdef eprintln
#undef eprintln
#endif

#ifdef eprint
#undef eprint
#endif

#ifdef print_fd
#undef print_fd
#endif
#ifdef println_fd
#undef println_fd
#endif

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
using isize = std::make_signed_t<size_t>;

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

/// @brief creates a valid nullptr value for type T
template <class T>
inline constexpr auto Nullptr = static_cast<T*>(nullptr);

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
consteval Str static_str(StringLiteral<N> s) noexcept {
  static_assert(N >= 0, "template arguement N must be >= 0!");
  return Str{s, N};
}

template <class T, isize N>
using Array = std::array<T, N>;

/// @brief base class to be inherited from dervied types that desire Rust-like move-only semantics
struct MoveOnly {
  constexpr MoveOnly() noexcept = default;
  constexpr MoveOnly(MoveOnly&&) noexcept = default;
  constexpr MoveOnly& operator=(MoveOnly&&) noexcept = default;
  constexpr MoveOnly(const MoveOnly&) noexcept = delete;
  constexpr MoveOnly& operator=(const MoveOnly&) noexcept = delete;
};

#ifdef __cpp_lib_print

using std::println;

using std::print;

/// @brief error version of [println]
/// @details writes to [stderr] instead of [stdout]
template <class... Args>
constexpr void eprintln(const std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::println(stderr, fmt, std::forward<Args>(args)...);
}

/// @brief error version of [print]
/// @details writes to [stderr] instead of [stdout]
template <class... Args>
constexpr void eprint(const std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::print(stderr, fmt, std::forward<Args>(args)...);
}

#else

template <class... Args>
constexpr void println(const std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::cout << std::vformat(fmt.get(), std::make_format_args(std::forward<Args>(args)...)) << "\n";
}

template <class... Args>
constexpr void print(const std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::cout << std::vformat(fmt.get(), std::make_format_args(std::forward<Args>(args)...));
}

/// @brief error version of [println]
/// @details writes to [stderr] instead of [stdout]
template <class... Args>
constexpr void eprintln(const std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::clog << std::vformat(fmt.get(), std::make_format_args(std::forward<Args>(args)...)) << "\n";
}

/// @brief error version of [print]
/// @details writes to [stderr] instead of [stdout]
template <class... Args>
constexpr void eprint(const std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::clog << std::vformat(fmt.get(), std::make_format_args(std::forward<Args>(args)...));
}

#endif

template <class T, usize N = alignof(T)>
constexpr bool is_aligned(T* ptr) {
  return std::bit_cast<std::uintptr_t>(ptr) % N == 0;
}
template <std::size_t N, class T>
constexpr bool is_aligned(T* ptr) {
  return is_aligned<T, N>(ptr);
}

template <class... Args>
[[noreturn]]
constexpr void log_fatal(const std::format_string<Args...> fmt = "Fatal Error!", Args&&... args) noexcept {
  eprintln(fmt, std::forward<Args>(args)...);
  std::terminate();
}

template <class T>
constexpr void* voidify(T* ptr) noexcept {
  return static_cast<void*>(ptr);
}

template <class T>
constexpr std::remove_reference_t<T>* ptr_cast(void* ptr) noexcept {
  return static_cast<std::remove_reference_t<T>*>(ptr);
}

template <class T>
  requires(std::is_standard_layout_v<T>)
constexpr std::remove_reference_t<T>* ptr_cast(byte* ptr) noexcept {
  return static_cast<std::remove_reference_t<T>*>(ptr);
}

inline namespace typeops {

template <bool Condition, class Then, class Else>
using Cond = std::conditional_t<Condition, Then, Else>;

/// @brief alias for [std::remove_reference]
template <class T>
using RemoveRef = std::remove_reference_t<T>;

/// @brief alias for [std::remove_reference]
template <class T>
using RemRef = RemoveRef<T>;

/// @brief alias for [std::remove_reference]
template <class T>
using Unref = RemoveRef<T>;

template <class T>
using RemovePtr = std::remove_pointer_t<T>;

template <class T>
using RemovePref = RemovePtr<RemoveRef<T>>;

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
using RemoveVolRef = std::remove_volatile_t<RemovePtr<RemoveRef<T>>>;

template <class T>
using Decay = std::decay_t<T>;

template <class T>
using TypeId = std::type_identity_t<T>;

template <class T>
using DecayRef = Cond<std::is_reference_v<T> || std::is_array_v<T>, Decay<T>, T>;

// static_assert()

template <class T>
using Prune = RemovePtr<Decay<RemoveRef<T>>>;

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
struct ValTypeImpl {
  using Type = T;
};

template <class T>
struct ValTypeImpl<T*> {
  using Type = typename ValTypeImpl<T>::Type;
};

template <class T>
struct ValTypeImpl<T&> {
  using Type = typename ValTypeImpl<T>::Type;
};

template <class T>
struct ValTypeImpl<T&&> {
  using Type = typename ValTypeImpl<T>::Type;
};

template <class T>
struct ValTypeImpl<T[]> {
  using Type = typename ValTypeImpl<std::remove_all_extents_t<T>>::Type;
};

template <class T, isize N>
struct ValTypeImpl<T[N]> {
  using Type = typename ValTypeImpl<std::remove_all_extents_t<T>>::Type;
};

}  // namespace priv

/// @brief Removes all qualifiers from T, maintaining const/volatility
template <class T>
using ValType = typename priv::ValTypeImpl<T>::Type;

/// @brief gets T's bare scalar type with no cv or any qualifiers on it
/// @details does not maintain constness or volatility. for a version of this that does, @see [ValType]
template <class T>
using Unwrap = std::remove_cv_t<ValType<T>>;

template <class T>
using RemoveCvrefRec = typename priv::RemoveCvrefRecImpl<T>::Type;

template <class T>
using RemovePtrRec = typename priv::RemovePtrRecImpl<T>::Type;

}  // namespace typeops

/// @brief adds pointer to T
/// @details useful for writing templates, otherwise obfuscates pointer types in an unclear manner, so try to use
/// sparingly
template <class T>
using ptr = std::add_pointer_t<T>;

template <class T>
using const_ptr = ptr<const T>;

using void_ptr = void*;

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
  requires(std::is_standard_layout_v<Unwrap<T>>)
[[gnu::nonnull, gnu::returns_nonnull]]
inline ptr<Unwrap<T>> byte_cast(byte* p) noexcept {
  assert_debug(is_not_null(p), "Cannot byte_cast a nullptr!");
  return std::launder(reinterpret_cast<ptr<Unwrap<T>>>(p));
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

#if defined(__cpp_concepts)

template <class T>
concept Cursorlike = requires(const T& a) {
  { a.cursor() } noexcept -> std::convertible_to<typename T::IterType>;
  { a.begin() } noexcept -> std::convertible_to<typename T::IterType>;
  { a.end() } noexcept -> std::convertible_to<typename T::IterType>;
};

template <class T>
concept IsRef = std::is_reference_v<T>;

template <class T>
concept IsVolatile = std::is_volatile_v<T>;

template <class T>
concept IsLvref = std::is_lvalue_reference_v<T>;

template <class T>
concept IsRvref = std::is_rvalue_reference_v<T>;

template <class T>
concept IsPtr = std::is_pointer_v<T>;

template <class T>
concept IsConst = std::is_const_v<T>;

template <class T>
concept IsCvref = IsConst<T> || IsVolatile<T> || IsRef<T>;

template <class T>
concept IsVoid = std::is_void_v<T>;

template <class T>
concept Copy = (std::assignable_from<lvalue<Unwrap<T>>, const Unwrap<T>&> ||
                std::constructible_from<lvalue<Unwrap<T>>, const Unwrap<T>&>);

template <class T>
concept Clone = requires(T a) {
  { a.clone() } noexcept -> std::same_as<Unwrap<T>>;
} && Copy<T>;

template <class T>
concept IsObject = std::is_object_v<T>;

template <class T>
concept IsArray = std::is_array_v<T>;

template <class T>
concept IsUnbounded = std::is_unbounded_array_v<T>;

template <class T>
concept IsEnum = std::is_enum_v<T>;

template <class T>
concept IsStruct = std::is_class_v<T>;

template <class T>
concept IsUnion = std::is_union_v<T>;

template <class T>
concept Default = std::default_initializable<T>;

template <class T>
concept Eq = std::equality_comparable<T>;

template <class Func, class... Args>
concept Callable = std::is_invocable_r_v<ReturnType<Func, Args...>, Func, Args...>;
/// @brief Standard Layout and Trivial Type
template <class T>
concept Pod = std::is_standard_layout_v<T> && std::is_trivially_destructible_v<T>;

/// @brief concept for dynamically sized types for structs with a flexible array member
template <class T>
concept Dst = Pod<typename T::FlexValueType> && std::is_unbounded_array_v<typename T::FlexMemberType>;

template <class T>
concept StdLayout = std::is_standard_layout_v<T>;

#else

template <class Func, class... Args>
constexpr bool Callable = std::is_invocable_r_v<ReturnType<Func, Args...>, Func, Args...>;

#endif

/// @brief invokes [is_empty] method on value of T, if it exists
/// If T does not have an is_empty method, you can use [is_zeroed] instead
template <class T>
  requires requires(const T& a) {
    { a.is_empty() } noexcept -> std::same_as<bool>;
  }
constexpr bool is_empty(const T& self) noexcept {
  return self.is_empty();
}

/// @brief Equality compares given value with a zeroed (default) instance of T on the stack
/// @details T must be default initializable and equality comparible with itself (const T&) in order to match this
/// overload
template <class T>
  requires(Default<T> && Eq<T>)
constexpr bool is_zeroed(const T& self) noexcept {
  static constexpr const Unref<T>& EMPTY{};
  return self == EMPTY;
}

/// @brief compar s given T value with a zeroed instance of T using [std::memcmp]
template <class T>
  requires(!Eq<T> && Default<T> && StdLayout<T>)
constexpr bool is_zeroed(const T& self) noexcept {
  static constexpr const Unref<T>& EMPTY{};
  return std::memcmp(&self, &EMPTY, sizeof(Unref<T>)) == 0;
}
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
