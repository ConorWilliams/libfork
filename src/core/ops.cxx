module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:ops;

import std;

import libfork.utils;

import :concepts_invocable;
import :frame;
import :stop;

namespace lf {

// Placeholder types for absent optional fields.
struct no_stop_t {};
struct no_ret_t {};

// clang-format off

template <category Cat, bool StopToken, typename Context, typename R, typename Fn, typename... Args>
struct [[nodiscard("You should immediately co_await this!")]] pkg {
  [[no_unique_address]] std::conditional_t<StopToken, stop_source::stop_token, no_stop_t> stop_token;
  [[no_unique_address]] std::conditional_t<std::is_void_v<R>, no_ret_t, R *> return_addr;
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

// =============== Join =============== //

struct join_type {};

/**
 * @brief Base class shared by scope_ops and child_scope_ops.
 *
 * Provides a member `join()` so that `co_await sc.join()` works on any scope type.
 */
struct scope_base {
  [[nodiscard("You should immediately co_await this!")]]
  static constexpr auto join() noexcept -> join_type {
    return {};
  }
};

// =============== Scope ops (no embedded stop source) =============== //

template <typename Context>
struct scope_ops : scope_base {
 private:
  template <typename R, typename Fn, typename... Args>
  using call_pkg = pkg<category::call, false, Context, R, Fn &&, Args &&...>;

  template <typename R, typename Fn, typename... Args>
  using fork_pkg = pkg<category::fork, false, Context, R, Fn &&, Args &&...>;

 public:
  // Default constructible
  scope_ops() noexcept = default;

  // Immovable
  scope_ops(const scope_ops &) = delete;
  scope_ops(scope_ops &&) = delete;
  auto operator=(const scope_ops &) -> scope_ops & = delete;
  auto operator=(scope_ops &&) -> scope_ops & = delete;

  // === Fork === //

  template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
  static constexpr auto fork(R *ret, Fn &&fn, Args &&...args) noexcept -> fork_pkg<R, Fn, Args...> {
    return {.return_addr = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable<Context, Args...> Fn>
  static constexpr auto fork_drop(Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args...> {
    return {.return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
  static constexpr auto fork(Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args...> {
    return {.return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }

  // === Call === //

  template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
  static constexpr auto call(R *ret, Fn &&fn, Args &&...args) noexcept -> call_pkg<R, Fn, Args...> {
    return {.return_addr = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable<Context, Args...> Fn>
  static constexpr auto call_drop(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args...> {
    return {.return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
  static constexpr auto call(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args...> {
    return {.return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
};

// ==== Scope awaitable ==== //

template <worker_context Context>
struct scope_awaitable : std::suspend_never {
  static constexpr auto await_resume() -> scope_ops<Context> { return {}; }
};

struct scope_type {};

export [[nodiscard("You should immediately co_await this!")]]
constexpr auto scope() noexcept -> scope_type {
  return {};
}

// =============== Child scope ops (with embedded stop source) =============== //

/**
 * @brief A scope that is a stop_source.
 */
template <typename Context>
struct child_scope_ops : scope_base, stop_source {
 private:
  template <typename R, typename Fn, typename... Args>
  using call_pkg = pkg<category::call, true, Context, R, Fn &&, Args &&...>;

  template <typename R, typename Fn, typename... Args>
  using fork_pkg = pkg<category::fork, true, Context, R, Fn &&, Args &&...>;

 public:
  /**
   * @brief Construct the scope, chaining its stop source onto the parent's token.
   */
  explicit constexpr child_scope_ops(stop_source::stop_token parent) noexcept : stop_source(parent) {}

  // Immovable (stop_source base is immovable)
  child_scope_ops(const child_scope_ops &) = delete;
  child_scope_ops(child_scope_ops &&) = delete;
  auto operator=(const child_scope_ops &) -> child_scope_ops & = delete;
  auto operator=(child_scope_ops &&) -> child_scope_ops & = delete;

  // === Fork (binds this scope's stop source as child stop source) === //

  template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
  constexpr auto fork(R *ret, Fn &&fn, Args &&...args) noexcept -> fork_pkg<R, Fn, Args...> {
    return {.stop_token = token(), .return_addr = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable<Context, Args...> Fn>
  constexpr auto fork_drop(Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args...> {
    return {.stop_token = token(), .return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
  constexpr auto fork(Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args...> {
    return {.stop_token = token(), .return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }

  // === Call (binds this scope's stop source as child stop source) === //

  template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
  constexpr auto call(R *ret, Fn &&fn, Args &&...args) noexcept -> call_pkg<R, Fn, Args...> {
    return {.stop_token = token(), .return_addr = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable<Context, Args...> Fn>
  constexpr auto call_drop(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args...> {
    return {.stop_token = token(), .return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
  template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
  constexpr auto call(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args...> {
    return {.stop_token = token(), .return_addr = {}, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}};
  }
};

// =============== child_scope_awaitable =============== //

template <worker_context Context>
struct child_scope_awaitable : std::suspend_never {

  stop_source::stop_token parent_stop_source;

  constexpr auto await_resume(this child_scope_awaitable self) -> child_scope_ops<Context> {
    return child_scope_ops<Context>{self.parent_stop_source};
  }
};

struct child_scope_type {};

export [[nodiscard("You should immediately co_await this!")]]
constexpr auto child_scope() noexcept -> child_scope_type {
  return {};
}

} // namespace lf
