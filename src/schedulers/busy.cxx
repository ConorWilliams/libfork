export module libfork.schedulers:busy_scheduler;

import std;

import libfork.utils;
import libfork.core;
import libfork.batteries;

namespace lf {

template <typename Derived, typename Base>
concept derived_context_from = worker_context<Base> && std::derived_from<Derived, Base>;

template <typename Context>
concept derived_worker_context =
    has_context_typedef<Context> && derived_context_from<Context, context_t<Context>>;

export template <bool Polymorphic, worker_stack Stack>
class busy_scheduler {

  using context = std::conditional_t<           //
      Polymorphic,                              //
      derived_poly_context<Stack, adapt_deque>, //
      mono_context<Stack, adapt_deque>          //
      >;

  static_assert(derived_worker_context<context>);

 public:
  using context_type = context::context_type;

  explicit busy_scheduler(std::size_t num_threads = std::thread::hardware_concurrency())
      : m_contexts(num_threads) {
    m_threads.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
      m_threads.emplace_back([this, i](std::stop_token stop) {
        worker(stop, i);
      });
    }
  }

  ~busy_scheduler() {
    for (auto &t : m_threads) {
      t.request_stop();
    }
    for (auto &t : m_threads) {
      t.join();
    }
  }

  busy_scheduler(busy_scheduler const &) = delete;
  busy_scheduler(busy_scheduler &&) = delete;
  auto operator=(busy_scheduler const &) -> busy_scheduler & = delete;
  auto operator=(busy_scheduler &&) -> busy_scheduler & = delete;

  void post(sched_handle<context_type> handle) {
    auto lock = std::unique_lock(m_mutex);
    m_posted.push_back(handle);
  }

 private:
  void worker(std::stop_token stop, std::size_t id) {

    auto &ctx = m_contexts[id];

    thread_local_context<context_type> = static_cast<context_type *>(&ctx);

    defer cleanup = [] noexcept {
      thread_local_context<context_type> = nullptr;
    };

    auto const n = m_contexts.size();

    std::minstd_rand rng(static_cast<unsigned>(id + 1));

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
