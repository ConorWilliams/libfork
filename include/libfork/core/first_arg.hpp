#ifndef DD0B4328_55BD_452B_A4A5_5A4670A6217B
#define DD0B4328_55BD_452B_A4A5_5A4670A6217B

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

#include <libfork/core/tag.hpp>

#include <libfork/core/impl/utility.hpp>

namespace lf {

/**
 * @brief Test if the expression `*std::declval<T&>()` is valid and has a referenceable type.
 */
template <typename I>
concept dereferenceable = requires (I val) {
  { *val } -> impl::can_reference;
};

/**
 * @brief A quasi-pointer if a movable type that can be dereferenced to a referenceable type.
 *
 * A quasi-pointer is assumed to be cheap-to-move like an iterator/legacy-pointer.
 */
template <typename I>
concept quasi_pointer = std::default_initializable<I> && std::movable<I> && dereferenceable<I>;

/**
 * @brief A concept that requires a type be a copyable [function
 * object](https://en.cppreference.com/w/cpp/named_req/FunctionObject).
 *
 * An async function object is a function object that returns an `lf::task` when `operator()` is called.
 * with appropriate arguments. The call to `operator()` must create a coroutine. The first argument
 * of an async function must accept a deduced templated type that satisfies the `first_arg` concept.
 * The return type and invocability of an async function must be independent of the first argument except
 * for its tag value.
 *
 * An async function may be copied, its copies must be equivalent to the original and support concurrent
 * invocation from multiple threads. It is assumed that an async function is cheap-to-copy like
 * an iterator/legacy-pointer.
 */
template <typename F>
concept async_function_object = std::is_object_v<F> && std::copy_constructible<F>;

/**
 * @brief This describes the public-API of the first argument passed to an async function.
 *
 * An async functions invocability and return type must be independent of their first argument.
 */
template <typename T>
concept first_arg = async_function_object<T> && requires {
  { T::tag } -> std::convertible_to<tag>;
};

namespace impl {

/**
 * @brief The type passed as the first argument to async functions.
 *
 * Its functions are:
 *
 * - Act as a y-combinator (expose same invocability as F).
 * - Statically inform the return pointer type.
 * - Statically provide the tag.
 */
template <quasi_pointer I, tag Tag, async_function_object F>
class first_arg_t {
 public:
  static constexpr tag tag = Tag; ///< The way this async function was called.

  first_arg_t() = default;

  template <different_from<first_arg_t> T>
    requires std::constructible_from<F, T>
  explicit first_arg_t(T &&expr) noexcept(std::is_nothrow_constructible_v<F, T>)
      : m_fun(std::forward<T>(expr)) {}

  template <typename... Args>
    requires std::invocable<F &, Args...>
  auto operator()(Args &&...args) & noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F &, Args...> {
    return std::invoke(m_fun, std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::invocable<F const &, Args...>
  auto operator()(Args &&...args) const & noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F const &, Args...> {
    return std::invoke(m_fun, std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::invocable<F &&, Args...>
  auto operator()(Args &&...args) && noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F &&, Args...> {
    return std::invoke(std::move(m_fun), std::forward<Args>(args)...);
  }

  template <typename... Args>
    requires std::invocable<F const &&, Args...>
  auto operator()(Args &&...args) const && noexcept(std::is_nothrow_invocable_v<F &, Args...>)
      -> std::invoke_result_t<F const &&, Args...> {
    return std::invoke(std::move(m_fun), std::forward<Args>(args)...);
  }

 private:
  /**
   * @brief Hidden friend reduces discoverability.
   */
  friend auto unwrap(first_arg_t &&arg) -> F && { return std::move(arg.m_fun); }

  F m_fun;
};

} // namespace impl

} // namespace lf

#endif /* DD0B4328_55BD_452B_A4A5_5A4670A6217B */
