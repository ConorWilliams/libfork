#ifndef ED70014B_999F_49E9_9E2F_633E51E31469
#define ED70014B_999F_49E9_9E2F_633E51E31469

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <coroutine>
#include <exception>
#include <memory>
#include <semaphore>
#include <thread>
#include <utility>

#include "libfork/core.hpp"

namespace lf::experimental {

enum class kind : std::uint8_t { root, fork, call };

template <typename T>
struct task;

template <typename T>
struct promise;

// --------------------- //

template <typename T>
struct coroutine_deleter {
  static void operator()(promise<T> *handle) noexcept {
    std::coroutine_handle<promise<T>>::from_promise(*handle).destroy();
  }
};

template <typename T>
using unique_promise = std::unique_ptr<promise<T>, coroutine_deleter<T>>;

template <typename T>
struct task {
  using promise_type = promise<T>;
  using value_type = T;
  unique_promise<T> promise;
};

struct scope_t;

template <typename T>
struct forked : std::suspend_always {
  scope_t *scope;
  task<T> tsk;

  auto await_suspend(std::coroutine_handle<> /*unused*/) -> std::coroutine_handle<> {
    LF_LOG("Forking, push parent to context");

    // Need a copy (on stack) in case *this is destructed after push.
    // std::coroutine_handle stack_child = this->child->self();

    auto stack_child = std::exchange(tsk.promise, nullptr);

    // Hence, if this throws that is ok.
    impl::tls::context()->push(std::bit_cast<task_handle>(scope));

    auto h = std::coroutine_handle<typename task<T>::promise_type>::from_promise(*stack_child);

    stack_child.release();

    // If the above didn't throw we take ownership of child's lifetime.
    return h;
  }

  LF_FORCEINLINE void await_resume() const noexcept { LF_ASSERT(tsk.promise == nullptr); }
};

template <typename T>
struct called : std::suspend_always {

  task<T> tsk;

  auto await_suspend(std::coroutine_handle<> x) -> std::coroutine_handle<> {

    auto stack_child = std::exchange(tsk.promise, nullptr);

    auto h = std::coroutine_handle<typename task<T>::promise_type>::from_promise(*stack_child);

    stack_child.release();

    return h;
  }

  LF_FORCEINLINE void await_resume() const noexcept { LF_ASSERT(tsk.promise == nullptr); }
};

struct scope_t {
  std::coroutine_handle<> continuation; // Of parent task.
  impl::stack::stacklet *stacklet;      // Thread-local written at make_scope().

  uintptr_t tagged = 0;

  std::uint16_t n_joins = impl::k_u16_max; // No atomic operations.
  std::uint16_t m_steal = 0;               // Atomic ops to decrement.

  auto fetch_sub_joins(std::uint16_t val, std::memory_order order) noexcept -> std::uint16_t {
    return std::atomic_ref{n_joins}.fetch_sub(val, order);
  }

  void reset() noexcept {
    m_steal = 0;
    n_joins = impl::k_u16_max;
  }

  template <typename T, typename F, typename... Args>
  auto fork(T *ret, F &&f, Args &&...args) -> forked<T> {

    task<T> tsk = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);

    tsk.promise->ret = ret;
    tsk.promise->scope = this;
    tsk.promise->type = kind::fork;

    // // push pointer to scope into the context
    // impl::tls::context()->push(std::bit_cast<task_handle>(this));

    // tsk.promise.release();

    // int i = tsk;

    return {{}, this, std::move(tsk)};
  }

  template <typename T, typename F, typename... Args>
  auto call(T *ret, F &&f, Args &&...args) -> called<T> {

    task<T> tsk = std::invoke(std::forward<F>(f), std::forward<Args>(args)...);

    tsk.promise->ret = ret;
    tsk.promise->scope = this;
    tsk.promise->type = kind::call;

    // tsk.promise.release();

    return {{}, std::move(tsk)};
  }

  void take_stack_reset_frame() noexcept {
    // Steals have happened so we cannot currently own this tasks stack.
    LF_ASSERT(m_steal != 0);
    LF_ASSERT(impl::tls::stack()->empty());
    *impl::tls::stack() = impl::stack{stacklet};
    // Some steals have happened, need to reset the control block.
    reset();
  }

  /**
   * @brief Perform a `.load(order)` on the atomic join counter.
   */
  [[nodiscard]] auto load_joins(std::memory_order order) const noexcept -> std::uint16_t {
    return std::atomic_ref{n_joins}.load(order);
  }

