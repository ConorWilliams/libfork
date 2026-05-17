module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:schedule;

import std;

import :concepts_invocable;
import :concepts_scheduler;
import :frame;
import :stop;
import :thread_locals;
import :promise;
import :root;
import :handles;
import :receiver;
import :exception;

namespace lf {

export struct schedule_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "schedule called from within a worker thread!";
  }
};

template <typename T>
concept decay_copyable = std::convertible_to<T, std::decay_t<T>>;

/**
 * @brief Schedule a function using a caller-provided `recv_state`.
 *
 * This will create a root task that stores decayed copies of `Fn` and
 * `Args...` in its frame, then post it to the scheduler. The root task must
 * then be resumed by a worker which will perform the invocation of `Fn`.
 *
 * The return address/exception and possibly stop token of the root task are
 * bound to the provided `recv_state` and can be observed by the caller via the
 * returned `receiver`.
 *
 * Strongly exception safe.
 */
export template <scheduler Sch, typename R, bool Stoppable, decay_copyable Fn, decay_copyable... Args>
  requires async_invocable_to<std::decay_t<Fn>, R, context_t<Sch>, std::decay_t<Args>...>
[[nodiscard("Fire and forget is an anti-pattern")]]
constexpr auto
schedule(Sch &&sch, recv_state<R, Stoppable> state, Fn &&fn, Args &&...args) -> receiver<R, Stoppable> {

  using context_type = context_t<Sch>;

  if (thread_local_context<context_type> != nullptr) {
    LF_THROW(schedule_error{});
  }

  state_handle<R, Stoppable> state_ptr = get(key(), std::move(state));

  LF_ASSUME(state_ptr != nullptr);

  // root_pkg's operator new may throw root_alloc_error if the frame is
  // too large; if so, `state_ptr` goes out of scope and destroys the state.
  root_task task = root_pkg<context_type>(state_ptr, std::forward<Fn>(fn), std::forward<Args>(args)...);

  LF_ASSUME(task.promise != nullptr);

  task.promise->frame.kind = category::root;
  task.promise->frame.parent = nullptr;

  if constexpr (Stoppable) {
    task.promise->frame.stop_token = state_ptr->stop.token();
  } else {
    task.promise->frame.stop_token = stop_source::stop_token{}; // non-cancellable root
  }

  LF_TRY {
    std::forward<Sch>(sch).post(sched_handle<context_type>{key(), &task.promise->frame});
    // If ^ didn't throw then the root_task will destroy itself at the final suspend.
  } LF_CATCH_ALL {
    // Otherwise, if it did throw, we must clean up
    task.promise->frame.handle().destroy();
    LF_RETHROW;
  }

  return {key(), std::move(state_ptr)};
}

template <typename T>
concept schedulable_return = std::is_void_v<T> || (std::default_initializable<T> && std::movable<T>);

template <typename Fn, typename Context, typename... Args>
concept default_schedulable =
    async_invocable<Fn, Context, Args...> && schedulable_return<async_result_t<Fn, Context, Args...>>;

template <typename Fn, typename Context, typename... Args>
using async_decay_result_t = async_result_t<std::decay_t<Fn>, Context, std::decay_t<Args>...>;

/**
 * @brief Convenience overload: default-constructs a non-cancellable recv_state.
 *
 * Uses the default allocator (`make_shared`) for all allocations.
 */
export template <scheduler Sch, decay_copyable Fn, decay_copyable... Args>
  requires default_schedulable<std::decay_t<Fn>, context_t<Sch>, std::decay_t<Args>...>
[[nodiscard("Fire and forget is an anti-pattern")]]
constexpr auto
schedule(Sch &&sch, Fn &&fn, Args &&...args) -> receiver<async_decay_result_t<Fn, context_t<Sch>, Args...>> {
  using result_type = async_decay_result_t<Fn, context_t<Sch>, Args...>;
  recv_state<result_type, false> state;
  return schedule(
      std::forward<Sch>(sch), std::move(state), std::forward<Fn>(fn), std::forward<Args>(args)...);
}

} // namespace lf
