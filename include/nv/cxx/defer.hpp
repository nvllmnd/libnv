#pragma once

#include <utility>

namespace nv::priv_impl {
template <typename Func>
struct Defer {
  Func f;
  bool isValid = true;

  Defer(const Defer&) = delete;
  constexpr Defer(Defer&& other) noexcept: f(std::move(other.f)) { isValid = false; }
  constexpr Defer(const Func&& f) noexcept: f(std::forward<const Func>(f)) {}

  ~Defer() noexcept {
    if (isValid) f();
  }
};

static constexpr struct {
  template <typename Func>
  constexpr Defer<Func> operator<<(const Func&& f) const noexcept {
    return Defer<Func>(std::forward<const Func>(f));
  }
} defer_helper;
}  // namespace nv::priv_impl

#define DEFER_CONCAT(x, y) x##y
#define DEFER_CONCAT_HELPER(x, y) (DEFER_CONCAT(x, y))
#define DEFER_VAR_NAME DEFER_CONCAT_HELPER(s_defer_, __LINE__)

#ifndef defer
#define defer const auto DEFER_VAR_NAME = nv::priv_impl::defer_helper << [&]
#endif

#undef DEFER_CONCAT
#undef DEFER_CONCAT_HELPER
#undef DEFER_VAR_NAME
