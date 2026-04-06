module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:concepts_invocable;

import std;

import libfork.utils;

import libfork.core:task;
import libfork.core:concepts_context;

namespace lf {

template <typename Context>
struct ctx_invoke_t {
  // Explicitly constrained so overload resolution selects prefers
  template <typename... Args, typename Fn>
    requires std::invocable<Fn, env<Context>, Args...>
  static constexpr auto operator()(Fn &&fn, Args &&...args)
      LF_HOF(std::invoke(std::forward<Fn>(fn), env<Context>{key()}, std::forward<Args>(args)...))

  template <typename... Args, typename Fn>
  static constexpr auto operator()(Fn &&fn, Args &&...args)
      LF_HOF(std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...))
};

template <typename Context, typename R>
concept task_from = specialization_of<R, task> && std::same_as<Context, typename R::context_type>;

/**
 * @brief Test if a callable `Fn` when invoked with `Args...` returns an `lf::task`.
 */
export template <typename Fn, typename Context, typename... Args>
concept async_invocable = worker_context<Context> &&                                                    //
                          std::invocable<ctx_invoke_t<Context>, Fn, Args...> &&                         //
                          task_from<Context, std::invoke_result_t<ctx_invoke_t<Context>, Fn, Args...>>; //

/**
 * @brief Subsumes `async_invocable` and checks that the invocation is `noexcept`.
 */
export template <typename Fn, typename Context, typename... Args>
concept async_nothrow_invocable =
    async_invocable<Fn, Context, Args...> && std::is_nothrow_invocable_v<ctx_invoke_t<Context>, Fn, Args...>;

/**
 * @brief The result type of invoking an async function `Fn` with `Args...`.
 */
export template <typename Fn, typename Context, typename... Args>
  requires async_invocable<Fn, Context, Args...>
using async_result_t = std::invoke_result_t<ctx_invoke_t<Context>, Fn, Args...>::value_type;

/**
 * @brief Subsumes `async_invocable` and checks the result type is `R`.
 */
export template <typename Fn, typename R, typename Context, typename... Args>
concept async_invocable_to =
    async_invocable<Fn, Context, Args...> && std::same_as<R, async_result_t<Fn, Context, Args...>>;

/**
 * @brief Subsumes `async_nothrow_invocable` and `async_invocable_to`.
 */
export template <typename Fn, typename R, typename Context, typename... Args>
concept async_nothrow_invocable_to =
    async_nothrow_invocable<Fn, Context, Args...> && async_invocable_to<Fn, R, Context, Args...>;

} // namespace lf
