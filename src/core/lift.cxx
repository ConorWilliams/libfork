module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:lift;

import libfork.utils;

import :task;
import :ops;
import :final_suspend;
import :awaitables;

namespace lf {

template <typename T>
using lift_store_t = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::remove_cvref_t<T>>;

struct lift_impl {
 private:
  template <typename Fn, typename Context, typename... Args>
  using task_t = task<std::invoke_result_t<Fn, Args...>, Context>;

  template <typename Fn, typename Context, typename... Args>
  static auto impl(lift_store_t<Fn> fn, lift_store_t<Args>... args) -> task_t<Fn, Context, Args...> {
    co_return std::invoke(static_cast<Fn>(fn), static_cast<Args>(args)...);
  }

 public:
  template <typename Fn, typename Context, typename... Args>
    requires std::invocable<Fn &&, Args &&...>
  static auto operator()(env<Context>, Fn &&fn, Args &&...args) -> task_t<Fn &&, Context, Args &&...> {
    return impl<Fn &&, Context, Args &&...>(LF_FWD(fn), LF_FWD(args)...);
  }
};

// TODO: merge fn in ops to args

/**
 * @brief Lifts a synchronous function into an asynchronous task.
 *
 * Forked lifted tasks capture rvalues by value.
 * Called lifted tasks have an optimized path that avoids creating a new task.
 *
 * Both invocations respect cancellation and push exceptions to the parent scope.
 */
export inline constexpr lift_impl lift{};

/**
 * @brief An optimization for non-forked lifted functions.
 */
template <worker_context Context, bool StopToken, typename R, typename Fn, typename... Args>
struct lifted_awaitable : std::suspend_never {

  [[no_unique_address]]
  pkg<category::call, StopToken, Context, R, Fn, Args...> pkg;

  frame_t<Context> *parent;

  constexpr void await_resume() noexcept {

    // Noop if stop has been requested.
    if constexpr (StopToken) {
      if (pkg.stop_token.stop_requested()) {
        return;
      }
    } else {
      if (parent->stop_requested()) {
        return;
      }
    }

    LF_TRY {
      if constexpr (std::is_void_v<R>) {
        std::move(pkg.args).apply([](lift_impl, auto &&fn, auto &&...args) -> void {
          std::invoke(LF_FWD(fn), LF_FWD(args)...);
        });
      } else {
        std::move(pkg.args).apply([addr = pkg.return_addr](lift_impl, auto &&fn, auto &&...args) -> void {
          LF_ASSUME(addr);
          *addr = std::invoke(LF_FWD(fn), LF_FWD(args)...);
        });
      }
    } LF_CATCH(...) {
      stash_current_exception(parent);
    }
  }
};

} // namespace lf
