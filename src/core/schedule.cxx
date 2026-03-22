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

// TODO: void specialization of block

template <typename T>
  requires std::is_void_v<T> || std::default_initializable<T>
struct block;

export template <typename T>
auto make_block() noexcept -> std::unique_ptr<block<T>, block_deleter> {
  return std::unique_ptr<block<T>, block_deleter>{new block<T>{}};
}

template <>
struct block<void> final : block_type {
 private:
  friend auto make_block<void>() noexcept -> std::unique_ptr<block<void>, block_deleter>;
};

template <std::default_initializable T>
struct block<T> final : block_type {

  [[no_unique_address]]
  T return_value;

 private:
  friend auto make_block<T>() noexcept -> std::unique_ptr<block<T>, block_deleter>;
};

// TODO: break this API into a simpler one.

export template <worker_context Context, typename... Args, async_invocable<Args...> Fn>
LF_FORCE_INLINE constexpr auto schedule(Context *context, Fn &&fn, Args &&...args) noexcept -> auto {

  // This is what the async function will return.
  using result_type = async_result_t<Fn, Args...>;

  std::unique_ptr block = make_block<result_type>();

  // TODO: Before doing this we must be on a valid context.
  LF_ASSUME(thread_context<Context> == context);
  task task = std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);

  // TODO: expose cancellable?
  task.promise->frame.parent.block = block.get();
  task.promise->frame.cancel = nullptr;
  task.promise->frame.stack_ckpt = thread_context<Context>->allocator().checkpoint();
  task.promise->frame.kind = lf::category::root;

  if constexpr (!std::is_void_v<result_type>) {
    task.promise->return_address = &block->return_value;
  }

  task.promise->handle().resume();

  block->sem.acquire();

  if constexpr (!std::is_void_v<result_type>) {
    return block->return_value;
  } else {
    return;
  }
}

} // namespace lf
