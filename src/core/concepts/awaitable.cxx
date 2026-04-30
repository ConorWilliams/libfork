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
constexpr auto do_acquire_awaitable(T &&t) LF_HOF(LF_FWD(t))

template <member_co_awaitable T>
[[nodiscard]]
constexpr auto do_acquire_awaitable(T &&t) LF_HOF(LF_FWD(t).operator co_await())

template <operator_co_awaitable T>
[[nodiscard]]
constexpr auto do_acquire_awaitable(T &&t)
    LF_HOF(operator co_await(LF_FWD(t)))

/**
 * @brief Specify that an awaitable can be unambiguously acquired from `T` by free/member operator co_await.
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
 * @brief Checks for methods.
 */
template <typename T, typename Context>
concept custom_awaitable_methods = requires (T x, Context &ctx, sched_handle<Context> handle) {
  { x.await_ready() } -> std::convertible_to<bool>;
  { x.await_suspend(handle, ctx) } -> std::same_as<void>;
  { x.await_resume() };
};

template <typename T, typename Context>
concept nothrow_await_suspend = requires (T x, Context &ctx, sched_handle<Context> handle) {
  { x.await_suspend(handle, ctx) } noexcept;
};

/**
 * @brief Checks for methods + storable
 */
template <typename T, typename Context>
concept custom_awaitable = storable<T> && custom_awaitable_methods<std::remove_cvref_t<T>, Context>;

/**
 * @brief  Specifies the requirements for a context-switching awaitable type.
 *
 * Note: await_suspend may not complete inline i.e. the current thread remains
 * bound to the context.
 */
export template <typename T, typename Context>
concept awaitable = worker_context<Context> && requires (T x) {
  { acquire_awaitable(static_cast<T &&>(x)) } -> custom_awaitable<Context>;
};

} // namespace lf
