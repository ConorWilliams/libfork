module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
#include <cstddef>
export module libfork.core:schedule;

import std;

import libfork.utils;

import :concepts_invocable;
import :concepts_scheduler;
import :frame;
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

template <typename Fn, typename Context, typename... Args>
using schedule_state_t = receiver_state<invoke_decay_result_t<Fn, Context, Args...>>;

export template <typename Fn, typename Context, typename... Args>
  requires schedulable<Fn, Context, Args...>
using schedule_result_t = receiver<invoke_decay_result_t<Fn, Context, Args...>>;

/**
 * @brief Schedule a function to be run on a scheduler.
 *
 * This function is strongly exception safe.
 */
export template <scheduler Sch, decay_copyable Fn, decay_copyable... Args>
  requires schedulable<Fn, context_t<Sch>, Args...>
constexpr auto
schedule(Sch &&sch, Fn &&fn, Args &&...args) -> schedule_result_t<Fn, context_t<Sch>, Args...> {

  using context_type = context_t<Sch>;

  if (thread_local_context<context_type> != nullptr) {
    LF_THROW(schedule_error{});
  }

  // TODO: allocator aware new
  std::shared_ptr state = std::make_shared<schedule_state_t<Fn, context_type, Args...>>();

  // Package has shared ownership of the state, fine if this throws
  root_task task = root_pkg<context_type>(state, std::forward<Fn>(fn), std::forward<Args>(args)...);

  LF_ASSUME(task.promise != nullptr);

  // TODO: benchmark if it's worth having an unstoppable root task

  task.promise->frame.kind = category::root;
  task.promise->frame.parent = nullptr;
  task.promise->frame.cancel = nullptr;

  LF_TRY {
    sch.post(sched_handle<context_type>{key(), &task.promise->frame});
    // If ^ didn't throw then the root_task will destroy itself at the final suspend.
  } LF_CATCH_ALL {
    task.promise->frame.handle().destroy();
    LF_RETHROW;
  }

  return {key(), std::move(state)};
}

} // namespace lf
