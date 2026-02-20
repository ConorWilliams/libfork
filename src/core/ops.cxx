module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:ops;

import std;

import :concepts;
import :tuple;
import :utility;

namespace lf {

// clang-format off

// TODO: drop immovable/move_only

template <typename R, typename Fn, typename... Args>
struct pkg : immovable {
  R *return_address;
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

template <typename Fn, typename... Args>
struct pkg<void, Fn, Args...> : immovable {
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

// clang-format on

// TODO: consider a prelude namespace for these ops + task
// TODO: consider neibloids to block ADL

// ======== Fork ======== //

template <typename R, typename Fn, typename... Args>
struct [[nodiscard("You should immediately co_await this!")]] fork_pkg : pkg<R, Fn, Args...> {};

export template <typename... Args, async_invocable<Args...> Fn>
constexpr auto fork(std::nullptr_t, Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args &&...> {
  return {{.fn = LF_FWD(fn), .args = {LF_FWD(args)...}}};
}

export template <typename... Args, async_invocable_to<void, Args...> Fn>
constexpr auto fork(Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args &&...> {
  return {{.fn = LF_FWD(fn), .args = {LF_FWD(args)...}}};
}

export template <typename R, typename... Args, async_invocable_to<R, Args...> Fn>
constexpr auto fork(R *ret, Fn &&fn, Args &&...args) noexcept -> fork_pkg<R, Fn, Args &&...> {
  return {{.return_address = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}}};
}
// ======== Call ======== //

template <typename R, typename Fn, typename... Args>
struct [[nodiscard("You should immediately co_await this!")]] call_pkg : pkg<R, Fn, Args...> {};

export template <typename... Args, async_invocable<Args...> Fn>
constexpr auto call(std::nullptr_t, Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args &&...> {
  return {{.fn = LF_FWD(fn), .args = {LF_FWD(args)...}}};
}

export template <typename... Args, async_invocable_to<void, Args...> Fn>
constexpr auto call(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args &&...> {
  return {{.fn = LF_FWD(fn), .args = {LF_FWD(args)...}}};
}

export template <typename R, typename... Args, async_invocable_to<R, Args...> Fn>
constexpr auto call(R *ret, Fn &&fn, Args &&...args) noexcept -> call_pkg<R, Fn, Args &&...> {
  return {{.return_address = ret, .fn = LF_FWD(fn), .args = {LF_FWD(args)...}}};
}

// =============== Join =============== //

struct [[nodiscard("You should immediately co_await this!")]] join_type {};

export constexpr auto join() noexcept -> join_type { return {}; }

} // namespace lf
