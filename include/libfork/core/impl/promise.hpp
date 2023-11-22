#ifndef C854CDE9_1125_46E1_9E2A_0B0006BFC135
#define C854CDE9_1125_46E1_9E2A_0B0006BFC135

#include <concepts>

#include <libfork/core/invocable.hpp>
#include <type_traits>
#include <utility>

#include <libfork/core/context.hpp>
#include <libfork/core/tag.hpp>
#include <libfork/core/tls.hpp>

#include <libfork/core/impl/combinate.hpp>
#include <libfork/core/impl/frame.hpp>
#include <libfork/core/impl/utility.hpp>

namespace lf::impl {

namespace detail {

inline auto final_await_suspend(frame *parent) noexcept -> std::coroutine_handle<> {

  full_context *context = tls::context();

  if (task_handle parent_task = context->pop()) {
    // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
    LF_LOG("Parent not stolen, keeps ripping");
    LF_ASSERT(byte_cast(parent_task) == byte_cast(parent));
    // This must be the same thread that created the parent so it already owns the fibre.
    // No steals have occurred so we do not need to call reset().;
    return parent->self();
  }

  /**
   * An owner is a worker who:
   *
   * - Created the task.
   * - Had the task submitted to them.
   * - Won the task at a join.
   *
   * An owner of a task owns the fibre the task is on.
   *
   * As the worker who completed the child task this thread owns the fibre the child task was on.
   *
   * Either:
   *
   * 1. The parent is on the same fibre as the child.
   * 2. The parent is on a different fibre to the child.
   *
   * Case (1) implies: we owned the parent; forked the child task; then the parent was then stolen.
   * Case (2) implies: we stole the parent task; then forked the child; then the parent was stolen.
   *
   * In case (2) the workers fibre has no allocations on it.
   */

  LF_LOG("Task's parent was stolen");

  fibre *tls_fibre = tls::fibre();

  // Need to copy this onto stack for else-branch later.
  fibre::fibril *parents_fibril = parent->fibril();

  // Register with parent we have completed this child task.
  if (parent->fetch_sub_joins(1, std::memory_order_release) == 1) {
    // Acquire all writes before resuming.
    std::atomic_thread_fence(std::memory_order_acquire);

    // Parent has reached join and we are the last child task to complete.
    // We are the exclusive owner of the parent therefore, we must continue parent.

    LF_LOG("Task is last child to join, resumes parent");

    if (parents_fibril->top() != tls_fibre->top()) {
      // Case (2), the tls_fibre has no allocations on it.

      // TODO: fibre.splice()? Here the old fibre is empty and thrown away, if it is larger
      // then we could splice it onto the parents one? Or we could attempt to cache the old one.
      *tls_fibre = fibre{parents_fibril};
    }

    // Must reset parents control block before resuming parent.
    parent->reset();

    return parent->self();
  }

  // We did not win the join-race, we cannot deference the parent pointer now as
  // the frame may now be freed by the winner.

  // Parent has not reached join or we are not the last child to complete.
  // We are now out of jobs, must yield to executor.

  LF_LOG("Task is not last to join");

  if (parents_fibril->top() == tls_fibre->top()) {
    // We are unable to resume the parent and where its owner, as the resuming
    // thread will take ownership of the parent's we must give it up.
    LF_LOG("Thread releases control of parent's stack");

    ignore_t{} = tls_fibre->release();

  } else {
    // Case (2) the tls_fibre has no allocations on it, it may be used later.
    tls_fibre->squash();
  }

  return std::noop_coroutine();
}

} // namespace detail

/**
 * sync wait:
 *
 * if worker:
 *
 *  tmp = move(fibre);
 *  frame * = func(args...);
 *  fibre.release();
 *  fibre = move(tmp);
 *  frame->resume();
 *
 * else
 *
 *  fibre.construct();
 *  frame * = func(args...);
 *  fibre.release();
 *  fibre.destruct();
 *  frame->resume();
 */

/**
 * @brief The promise type for all tasks/coroutines.
 *
 * @tparam R The type of the return address.
 * @tparam T The value type of the coroutine (what it promises to return).
 * @tparam Context The type of the context this coroutine is running on.
 * @tparam Tag The dispatch tag of the coroutine.
 */
template <typename R, quasi_pointer I, tag Tag>
struct promise_type : frame {
  /**
   * @brief Allocate the coroutine on a new fibre.
   */
  static auto operator new(std::size_t size) -> void * { return tls::fibre()->allocate(size); }

  /**
   * @brief Deallocate the coroutine from current `fibre`s stack.
   */
  static void operator delete(void *ptr) { tls::fibre()->deallocate(ptr); }

