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

namespace lf {

///////////////////////////////////////////////////

export template <typename T>
concept schedulable_return = std::is_void_v<T> || std::default_initializable<T>;

template <schedulable_return T, typename Checkpoint>
struct reciver_state {

  struct empty {};

  // frame_type<Checkpoint> frame;

  [[no_unique_address]]
  std::conditional_t<std::is_void_v<T>, empty, T> m_return_value;
  std::atomic_flag m_ready;

  // TODO: destructor to clean up exception
};

export struct schedule_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

template <schedulable_return T, typename Checkpoint>
class reciver {

  using state_type = reciver_state<T, Checkpoint>;

 public:
  constexpr reciver(key_t, std::shared_ptr<state_type> &&state) : m_state(std::move(state)) {}
  constexpr reciver(reciver &&) noexcept = default;
  constexpr auto operator=(reciver &&) noexcept -> reciver & = default;

  // Move only
  constexpr reciver(const reciver &) = delete;
  constexpr auto operator=(const reciver &) -> reciver & = delete;

  [[nodiscard]]
  constexpr auto valid() const noexcept -> bool {
    return m_state != nullptr;
  }

  [[nodiscard]]
  auto ready() const -> bool {
    if (!valid()) {
      LF_THROW(schedule_error{"reciver is not valid!"});
    }
    return m_state->m_ready.test();
  }

  void wait() const {
    if (!valid()) {
      LF_THROW(schedule_error{"Invalid reciver!"});
    }
    m_state->m_ready.wait(false);
  }

  [[nodiscard]]
  auto get() -> T {

    wait();

    if (m_state->m_exception) {
      std::rethrow_exception(m_state->m_exception);
    }

    if constexpr (!std::is_void_v<T>) {
      return std::move(m_state->m_return_value);
    }
  }

 private:
  std::shared_ptr<state_type> m_state;
};

template <typename Context, typename State, typename Fn, typename... Args>
auto package(std::shared_ptr<State> recv, Fn fn, Args... args) -> root_task<checkpoint_t<Context>> {

  // This should be resumed on a valid context.
  LF_ASSUME(thread_context<Context> != nullptr);

  LF_TRY {
    using checkpoint = checkpoint_t<Context>;

    // This is a pointer to the current root_task's frame
    frame_type<checkpoint> *root = co_await get_frame_t{};

    // Now we do a manual "call" invocation.

    using result_type = async_result_t<Fn, Context, Args...>;
    using promise_type = promise_type<result_type, Context>;

    // This is a pointer to the promise
    promise_type *child = get(key(), ctx_invoke_t<Context>{}(std::move(fn), std::move(args)...));

    // TODO: cancellation

    child->frame.parent.frame = root;
    child->frame.cancel = nullptr;

    LF_ASSUME(child->frame.kind == category::call);

    if constexpr (!std::is_void_v<async_result_t<Fn, Context, Args...>>) {
      child->return_address = std::addressof(recv->m_return_value);
    }

    co_await &child->frame;

  } LF_CATCH_ALL {
    // TODO:
  }

  // Now to that which we would otherwise do at a final suspend.

  // Notify the reciver that the task is done.
  recv->m_ready.test_and_set();
  recv->m_ready.notify_one();

  co_return;
}

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
using schedule_state_t = reciver_state<invoke_decay_result_t<Fn, Context, Args...>, checkpoint_t<Context>>;

export template <typename Fn, typename Context, typename... Args>
  requires schedulable<Fn, Context, Args...>
using schedule_result_t = reciver<invoke_decay_result_t<Fn, Context, Args...>, checkpoint_t<Context>>;

export template <scheduler Sch, decay_copyable Fn, decay_copyable... Args>
  requires schedulable<Fn, context_t<Sch>, Args...>
constexpr auto
schedule2(Sch &&sch, Fn &&fn, Args &&...args) -> schedule_result_t<Fn, context_t<Sch>, Args...> {

  using context_type = context_t<Sch>;

  // TODO: make sure this is exception safe and correctly qualifed

  if (thread_context<context_type> != nullptr) {
    LF_THROW(schedule_error{"Schedule called from within a worker thread!"});
  }

  std::shared_ptr state = std::make_shared<schedule_state_t<Fn, context_type, Args...>>();

  // TODO: clean up block if exception
  // TODO: make sure we're cancel safe

  // package has shared ownership of the state.
  root_task task = package<context_type>(state, std::forward<Fn>(fn), std::forward<Args>(args)...);

  LF_TRY {
    sch.post(sched_handle<context_type>{key(), &task.promise->frame});
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
