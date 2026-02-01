module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:ops;

import std;

import :concepts;
import :tuple;
import :utility;

namespace lf {

// clang-format off

template <typename R, typename Fn, typename... Args>
struct pkg {
  R *return_address;
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

template <typename Fn, typename... Args>
struct pkg<void, Fn, Args...> {
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

// clang-format on

// ======== Fork ======== //

template <typename R, typename Fn, typename... Args>
struct [[nodiscard("You should immediately co_await this!")]] fork_pkg : pkg<R, Fn, Args...>, immovable {};

export template <typename... Args, async_invocable_to<void, Args...> Fn>
constexpr auto fork(Fn &&fn, Args &&...args) noexcept -> fork_pkg<void, Fn, Args &&...> {
  return {LF_FWD(fn), {LF_FWD(args)...}};
}

export template <typename R, typename... Args, async_invocable_to<R, Args...> Fn>
constexpr auto fork(R *ret, Fn &&fn, Args &&...args) noexcept -> fork_pkg<R, Fn, Args &&...> {
  return {ret, LF_FWD(fn), {LF_FWD(args)...}};
}
// ======== Call ======== //

template <typename R, typename Fn, typename... Args>
struct [[nodiscard("You should immediately co_await this!")]] call_pkg : pkg<R, Fn, Args...>, immovable {};

export template <typename... Args, async_invocable_to<void, Args...> Fn>
constexpr auto call(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args &&...> {
  return {LF_FWD(fn), {LF_FWD(args)...}};
}

export template <typename R, typename... Args, async_invocable_to<R, Args...> Fn>
constexpr auto call(R *ret, Fn &&fn, Args &&...args) noexcept -> call_pkg<R, Fn, Args &&...> {
  return {ret, LF_FWD(fn), {LF_FWD(args)...}};
}

} // namespace lf
