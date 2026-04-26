module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:concepts_awaitable;

import std;

import :handles;
import :concepts_context;

namespace lf {

/**
 * @brief Specifies that a cv-ref stripped type is constructible from `T`.
 */
template <typename T>
concept storable = std::constructible_from<std::remove_cvref_t<T>, T &&>;

/**
 * @brief  Specifies the requirements for a context-switching awaitable type.
 */
export template <typename T, typename U>
concept awaitable =
    storable<T> && worker_context<U> && requires (std::remove_cvref_t<T> x, U &ctx, sched_handle<U> handle) {
      { x.await_ready() } -> std::convertible_to<bool>;
      { x.await_suspend(handle, ctx) } -> std::same_as<void>;
      { x.await_resume() };
    };

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
constexpr auto do_acquire_awaitable(T &&t) LF_HOF(operator co_await(LF_FWD(t)))

template <typename T>
concept awaitable_acquirable = requires (T t) { do_acquire_awaitable(static_cast<T &&>(t)); };

export template <awaitable_acquirable T>
constexpr auto acquire_awaitable(T &&t)
    LF_HOF(do_acquire_awaitable(LF_FWD(t)))

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