 public:
  /**
   * @brief Shortcut if children are ready.
   */
  auto await_ready() noexcept -> bool {
    // If no steals then we are the only owner of the parent and we are ready to join.
    if (m_steal == 0) {
      LF_LOG("Sync ready (no steals)");
      // Therefore no need to reset the control block.
      return true;
    }
    // Currently:            joins() = k_u16_max - num_joined
    // Hence:       k_u16_max - joins() = num_joined

    // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
    // better if we see all the decrements to joins() and avoid suspending
    // the coroutine if possible. Cannot fetch_sub() here and write to frame
    // as coroutine must be suspended first.
    auto joined = impl::k_u16_max - load_joins(std::memory_order_acquire);

    if (m_steal == joined) {
      LF_LOG("Sync is ready");
      take_stack_reset_frame();
      return true;
    }

    LF_LOG("Sync not ready");
    return false;
  }

  /**
   * @brief Mark at join point then yield to scheduler or resume if children are done.
   */
  auto await_suspend(std::coroutine_handle<> task) const noexcept -> std::coroutine_handle<> {
    std::terminate();
  }

  /**
   * @brief Propagate exceptions.
   */
  void await_resume() const {
    LF_LOG("join resumes");
    // Check we have been reset.
    LF_ASSERT(m_steal == 0);
    LF_ASSERT(n_joins == impl::k_u16_max);
    LF_ASSERT(stacklet == impl::tls::stack()->top());

    if (tagged != 0) {
      throw "bigbad";
    }

    // self->unsafe_rethrow_if_exception();
  }
};

struct make_scope_t {};

template <typename T>
struct promise {

  T *ret; // Return address (templated on T)

  union {
    scope_t *scope;
    std::binary_semaphore *sem;
  };

  kind type; // Fork/Call (maybe root as well?)

  auto await_transform(make_scope_t) noexcept {

    struct awaitable : std::suspend_never {
      scope_t scope;

      scope_t await_resume() noexcept { return scope; }
    };

    return awaitable{
        {},
        {std::coroutine_handle<promise>::from_promise(*this), impl::tls::stack()->top()},
    };
  }

  template <typename U>
  auto await_transform(U &&x) -> U && {
    return std::forward<U>(x);
  }

  /**
   * @brief Allocate the coroutine on a new stack.
   */
  LF_FORCEINLINE static auto operator new(std::size_t size) -> void * {
    return impl::tls::stack()->allocate(size);
  }

  /**
   * @brief Deallocate the coroutine from current `stack`s stack.
   */
  LF_FORCEINLINE static void operator delete(void *ptr) noexcept { impl::tls::stack()->deallocate(ptr); }

  /**
   * @brief Start suspended (lazy).
   */
  static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  auto get_return_object() noexcept -> task<T> { return {unique_promise<T>(this)}; }

  void unhandled_exception() noexcept { std::terminate(); }

  void return_value(T &&value) { *ret = std::move(value); }

  auto final_suspend() const noexcept { return final_awaitable{}; }

 private:
  struct final_awaitable : std::suspend_always {
    static auto await_suspend(std::coroutine_handle<promise> child) noexcept -> std::coroutine_handle<> {

      promise const &prom = child.promise();

      switch (prom.type) {
        case kind::root: {
          prom.sem->release();
          child.destroy();
          return std::noop_coroutine();
        }
        case kind::call: {
          auto *parent = prom.scope;
          child.destroy();
          return parent->continuation;
        }
        case kind::fork: {
          auto *scope = prom.scope;
          child.destroy();
          impl::full_context *context = impl::tls::context();

          if (task_handle parent_task = context->pop()) {
            LF_ASSERT(impl::byte_cast(parent_task) == impl::byte_cast(scope));
            return scope->continuation;
          }

          impl::stack *tls_stack = impl::tls::stack();

          impl::stack::stacklet *p_stacklet = scope->stacklet;
          impl::stack::stacklet *c_stacklet = tls_stack->top();

          // Register with parent we have completed this child task, this may release ownership of our stack.
          if (scope->fetch_sub_joins(1, std::memory_order_release) == 1) {
            // Acquire all writes before resuming.
            std::atomic_thread_fence(std::memory_order_acquire);

            if (p_stacklet != c_stacklet) {
              // Case (2), the tls_stack has no allocations on it.

              LF_ASSERT(tls_stack->empty());

              // TODO: stack.splice()? Here the old stack is empty and thrown away, if it is larger
              // then we could splice it onto the parents one? Or we could attempt to cache the old one.
              *tls_stack = impl::stack{p_stacklet};
            }

            // Must reset parents control block before resuming parent.
            scope->reset();

            return scope->continuation;
          }

          // We did not win the join-race, we cannot deference the parent pointer now as
          // the frame may now be freed by the winner.

          // Parent has not reached join or we are not the last child to complete.
          // We are now out of jobs, must yield to executor.

          LF_LOG("Task is not last to join");

          if (p_stacklet == c_stacklet) {
            // We are unable to resume the parent and where its owner, as the resuming
            // thread will take ownership of the parent's we must give it up.
            LF_LOG("Thread releases control of parent's stack");

            // If this throw an exception then the worker must die as it does not have a stack.
            // Hence, program termination is appropriate.
            impl::ignore_t{} = tls_stack->release();

          } else {
            // Case (2) the tls_stack has no allocations on it, it may be used later.
          }

          return std::noop_coroutine();
        }
        default:
          impl::unreachable();
      }
    }
  };
};

