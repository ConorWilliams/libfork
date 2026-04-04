module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
export module libfork.core:schedule;

import std;

import :concepts;
import :frame;
import :thread_locals;
import :promise;

namespace lf {

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
  promise->frame.stack_ckpt = get_allocator<Context>().checkpoint();
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

export struct schedule_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

export template <worker_context Context, typename... Args, async_invocable<Context, Args...> Fn>
  requires void_or_default_initializable<async_result_t<Fn, Context, Args...>>
constexpr auto
schedule2(Context &context, Fn &&fn, Args &&...args) noexcept -> async_result_t<Fn, Context, Args...> {

  // TODO: make sure this is exception safe and correctly qualifed

  LF_ASSUME(context != nullptr);

  if (thread_context<Context> != nullptr) {
    LF_THROW(schedule_error{"Schedule called from within a worker thread!"});
  }

  // This is what the async function will return.
  using result_type = async_result_t<Fn, Context, Args...>;

  auto root_block = std::unique_ptr<block<result_type>, block_deleter>{new block<result_type>{}};

  // TODO: clean up block if exception
  // TODO: make sure we're cancel safe

  // TODO: Before doing this we must be on a valid context.
  LF_ASSUME(get_context<Context>() == context);

  // The following invocation of the async function will access `context`

  auto *promise = get(key(), ctx_invoke_t<Context>{}(std::forward<Fn>(fn), std::forward<Args>(args)...));

  // TODO: expose cancellable?
  promise->frame.parent.block = root_block.get();
  promise->frame.cancel = nullptr;
  promise->frame.stack_ckpt = get_allocator<Context>().checkpoint();
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
