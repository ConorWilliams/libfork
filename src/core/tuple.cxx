module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:tuple;

import std;

namespace lf {

// TODO: Replace with reflection tuple?

//========== Copy Qualifiers =============//

template <typename T, typename U>
struct copy_cvref {
 private:
  using u0 = std::remove_reference_t<T>;
  using u1 = std::conditional_t<std::is_const_v<u0>, std::add_const_t<U>, U>;
  using u2 = std::conditional_t<std::is_volatile_v<u0>, std::add_volatile_t<u1>, u1>;
  using u3 = std::conditional_t<std::is_lvalue_reference_v<T>, std::add_lvalue_reference_t<u2>, u2>;
  using u4 = std::conditional_t<std::is_rvalue_reference_v<T>, std::add_rvalue_reference_t<u3>, u3>;

 public:
  using type = u4;
};

/**
 * Copy the const/volatile/reference qualifiers from `From` to `To`.
 */
export template <typename From, typename To>
using copy_cvref_t = copy_cvref<From, To>::type;

template <int I, typename T>
struct tuple_leaf {
  [[no_unique_address]]
  T elem;
};

//========== Tuple =============//

template <typename, typename...>
struct tuple_impl;

template <std::size_t... Is, typename... Ts>
struct tuple_impl<std::index_sequence<Is...>, Ts...> : tuple_leaf<Is, Ts>... {
  template <std::size_t I, typename Self>
  [[nodiscard]]
  constexpr auto get(this Self &&self) noexcept -> copy_cvref_t<Self &&, Ts... [I]> {
    return LF_FWD(self).template tuple_leaf<I, Ts...[I]>::elem;
  }

  [[nodiscard]] constexpr auto apply(this auto &&self, auto &&fn)
      LF_HOF(std::invoke(LF_FWD(fn), LF_FWD(self).template get<Is>()...))
};

template <>
struct tuple_impl<std::index_sequence<>> {
  [[nodiscard]]
  static constexpr auto apply(auto &&fn) LF_HOF(std::invoke(LF_FWD(fn)))
};

/**
 * @brief A minimal non-recursive tuple implementation.
 *
 * This is a very stripped back tuple that only:
 *
 * - Provides `.get<I>()` member function.
 * - Provides an `apply(fn)` member function.
 * - Supports structured bindings.
 *
 * This is has the advantage of significantly faster compilation times
 * compared to the standard library's `std::tuple`. In addition it is an
 * aggregate type hence, is trivially copyable/constructable/destructible
 * conditional on the types it contains. It even works as an NTTP.
 */
export template <typename... Ts>
struct tuple final : tuple_impl<std::index_sequence_for<Ts...>, Ts...> {};

template <typename... Ts>
tuple(Ts &&...) -> tuple<Ts...>;

} // namespace lf

template <typename... Ts>
struct std::tuple_size<lf::tuple<Ts...>> : integral_constant<size_t, sizeof...(Ts)> {};

template <std::size_t I, typename... Ts>
struct std::tuple_element<I, lf::tuple<Ts...>> : type_identity<Ts... [I]> {};
