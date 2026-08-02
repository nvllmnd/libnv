#pragma once
#include <variant>

namespace nv::tunion {

using std::get;
using std::get_if;
using std::holds_alternative;
using std::variant;

template <class... Ts>
using Union = std::variant<Ts...>;

}  // namespace nv::tunion
