
#pragma once

#include <atomic>
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <optional>
#include <semaphore>
#include <thread>

#include "riften/deque.hpp"
#include "riften/detail/macrologger.h"
#include "riften/detail/xoshiro.hpp"
#include "riften/eventcount.hpp"

namespace riften::detail {

inline thread_local std::size_t root_id = std::numeric_limits<std::size_t>::max();
inline thread_local std::size_t static_id = root_id;

class Forkpool {
  private:
    struct task_handle : std::coroutine_handle<> {
        std::uint64_t* alpha;
    };

    using task_t = std::optional<task_handle>;

  public:
    static void push(task_handle handle) {
        assert(static_id < get()._thread.size());
        get()._deque[static_id].emplace(handle);
    }

    static void push_root(task_handle handle) {
        LOG_DEBUG("Root node pushed");
        assert(static_id == root_id);
        get()._deque.back().emplace(handle);
        get()._notifyer.notify_one();
    }

    static task_t pop() noexcept {
        assert(static_id < get()._thread.size());
        return get()._deque[static_id].pop();
    }

    // Can only throw on first call hence tag above as noexcept as only main thread can throw
    static Forkpool& get() {
        static Forkpool pool{std::thread::hardware_concurrency()};
        return pool;
    }

  private:
    explicit Forkpool(std::size_t n = std::thread::hardware_concurrency()) : _deque(n + 1) {
        // Master thread uses nth deque

        for (std::size_t id = 0; id < n; ++id) {
            _thread.emplace_back([&, id] {
                // Set id for calls to fork
                static_id = id;

                // Initialise PRNG stream
                for (size_t j = 0; j < id; j++) {
                    long_jump();
                }

                task_t task = std::nullopt;

                // Enter work loop
                while (true) {
                    exploit_task(id, task);
                    if (wait_for_task(id, task) == false) {
                        break;
                    }
                }
            });
        }
    }

    ~Forkpool() noexcept {
        _stop.store(true);
        _notifyer.notify_all();
    }

    void exploit_task([[maybe_unused]] std::size_t id, task_t& task) noexcept {
        if (task) {
            if (_actives.fetch_add(1, std::memory_order_acq_rel) == 0) {
                if (_thieves.load(std::memory_order_acquire) == 0) {
                    _notifyer.notify_one();
                }
            }

            task->resume();
            assert(!_deque[id].pop());

            _actives.fetch_sub(1, std::memory_order_release);
        }
    }

    void steal_task(std::size_t id, task_t& task) noexcept {
        for (std::size_t i = 0; i < _thread.size(); ++i) {
            if (auto v = xrand() % _thread.size(); v == id) {
                if ((task = _deque.back().steal())) {
                    LOG_DEBUG("Steal from master");
                    return;
                }
            } else {
                if ((task = _deque[v].steal())) {
                    LOG_DEBUG("Steal from worker");
                    *(task->alpha) += 1;
                    return;
                }
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

        if (_thieves.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            if (_actives.load(std::memory_order_acquire) > 0) {
                _notifyer.cancel_wait();
                goto wait_for_task;
            }
        }

        _notifyer.wait(key);

        return true;
    }

  private:
    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _actives = 0;
    alignas(hardware_destructive_interference_size) std::atomic<std::int64_t> _thieves = 0;
    alignas(hardware_destructive_interference_size) std::atomic<bool> _stop = false;

    EventCount _notifyer;

    std::vector<Deque<task_handle>> _deque;
    std::vector<std::jthread> _thread;
};

}  // namespace riften::detail