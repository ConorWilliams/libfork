module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:ops;

import std;

import libfork.utils;

import :concepts_invocable;
import :frame;

namespace lf {

// clang-format off

template <category Cat, typename Context, typename R, typename Fn, typename... Args>
struct [[nodiscard("You should immediately co_await this!")]] pkg {
  R *return_address;
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

template <category Cat, typename Context, typename Fn, typename... Args>
struct [[nodiscard("You should immediately co_await this!")]] pkg<Cat, Context, void, Fn, Args...> {
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

// clang-format on

/**
 * @brief Forward the function member of a pkg correctly
 *
 * The Fn member should be an l/r value reference, r-value reference need an
 * explicit move to be forwarded correctly.
 */
template <typename Fn>
constexpr auto fwd_fn(auto &&fn) noexcept -> Fn {

  static_assert(std::is_reference_v<Fn>);

  if constexpr (std::is_rvalue_reference_v<Fn>) {
    return std::move(fn);
  } else {
    return fn;
  }
}

export template <typename Context>
struct scope {
 private:
  // Use && for fn/args for zero move/copy + noexcept
  // TODO: Is it better to stores values for some types i.e. empty

  template <typename R, typename Fn, typename... Args>
  using call_pkg = pkg<category::call, Context, R, Fn &&, Args &&...>;

  template <typename R, typename Fn, typename... Args>
  using fork_pkg = pkg<category::fork, Context, R, Fn &&, Args &&...>;

 public:
  template <typename... Args, async_invocable<Context, Args...> Fn>
  static constexpr auto
  fork(std::nullptr_t, Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args...> {
    return {.fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
  static constexpr auto fork(Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args...> {
    return {.fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
  static constexpr auto fork(R *ret, Fn &&fn, Args &&...args) noexcept -> fork_pkg<R, Fn, Args...> {
    return {.return_address = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }

  template <typename... Args, async_invocable<Context, Args...> Fn>
  static constexpr auto
  call(std::nullptr_t, Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args...> {
    return {.fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
  static constexpr auto call(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args...> {
    return {.fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
  static constexpr auto call(R *ret, Fn &&fn, Args &&...args) noexcept -> call_pkg<R, Fn, Args...> {
    return {.return_address = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
};

// TODO: do we want join a member of scope?

// =============== Join =============== //

struct join_type {};

export [[nodiscard("You should immediately co_await this!")]]
constexpr auto join() noexcept -> join_type {
  return {};
}

} // namespace lf
