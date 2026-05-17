module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:lift;

import libfork.utils;

import :task;

namespace lf {

struct lift_impl {
  template <typename Fn, typename Context, typename... Args>
    requires std::invocable<Fn, Args...>
  static auto
  operator()(env<Context>, Fn &&fn, Args &&...args) -> task<std::invoke_result_t<Fn, Args...>, Context> {
    co_return std::invoke(LF_FWD(fn), LF_FWD(args)...);
  }
};

/**
 * @brief Lifts a synchronous function into an asynchronous task.
 */
export inline constexpr lift_impl lift{};

} // namespace lf
