module;
#include "libfork/__impl/assume.hpp"
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

    LF_ASSUME(id < m_contexts.size());

    context &ctx = m_contexts[id];

    std::size_t const n = m_contexts.size();

    std::default_random_engine rng(safe_cast<unsigned>(id + 1));
    std::uniform_int_distribution<std::size_t> dist(0, n - 2);

    constexpr int k_steal_attempts = 1024;

    while (!stop.stop_requested()) {

      LF_ASSUME(ctx.get_underlying().empty()); // ctx interactions are core-managed

      if (auto lock = std::unique_lock(m_mutex); !m_posted.empty()) {
        auto task = m_posted.back();
        m_posted.pop_back();
        lock.unlock();
        execute(static_cast<context_type &>(ctx), task);
        continue;
      }

      if (n > 1) {
        for (int i = 0; i < k_steal_attempts; ++i) {

          std::size_t victim = dist(rng);

          if (victim >= id) {
            victim += 1;
          }

          LF_ASSUME(victim < n);
          LF_ASSUME(victim != id);

          if (auto result = m_contexts[victim].get_underlying().thief().steal()) {
            // resume(*result);
            LF_UNREACHABLE(); // TODO:
            continue;
          }
        }
      }
    }
  }

  void join_all() {
    m_threads.clear(); // jthread calls stop and joins in destructor
  }

  std::vector<context> m_contexts;
  std::vector<std::jthread> m_threads;
  std::mutex m_mutex;
  std::vector<sched_handle<context_type>> m_posted;
};

} // namespace lf
