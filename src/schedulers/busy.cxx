module;
#include "libfork/__impl/compiler.hpp"
export module libfork.schedulers:busy_scheduler;

import std;

import libfork.utils;
import libfork.core;
import libfork.batteries;

namespace lf {

struct invalid_workers_error : std::exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "A thread pool must have at least one worker.";
  }
};

export template <bool Polymorphic, worker_stack Stack>
class busy_scheduler {

  using context = std::conditional_t<           //
      Polymorphic,                              //
      derived_poly_context<Stack, adapt_deque>, //
      mono_context<Stack, adapt_deque>          //
      >;

 public:
  using context_type = context::context_type;

  explicit busy_scheduler(std::size_t n = std::thread::hardware_concurrency()) : m_contexts(n) {

    if (n < 1) {
      LF_THROW(invalid_workers_error{});
    }

    LF_TRY{
      for (std::size_t id = 0; id < n; ++id) {
        m_threads.emplace_back([this, id](std::stop_token stop) -> void {
          worker(std::move(stop), id);
        });
      }
    } LF_CATCH_ALL {
      // Force joins before members (which threads reference) are destroyed.
      join_all();
      LF_RETHROW;
    }
  }

  busy_scheduler(busy_scheduler const &) = delete;
  busy_scheduler(busy_scheduler &&) = delete;

  auto operator=(busy_scheduler const &) -> busy_scheduler & = delete;
  auto operator=(busy_scheduler &&) -> busy_scheduler & = delete;

  ~busy_scheduler() { join_all(); }

  void post(sched_handle<context_type> handle) {
    // TODO: use a lock-free queue here
    auto lock = std::unique_lock(m_mutex);
    m_posted.push_back(handle);
  }

 private:
  void worker(std::stop_token stop, std::size_t id) {

    auto &ctx = m_contexts[id];

    thread_local_context<context_type> = static_cast<context_type *>(&ctx);

    defer cleanup = [] noexcept -> auto {
      thread_local_context<context_type> = nullptr;
    };

    auto const n = m_contexts.size();

    std::default_random_engine rng(static_cast<unsigned>(id + 1));

    while (!stop.stop_requested()) {

      // 1. Pop from own deque.
      if (auto task = ctx.pop()) {
        resume(task);
        continue;
      }

      // 2. Check the posted queue for new root tasks.
      {
        auto lock = std::unique_lock(m_mutex);
        if (!m_posted.empty()) {
          auto task = m_posted.back();
          m_posted.pop_back();
          lock.unlock();
          resume(task);
          continue;
        }
      }

      // 3. Try stealing from a random other worker.
      if (n > 1) {
        auto victim = std::uniform_int_distribution<std::size_t>(0, n - 2)(rng);
        if (victim >= id) {
          ++victim;
        }
        if (auto result = m_contexts[victim].get_underlying().thief().steal()) {
          resume(*result);
          continue;
        }
      }

      std::this_thread::yield();
    }
  }

  void join_all() {
    for (auto &t : m_threads) {
      t.request_stop();
    }
    for (auto &t : m_threads) {
      t.join();
    }
  }

  static void resume(steal_handle<context_type> task) {
    auto *frame = static_cast<frame_type<checkpoint_t<context_type>> *>(get(key(), task));
    frame->handle().resume();
  }

  static void resume(sched_handle<context_type> task) {
    auto *frame = static_cast<frame_type<checkpoint_t<context_type>> *>(get(key(), task));
    frame->handle().resume();
  }

  std::vector<context> m_contexts;
  std::vector<std::jthread> m_threads;
  std::mutex m_mutex;
  std::vector<sched_handle<context_type>> m_posted;
};

} // namespace lf
