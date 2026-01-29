
export module libfork.core:tuple;

import std;

namespace lf {

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
using cq = copy_cvref<From, To>::type;

template <int I, typename T>
struct tuple_leaf {
  [[no_unique_address]]
  T elem; // TODO: use reflection to give this a nice name
};

template <typename, typename...>
struct tuple_impl;

template <std::size_t... Is, typename... Ts>
struct tuple_impl<std::index_sequence<Is...>, Ts...> : tuple_leaf<Is, Ts>... {
  template <std::size_t I, typename Self>
  [[nodiscard]]
  constexpr auto get(this Self &&self) noexcept -> cq<Self &&, Ts... [I]> {
    return static_cast<cq<Self &&, tuple_leaf<Is...[I], Ts...[I]>>>(self).elem;
  }
};

/**
 * @brief A minimal non-recursive tuple implementation.
 *
 * This is a very stripped back tuple that only: supports movable types,
 * provides `.get<I>()` and, supports structured bindings.
 *
 * This is has the advantage of significantly faster compilation times
 * compared to the standard library's `std::tuple`. In addition it is an
 * aggregate type hence, is trivially copyable/constructable/destructible
 * conditional on the types it contains. It even works as an NTTP.
 */
export template <movable... Ts>
struct tuple : tuple_impl<std::index_sequence_for<Ts...>, Ts...> {};

template <movable... Ts>
tuple(Ts...) -> tuple<Ts...>;

} // namespace lf

template <typename... Ts>
struct std::tuple_size<bat::tuple<Ts...>> : integral_constant<size_t, sizeof...(Ts)> {};

template <std::size_t I, typename... Ts>
struct std::tuple_element<I, bat::tuple<Ts...>> : type_identity<Ts... [I]> {};