class unit_pool;

struct trans {
  std::coroutine_handle<> continuation;
  impl::stack::stacklet *stacklet;
};

inline void resume(trans &t) {
  impl::stack *stack = impl::tls::stack();
  LF_ASSERT(stack->empty());
  *stack = impl::stack{t.stacklet};
  t.continuation.resume();
}

class unit_pool : impl::immovable<unit_pool> {

  static void work(unit_pool *self) {

    worker_context *me = lf::worker_init(lf::nullary_function_t{[]() {}});

    LF_DEFER { lf::finalize(me); };

    self->m_context = me;
    self->m_ready.test_and_set(std::memory_order_release);
    self->m_ready.notify_one();

    while (!self->m_stop.test(std::memory_order_acquire)) {
      if (auto job = self->m_submit.try_pop_all()) {
        for_each_elem(job, [](trans &raw) LF_STATIC_CALL {
          resume(raw);
        });
      }
    }

    // Drain the queue.
    while (auto *job = self->m_submit.try_pop_all()) {
      for_each_elem(job, [](trans &raw) LF_STATIC_CALL {
        resume(raw);
      });
    }
  }

 public:
  /**
   * @brief Construct a new unit pool.
   */
  unit_pool() : m_thread{work, this} {
    // Wait until worker sets the context.
    m_ready.wait(false, std::memory_order_acquire);
  }

  /**
   * @brief Run a job inline.
   */
  void schedule(intrusive_list<trans>::node *job) { m_submit.push(job); }

  /**
   * @brief Destroy the unit pool object, waits for the worker to finish.
   */
  ~unit_pool() noexcept {
    m_stop.test_and_set(std::memory_order_release);
    m_thread.join();
  }

 private:
  std::atomic_flag m_stop = ATOMIC_FLAG_INIT;
  std::atomic_flag m_ready = ATOMIC_FLAG_INIT;

  intrusive_list<trans> m_submit;

  lf::worker_context *m_context = nullptr;

  std::thread m_thread;
};

template <typename F, class... Args>
LF_CLANG_TLS_NOINLINE auto schedule(unit_pool &sch, F &&fun, Args &&...args) {
  //
  if (impl::tls::has_stack || impl::tls::has_context) {
    LF_THROW(schedule_in_worker{});
  }

  // Initialize the non-worker's stack.
  impl::tls::thread_stack.construct();
  impl::tls::has_stack = true;

  // Clean up the stack on exit.
  LF_DEFER {
    impl::tls::thread_stack.destroy();
    impl::tls::has_stack = false;
  };

  auto task = std::invoke(std::forward<F>(fun), std::forward<Args>(args)...);

  typename decltype(task)::value_type ret;
  std::binary_semaphore sem{0};

  task.promise->ret = &ret;
  task.promise->sem = &sem;
  task.promise->type = kind::root;

  intrusive_list<trans>::node node{
      {std::coroutine_handle<typename decltype(task)::promise_type>::from_promise(*task.promise),
       impl::tls::thread_stack->top()}};

  // If this throws then `await` will clean up the coroutine.
  impl::ignore_t{} = impl::tls::thread_stack->release();

  // Schedule upholds the strong exception guarantee hence, if it throws `await` cleans up.
  sch.schedule(&node);

  // If -^ didn't throw then we release ownership of the coroutine, it will be cleaned up by the worker.
  impl::ignore_t{} = task.promise.release();

  sem.acquire();

  return ret;
}

} // namespace lf::experimental

#endif /* ED70014B_999F_49E9_9E2F_633E51E31469 */
