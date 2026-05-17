module;
#include "libfork/__impl/exception.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:lift;

import libfork.utils;

import :task;
import :ops;
import :final_suspend;
import :awaitables;

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

/**
 * @brief An optimization for non-forked lifted functions.
 */
template <worker_context Context, bool StopToken, typename R, typename Fn, typename... Args>
struct lifted_awaitable : std::suspend_never {

  [[no_unique_address]]
  pkg<category::call, StopToken, Context, R, lift_impl, Fn, Args...> pkg;

  frame_t<Context> *parent;

  constexpr void await_ready() noexcept {

    // Noop if stop has been requested.
    if constexpr (StopToken) {
      if (pkg.stop_token().stop_requested()) {
        return;
      }
    } else {
      if (parent->frame.stop_requested()) {
        return;
      }
    }

    LF_TRY {
      std::move(pkg.args).apply([this](auto &&fn, auto &&...args) -> void {
        if constexpr (std::is_void_v<R>) {
          std::invoke(LF_FWD(fn), LF_FWD(args)...);
        } else {
          *pkg.return_addr = std::invoke(LF_FWD(fn), LF_FWD(args)...);
        }
      });
    } LF_CATCH(...) {
      stash_current_exception(parent);
    }
  }
};

} // namespace lf
