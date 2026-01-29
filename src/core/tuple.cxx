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
  using R = std::remove_reference_t<T>;

  using U1 = std::conditional_t<std::is_const_v<R>, std::add_const_t<U>, U>;
  using U2 = std::conditional_t<std::is_volatile_v<R>, std::add_volatile_t<U1>, U1>;
  using U3 = std::conditional_t<std::is_lvalue_reference_v<T>, std::add_lvalue_reference_t<U2>, U2>;
  using U4 = std::conditional_t<std::is_rvalue_reference_v<T>, std::add_rvalue_reference_t<U3>, U3>;

 public:
  using type = U4;
};

/**
 * Copy the const/volatile/reference qualifiers from `From` to `To`.
 */
export template <typename From, typename To>
  requires std::is_object_v<To>
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
  constexpr auto get(this Self &&self)
      LF_HOF(static_cast<copy_cvref_t<Self &&, tuple_leaf<Is...[I], Ts...[I]>>>(self).elem)

  [[nodiscard]]
  constexpr auto apply(this auto &&self, auto &&fn)
      LF_HOF(std::invoke(LF_FWD(fn), LF_FWD(self).template get<Is>()...))
};

/**
 * @brief A minimal non-recursive tuple implementation.
 *
 * This is a very stripped back tuple that only:
 *
 * - Provides `.get<I>()` member function.
 * - Provides an apply member function.
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
struct std::tuple_size<bat::tuple<Ts...>> : integral_constant<size_t, sizeof...(Ts)> {};

template <std::size_t I, typename... Ts>
struct std::tuple_element<I, bat::tuple<Ts...>> : type_identity<Ts... [I]> {};
