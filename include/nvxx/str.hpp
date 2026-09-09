#pragma once

#include "nvxx/alloc.hpp"

namespace nv {
/// @brief a compile-time only, static string (literal)
/// @details non-type template paramter N does not account for null terminal
/// Supports concatenating and initializing from string literals, all at compile time!
template <isize N>
struct StaticString {
  static_assert(N >= 0, "N must be >= 0!");

  /// @brief +1 for null terminal!
  char data[N + 1];

  consteval StaticString() noexcept = default;

  template <isize O = N>
  consteval StaticString(StringLiteral<O> str) noexcept {
    static_assert(O >= 0, "O must be >= 0!");
    const isize len = std::min(N, O);

    for (i32 i = 0; i < len; i++) {
      this->data[i] = str[i];
    }
    this->data[len] = '\0';
  }

  template <isize O = N>
  consteval StaticString(StringLiteralPtr<O> strp) noexcept {
    static_assert(O >= 0, "O must be >= 0!");
    const isize len = std::min(N, O);
    auto str = *strp;

    for (i32 i = 0; i < len; i++) {
      this->data[i] = str[i];
    }
    this->data[len] = '\0';
  }

  static consteval usize len() noexcept { return static_cast<usize>(N); }
  static consteval isize ilen() noexcept { return N; }

  static consteval bool is_empty() noexcept { return len() == 0; }
  consteval operator Str() const noexcept { return this->str(); }

  constexpr std::string to_string() const noexcept { return std::string{this->begin(), this->end()}; }

  constexpr auto to_array() const noexcept { return std::array<const char, N>{this->begin(), this->end()}; }

  consteval const char* begin() const noexcept { return &this->data[0]; }
  consteval const char* end() const noexcept { return this->begin() + this->len(); }

  consteval const char* ptr() const noexcept { return &this->data[0]; }

  consteval Str str() const noexcept { return Str{this->ptr(), this->len()}; }

  template <isize O = N>
  consteval StaticString<N + O> concat(const StaticString<O>& rhs) const noexcept {
    const auto& lhs = *this;
    StaticString<N + O> res = {};

    i32 iter = 0;
    for (i32 i = 0; i < lhs.ilen(); i++, iter++) {
      res.data[iter] = lhs.data[i];
    }

    for (i32 i = 0; i < rhs.ilen(); i++, iter++) {
      res.data[iter] = rhs.data[i];
    }
    return res;
  }

  template <isize I>
  consteval const char& index() const noexcept {
    static_assert(I >= 0 && I < N, "Index out of range!");
    return this->operator[](I);
  }

  consteval decltype(auto) operator[](this auto& self, std::ptrdiff_t index) noexcept { return self.data[index]; }
};

template <isize L, isize R>
consteval StaticString<L + R> operator+(const StaticString<L>& lhs, const StaticString<R>& rhs) noexcept {
  return lhs.concat(rhs);
}

template <isize L, isize R>
consteval bool operator==(const StaticString<L>& lhs, const StaticString<R>& rhs) noexcept {
  if (L != R) {
    return false;
  }
  for (i32 i = 0; i < lhs.len(); i++) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

template <isize L, isize R>
consteval bool operator!=(const StaticString<L>& lhs, const StaticString<R>& rhs) noexcept {
  return !(lhs == rhs);
}

template <isize N>
consteval bool operator==(const StaticString<N>& self, const Str& rhs) noexcept {
  if (N != rhs.length()) {
    return false;
  }
  for (i32 i = 0; i < N; i++) {
    if (self[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

template <isize N>
consteval bool operator!=(const StaticString<N>& self, const Str& rhs) noexcept {
  return !(self == rhs);
}

template <isize N>
StaticString(ArrayLvref<char, N>) -> StaticString<N - 1>;

template <isize N>
StaticString(ArrayLvref<const char, N>) -> StaticString<N - 1>;

template <isize N>
StaticString(std::array<const char, N>) -> StaticString<N - 1>;

template <isize N>
constexpr StaticString<N - 1> static_string(StringLiteral<N> lit) noexcept {
  return StaticString<N - 1>{lit};
}

namespace priv {

struct StringOps {
  constexpr isize len(this auto& self) noexcept { return self.count; }
  constexpr char* begin(this auto& self) noexcept { return &self.data[0]; }
  constexpr char* end(this auto& self) noexcept { return self.begin() + self.len(); }
};

}  // namespace priv

template <class A>
struct String;

template <class A>
  requires(std::is_empty_v<A> && Allocator<A>)
struct String<A> : priv::StringOps {
  static_assert(std::is_object_v<A>,
                "Template parameter A for String must be object (not a pointer or reference type)");
  using Alloc = A;
  using Self = String<A>;

  char* data;
  isize count;
};

template <class A>
  requires(!std::is_empty_v<A> && Allocator<A>)
struct String<A> : priv::StringOps {
  char* data;
  isize count;
  A alloc;
};

template <>
struct String<AllocContext> : priv::StringOps {
  char* data;
  isize count;
  AllocContext alloc;
};

}  // namespace nv
