module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.core:schedule;

import std;

import :concepts;
import :frame;
import :thread_locals;
import :promise;
import :root;
import :handles;
import :utility;
import :receiver;

namespace lf {

///////////////////////////////////////////////////

export template <typename T>
concept schedulable_return = std::is_void_v<T> || std::default_initializable<T>;

export struct schedule_error : std::runtime_error {
  using std::runtime_error::runtime_error;
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
schedule2(Sch &&sch, Fn &&fn, Args &&...args) -> schedule_result_t<Fn, context_t<Sch>, Args...> {

  using context_type = context_t<Sch>;

  // TODO: make sure this is exception safe and correctly qualifed

  if (thread_context<context_type> != nullptr) {
    LF_THROW(schedule_error{"Schedule called from within a worker thread!"});
  }

  // TODO: allocator aware new
  std::shared_ptr state = std::make_shared<schedule_state_t<Fn, context_type, Args...>>();

  // TODO: clean up block if exception
  // TODO: make sure we're cancel safe

  // Package has shared ownership of the state, fine if this throws
  root_task task = package_as_root<context_type>(state, std::forward<Fn>(fn), std::forward<Args>(args)...);

  LF_ASSUME(task.promise != nullptr);

  LF_TRY {
    sch.post(sched_handle<context_type>{key(), &task.promise->frame});
    // If ^ didn't throw then the root_task will destroy itself at the final suspend.
  } LF_CATCH_ALL {
    task.promise->frame.handle().destroy();
    LF_RETHROW;
  }

  return {key(), std::move(state)};
}

//////////////////////////////////////////////////////////////////////
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
////////////////////////////////////////////////////////////////////////

// schedule(context*, fn, args...) -> heap allocated shared state this must
// create a task, bind appropriate values + cancellation submit the task to a
// context. So a context must have a submit(...) method, the thing we submit
// must be atomicable, ideally it would be able to accept submission at either
// a root OR context switch point

struct block_deleter {
  static void operator()(block_type *block) noexcept { release_ref(block); }
};

template <typename T>
concept void_or_default_initializable = std::is_void_v<T> || std::default_initializable<T>;

template <void_or_default_initializable T>
struct block;

template <>
struct block<void> final : block_type {};

template <std::default_initializable T>
struct block<T> final : block_type {
  [[no_unique_address]]
  T return_value;
};

// TODO: break this API into a simpler one.

export template <worker_context Context, typename... Args, async_invocable<Context, Args...> Fn>
  requires void_or_default_initializable<async_result_t<Fn, Context, Args...>>
constexpr auto
schedule(Context *context, Fn &&fn, Args &&...args) noexcept -> async_result_t<Fn, Context, Args...> {

  // TODO: make sure this is exception safe and correctly qualifed

  LF_ASSUME(context != nullptr);

  // This is what the async function will return.
  using result_type = async_result_t<Fn, Context, Args...>;

  auto root_block = std::unique_ptr<block<result_type>, block_deleter>{new block<result_type>{}};

  // TODO: clean up block if exception
  // TODO: make sure we're cancel safe

  // TODO: Before doing this we must be on a valid context.
  LF_ASSUME(get_context<Context>() == context);

  auto *promise = get(key(), ctx_invoke_t<Context>{}(std::forward<Fn>(fn), std::forward<Args>(args)...));

  // TODO: expose cancellable?
  promise->frame.parent.block = root_block.get();
  promise->frame.cancel = nullptr;
  promise->frame.stack_ckpt = get_stack<Context>().checkpoint();
  promise->frame.kind = lf::category::root;

  if constexpr (!std::is_void_v<result_type>) {
    promise->return_address = &root_block->return_value;
  }

  promise->handle().resume();

  root_block->sem.acquire();

  if constexpr (!std::is_void_v<result_type>) {
    return root_block->return_value;
  } else {
    return;
  }
}

} // namespace lf
