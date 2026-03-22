module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
export module libfork.core:schedule;

import std;

import :concepts;
import :frame;
import :context;

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

export template <worker_context Context, typename... Args, async_invocable<Args...> Fn>
  requires void_or_default_initializable<async_result_t<Fn, Args...>>
constexpr auto schedule(Context *context, Fn &&fn, Args &&...args) noexcept -> auto {

  // This is what the async function will return.
  using result_type = async_result_t<Fn, Args...>;

  auto root_block = std::unique_ptr<block<result_type>, block_deleter>{new block<result_type>{}};

  // TODO: clean up block if exception
  // TODO: make sure we're cancel safe

  // TODO: Before doing this we must be on a valid context.
  LF_ASSUME(thread_context<Context> == context);
  task task = std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);

  // TODO: expose cancellable?
  task.promise->frame.parent.block = root_block.get();
  task.promise->frame.cancel = nullptr;
  task.promise->frame.stack_ckpt = thread_context<Context>->allocator().checkpoint();
  task.promise->frame.kind = lf::category::root;

  if constexpr (!std::is_void_v<result_type>) {
    task.promise->return_address = &root_block->return_value;
  }

  task.promise->handle().resume();

  root_block->sem.acquire();

  if constexpr (!std::is_void_v<result_type>) {
    return root_block->return_value;
  } else {
    return;
  }
}

} // namespace lf
