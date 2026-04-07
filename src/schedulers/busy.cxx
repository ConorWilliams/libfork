module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/compiler.hpp"
export module libfork.schedulers:basic_busy_pool;

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

export enum class pool_kind { mono, poly };

export template <pool_kind Kind, worker_stack Stack>
class basic_busy_pool {

  using context = std::conditional_t<           //
      Kind == pool_kind::poly,                  //
      derived_poly_context<Stack, adapt_deque>, //
      mono_context<Stack, adapt_deque>          //
      >;

  static constexpr std::size_t k_cache_line = 64;

  struct alignas(k_cache_line) steal_counter {
    std::atomic<std::uint64_t> value{0};
  };

 public:
  using context_type = context::context_type;

  // TODO: sleep when zero work

  explicit basic_busy_pool(std::size_t n = std::thread::hardware_concurrency())
      : m_contexts(n), m_steal_counters(n) {

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

  basic_busy_pool(basic_busy_pool const &) = delete;
  basic_busy_pool(basic_busy_pool &&) = delete;

  auto operator=(basic_busy_pool const &) -> basic_busy_pool & = delete;
  auto operator=(basic_busy_pool &&) -> basic_busy_pool & = delete;

  ~basic_busy_pool() { join_all(); }

  void post(sched_handle<context_type> handle) {
    // TODO: use a lock-free queue here
    auto lock = std::unique_lock(m_mutex);
    m_posted.push_back(handle);
  }

  [[nodiscard]]
  auto steal_count() const noexcept -> std::uint64_t {
    std::uint64_t total = 0;
    for (auto const &c : m_steal_counters) {
      total += c.value.load(std::memory_order_relaxed);
    }
    return total;
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
        sched_handle task = m_posted.back();
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

          if (auto [err, result] = m_contexts[victim].get_underlying().thief().steal()) {
            m_steal_counters[id].value.fetch_add(1, std::memory_order_relaxed);
            execute(static_cast<context_type &>(ctx), result);
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
  std::vector<steal_counter> m_steal_counters;
};

export template <worker_stack Stack>
using mono_busy_pool = basic_busy_pool<pool_kind::mono, Stack>;

export template <worker_stack Stack>
using poly_busy_pool = basic_busy_pool<pool_kind::poly, Stack>;

} // namespace lf
