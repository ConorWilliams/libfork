module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:concepts_awaitable;

import std;

import :handles;
import :concepts_context;

namespace lf {

template <typename T>
concept member_co_awaitable = requires (T t) { static_cast<T &&>(t).operator co_await(); };

template <typename T>
concept operator_co_awaitable = requires (T t) { operator co_await(static_cast<T &&>(t)); };

template <typename T>
[[nodiscard]]
constexpr auto do_acquire_awaitable(T &&t) noexcept -> T && {
  std::forward<T>(t);
}

template <member_co_awaitable T>
[[nodiscard]]
constexpr auto do_acquire_awaitable(T &&t) LF_HOF(LF_FWD(t).operator co_await())

template <operator_co_awaitable T>
[[nodiscard]]
constexpr auto do_acquire_awaitable(T &&t)
    LF_HOF(operator co_await(LF_FWD(t)))

/**
 * @brief Specify that an awaitable can be unambiguously acuired from `T` by free/member operator co_await.
 *
 * If neither operator is present `T` is assumed to be a plain awaitable.
 */
template <typename T>
concept awaitable_acquirable = requires (T x) { do_acquire_awaitable(static_cast<T &&>(x)); };

/**
 * @brief Extracts the awaitable from `T` by invoking the appropriate operator co_await, or returning `T`
 * itself if neither operator is present.
 */
export template <awaitable_acquirable T>
constexpr auto acquire_awaitable(T &&t)
    LF_HOF(do_acquire_awaitable(LF_FWD(t)))

/**
 * @brief Specifies that a cv-ref stripped type is constructible from `T`.
 */
template <typename T>
concept storable = std::constructible_from<std::remove_cvref_t<T>, T &&>;

/**
 * @brief Checks for methods + storable
 */
template <typename T, typename Context>
concept awaitable_impl =
    storable<T> && requires (std::remove_cvref_t<T> x, Context &ctx, sched_handle<Context> handle) {
      { x.await_ready() } -> std::convertible_to<bool>;
      { x.await_suspend(handle, ctx) } -> std::same_as<void>;
      { x.await_resume() };
    };

/**
 * @brief  Specifies the requirements for a context-switching awaitable type.
 */
export template <typename T, typename Context>
concept awaitable = worker_context<Context> && requires (T x) {
  { acquire_awaitable(static_cast<T &&>(x)) } -> awaitable_impl<Context>;
};

// template <worker_context Context, awaitable<Context> T>
// struct context_switch_awaitable {
//
//   static_assert(plain_object<T>, "Expecting remove cv-ref");
//
//   [[no_unique_address]]
//   T value;
//
//   constexpr auto await_ready() LF_HOF(value.await_ready())
//
//   constexpr void await_suspend(std::coroutine_handle<>);
//
//   constexpr auto await_resume() LF_HOF(std::forward<T>(value).await_resume())
// };

} // namespace lf
