
#pragma once

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <limits>
#include <semaphore>
#include <thread>

#include "forkpool/deque.hpp"
#include "forkpool/detail/xoshiro.hpp"

namespace riften {

static thread_local std::size_t static_id;

class Forkpool {
  public:
    static void schedule(std::coroutine_handle<> handle) {
        static Forkpool pool{2};
        std::cout << static_id << " posts job\n";
        pool._todo.fetch_add(1, std::memory_order_release);
        pool._deque[static_id].emplace(handle);
        pool._sem.release();
    }

  private:
    explicit Forkpool(std::size_t n = std::thread::hardware_concurrency()) : _deque(n + 1) {
        //
        static_id = n;  // Set master thread's id

        for (std::size_t i = 0; i < n; ++i) {
            _thread.emplace_back([&, i, n](std::stop_token tok) {
                // Set id for calls to fork
                static_id = i;

                // Initialise PRNG stream
                for (size_t j = 0; j < i; j++) {
                    detail::long_jump();
                }

            sleep:
                // Wait on semaphore
                _sem.acquire();
                std::cout << i << ": awakens\n";

            steal:
                // While there are jobs try and steal one
                while (_todo.load(std::memory_order_acquire) > 0) {
                    auto k = detail::xrand() % n;

                    if (k == i) {
                        k = n;
                    }

                    if (std::optional coro = _deque[k].steal()) {
                        _todo.fetch_sub(1, std::memory_order_release);
                        std::cout << i << ": thieves a job.\n";
                        coro->resume();
                        goto work;
                    }
                }

            work:
                // Work doing only our jobs
                while (std::optional coro = _deque[i].pop()) {
                    _todo.fetch_sub(1, std::memory_order_release);
                    std::cout << i << ": does their work.\n";
                    coro->resume();
                }

                if (_todo.load(std::memory_order_acquire) > 0) {
                    goto steal;
                } else if (!tok.stop_requested()) {
                    std::cout << i << ": sleeps\n";
                    goto sleep;
                }

                std::cout << i << ": dies\n";
            });
        }
    }

    ~Forkpool() {
        std::cout << "REQUEST KILL\n";

        for (auto& thread : _thread) {
            thread.request_stop();
        }

        _sem.release(_thread.size());

        for (auto& thread : _thread) {
            thread.join();
        }
    }

  private:
    std::counting_semaphore<> _sem{0};

    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _hint = 0;
    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _todo = 0;

    std::vector<Deque2<std::coroutine_handle<>>> _deque;

    std::vector<std::jthread> _thread;
};

}  // namespace riften