 private:
  struct final_awaitable : std::suspend_always {
    static auto await_suspend(std::coroutine_handle<promise_type> child) noexcept -> std::coroutine_handle<> {

      if constexpr (Tag == tag::root) {

        LF_LOG("Root task at final suspend, releases semaphore and yields");

        child.promise().semaphore()->release();
        child.destroy();

        // A root task is always the first on a fibre, now it has been completed the fibre is empty.
        tls::fibre()->squash();

        return std::noop_coroutine();
      }

      LF_LOG("Task reaches final suspend, destroying child");

      frame *parent = child.promise().parent();
      child.destroy();

      if constexpr (Tag == tag::call) {
        LF_LOG("Inline task resumes parent");
        // Inline task's parent cannot have been stolen as its continuation was not
        // pushed to a queue hence, no need to reset control block. We do not
        // attempt to take the fibre because stack-eats only occur at a sync point.
        return parent->self();
      }

      return detail::final_await_suspend(parent);
    }
  };

 public:
  promise_type() noexcept
      : frame(std::coroutine_handle<promise_type>::from_promise(*this), tls::fibre()->top()) {}

  auto get_return_object() noexcept -> frame * { return this; }

  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  /**
   * @brief Terminates the program.
   */
  static void unhandled_exception() noexcept {
    noexcept_invoke([] {
      LF_RETHROW;
    });
  }

  /**
   * @brief Try to resume the parent.
   */
  auto final_suspend() const noexcept -> final_awaitable {

    LF_LOG("At final suspend call");

    // Completing a non-root task means we currently own the fibre_stack this child is on

    LF_ASSERT(this->debug_count() == 0);
    LF_ASSERT(this->steals() == 0);                                                // Fork without join.
    LF_ASSERT_NO_ASSUME(this->load_joins(std::memory_order_acquire) == k_u16_max); // Invalid state.

    return final_awaitable{};
  }

  /**
   * @brief Allocate on this ``fibre_stack``.
   */
  // template <typename U>
  // auto await_transform(co_alloc_t<U> to_alloc) noexcept {

  //   static_assert(Tag != tag::root, "Cannot allocate on root tasks");

  //   U *data = static_cast<U *>(this->stalloc(to_alloc.count * sizeof(U)));

  //   for (std::size_t i = 0; i < to_alloc.count; ++i) {
  //     std::construct_at(data + i);
  //   }

  //   struct awaitable : std::suspend_never {
  //     [[nodiscard]] auto await_resume() const noexcept -> std::span<U> { return allocated; }
  //     std::span<U> allocated;
  //   };

  //   return awaitable{{}, {data, to_alloc.count}};
  // }

  /**
   * @brief Transform a context pointer into a context-switch awaitable.
   */
  auto await_transform(Context *dest) -> detail::switch_awaitable<Context> {

    auto *fb = static_cast<frame *>(this);
    auto *sh = std::bit_cast<submit_h<Context> *>(fb);

    return {intruded_h<Context>{sh}, dest};
  }

  /**
   * @brief Transform a fork packet into a fork awaitable.
   */
  template <first_arg_tagged<tag::fork> Head, typename... Args>
  auto await_transform(packet<Head, Args...> &&packet) noexcept -> detail::fork_awaitable<Context> {
    return {{}, this, std::move(packet).template patch_with<Context>().invoke(this)};
  }

  /**
   * @brief Transform a call packet into a call awaitable.
   */
  template <first_arg_tagged<tag::call> Head, typename... Args>
  auto await_transform(packet<Head, Args...> &&packet) noexcept -> detail::call_awaitable {
    return {{}, std::move(packet).template patch_with<Context>().invoke(this)};
  }

  /**
   * @brief Transform a fork packet into a call awaitable.
   *
   * This subsumes the above `await_transform()` for forked packets if `Context::max_threads() == 1` is true.
   */
  template <first_arg_tagged<tag::fork> Head, typename... Args>
    requires single_thread_context<Context> && valid_packet<rewrite_tag<Head>, Args...>
  auto await_transform(packet<Head, Args...> &&pack) noexcept -> detail::call_awaitable {
    return await_transform(
        std::move(pack).apply([](Head head, Args &&...args) -> packet<rewrite_tag<Head>, Args...> {
          return {{std::move(head)}, std::forward<Args>(args)...};
        }));
  }

  /**
   * @brief Get a join awaitable.
   */
  auto await_transform(join_type) noexcept -> detail::join_awaitable<Context, Tag == tag::root> {
    return {this};
  }

  /**
   * @brief Transform an invoke packet into an invoke_awaitable.
   */
  template <impl::non_reference Packet>
  auto await_transform(Packet &&pack) noexcept -> detail::invoke_awaitable<Context, Packet> {
    return {{}, this, std::forward<Packet>(pack), {}};
  }
};

} // namespace lf::impl

#endif /* C854CDE9_1125_46E1_9E2A_0B0006BFC135 */
