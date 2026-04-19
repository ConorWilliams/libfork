module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:schedule;

import std;

import libfork.utils;

import :concepts_invocable;
import :concepts_scheduler;
import :frame;
import :stop;
import :thread_locals;
import :promise;
import :root;
import :handles;
import :receiver;

namespace lf {

export template <typename T>
concept schedulable_return = std::is_void_v<T> || (std::default_initializable<T> && std::movable<T>);

export struct schedule_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "schedule called from within a worker thread!";
  }
};

template <typename T>
concept decay_copyable = std::convertible_to<T, std::decay_t<T>>;

template <typename Fn, typename Context, typename... Args>
concept schedulable_decayed =
    async_invocable<Fn, Context, Args...> && schedulable_return<async_result_t<Fn, Context, Args...>>;

export template <typename Fn, typename Context, typename... Args>
concept schedulable = schedulable_decayed<std::decay_t<Fn>, Context, std::decay_t<Args>...>;

template <typename Fn, typename Context, typename... Args>
using invoke_decay_result_t = async_result_t<std::decay_t<Fn>, Context, std::decay_t<Args>...>;

export template <typename Fn, typename Context, typename... Args>
  requires schedulable<Fn, Context, Args...>
using schedule_result_t = receiver<invoke_decay_result_t<Fn, Context, Args...>>;

/**
 * @brief Schedule a function using a caller-provided `root_state`.
 *
 * Strongly exception safe: if the scheduler's `post()` throws, the root
 * frame is destroyed and the exception is rethrown to the caller.
 */
export template <scheduler Sch, typename R, bool Stoppable, decay_copyable Fn, decay_copyable... Args>
  requires schedulable<Fn, context_t<Sch>, Args...> &&
           std::same_as<R, invoke_decay_result_t<Fn, context_t<Sch>, Args...>>
constexpr auto
schedule(Sch &&sch, root_state<R, Stoppable> state, Fn &&fn, Args &&...args) -> receiver<R, Stoppable> {

  using context_type = context_t<Sch>;

  if (thread_local_context<context_type> != nullptr) {
    LF_THROW(schedule_error{});
  }

  std::shared_ptr<receiver_state<R, Stoppable>> sp = get(key(), state);

  LF_ASSUME(sp != nullptr);

  // root_pkg's operator new may throw root_alloc_error if the frame is
  // too large; if so, `sp` goes out of scope and destroys the state.
  root_task task = root_pkg<context_type>(sp, std::forward<Fn>(fn), std::forward<Args>(args)...);

  LF_ASSUME(task.promise != nullptr);

  task.promise->frame.kind = category::root;
  task.promise->frame.parent = nullptr;

  if constexpr (Stoppable) {
    task.promise->frame.stop_token = sp->stop.token();
  } else {
    task.promise->frame.stop_token = stop_source::stop_token{}; // non-cancellable root
  }

  LF_TRY {
    // TODO: forward sch + modify concept
    sch.post(sched_handle<context_type>{key(), &task.promise->frame});
    // If ^ didn't throw then the root_task will destroy itself at the final suspend.
  } LF_CATCH_ALL {
    task.promise->frame.handle().destroy();
    LF_RETHROW;
  }

  return {key(), std::move(sp)};
}

/**
 * @brief Convenience overload: default-constructs a non-cancellable root_state.
 */
export template <scheduler Sch, decay_copyable Fn, decay_copyable... Args>
  requires schedulable<Fn, context_t<Sch>, Args...>
constexpr auto
schedule(Sch &&sch, Fn &&fn, Args &&...args) -> receiver<invoke_decay_result_t<Fn, context_t<Sch>, Args...>> {

  using context_type = context_t<Sch>;
  using R = invoke_decay_result_t<Fn, context_type, Args...>;

  return schedule(
      std::forward<Sch>(sch), root_state<R, false>{}, std::forward<Fn>(fn), std::forward<Args>(args)...);
}

} // namespace lf
