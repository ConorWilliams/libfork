
#pragma once

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <limits>
#include <optional>
#include <semaphore>
#include <thread>

#include "forkpool/deque.hpp"
#include "forkpool/detail/eventcount.hpp"
#include "forkpool/detail/xoshiro.hpp"
#include "forkpool/task2.hpp"

namespace riften {

static thread_local std::size_t xtatic_id;

class forkpool2 {
  public:
    static forkpool2& get() {
        static forkpool2 pool{std::thread::hardware_concurrency()};
        return pool;
    }

    static pop() {}

    static void schedule(std::coroutine_handle<> handle) {
        get()._deque[xtatic_id].emplace(handle);

        if (xtatic_id == get()._thread.size()) {
            get()._notifyer.notify_one();
        }
    }

  private:
    using task_t = std::optional<std::coroutine_handle<>>;

    explicit forkpool2(std::size_t n = std::thread::hardware_concurrency()) : _deque(n + 1) {
        // Master thread uses nth deque
        xtatic_id = n;

        for (std::size_t id = 0; id < n; ++id) {
            _thread.emplace_back([&, id] {
                // Set id for calls to fork
                xtatic_id = id;

                // Initialise PRNG stream
                for (size_t j = 0; j < id; j++) {
                    detail::long_jump();
                }

                task_t task = std::nullopt;

                while (true) {
                    exploit_task(id, task);
                    if (wait_for_task(id, task) == false) {
                        break;
                    }
                }
            });
        }
    }

    ~forkpool2() {
        _stop.store(true);
        _notifyer.notify_all();
    }

    void exploit_task(std::size_t id, task_t& task) {
        if (task) {
            if (_actives.fetch_add(1, std::memory_order_acq_rel) == 0
                && _thieves.load(std::memory_order_acquire) == 0) {
                _notifyer.notify_one();
            }

            do {
                task->resume();
                task = _deque[id].pop();
                assert(!task);  //
            } while (task);

            _actives.fetch_sub(1, std::memory_order_release);
        }
    }

    void steal_task(std::size_t id, task_t& task) {
        for (std::size_t i = 0; i < _thread.size(); ++i) {
            if (auto v = detail::xrand() % _thread.size(); v == id) {
                task = _deque.back().steal();
            } else {
                task = _deque[v].steal();
            }
            if (task) {
                return;
            }
        }
    }

    bool wait_for_task(std::size_t id, task_t& task) {
    wait_for_task:
        _thieves.fetch_add(1, std::memory_order_release);
    steal_task:
        if (steal_task(id, task); task) {
            if (_thieves.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                _notifyer.notify_one();
            }
            return true;
        }

        auto key = _notifyer.prepare_wait();

        if (!_deque.back().empty()) {
            _notifyer.cancel_wait();
            task = _deque.back().steal();
            if (task) {
                if (_thieves.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    _notifyer.notify_one();
                }
                return true;
            } else {
                goto steal_task;
            }
        }

        if (_stop.load()) {
            _notifyer.cancel_wait();
            _notifyer.notify_all();
            _thieves.fetch_sub(1, std::memory_order_release);
            return false;
        }

        if (_thieves.fetch_sub(1, std::memory_order_acq_rel) == 1
            && _actives.load(std::memory_order_acquire) > 0) {
            _notifyer.cancel_wait();
            goto wait_for_task;
        }

        // std::osyncstream(std::cout) << xtatic_id << " sleeps\n";

        _notifyer.wait(key);

        return true;
    }

  private:
    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _actives = 0;
    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _thieves = 0;
    alignas(hardware_destructive_interference_size) std::atomic<bool> _stop = false;

    event_count _notifyer;
    std::vector<Deque2<std::coroutine_handle<>>> _deque;
    std::vector<std::jthread> _thread;
};

}  // namespace riften