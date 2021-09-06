
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

namespace riften {

static thread_local std::size_t static_id;

class Forkpool {
  public:
    static void schedule(std::coroutine_handle<> handle) {
        static Forkpool pool{12};
        pool._deque[static_id].emplace(handle);

        if (static_id == pool._thread.size()) {
            pool._notifyer.notify_one();
        }
    }

  private:
    using task_t = std::optional<std::coroutine_handle<>>;

    explicit Forkpool(std::size_t n = std::thread::hardware_concurrency()) : _deque(n + 1) {
        // Master thread uses nth deque
        static_id = n;

        for (std::size_t id = 0; id < n; ++id) {
            _thread.emplace_back([&, id] {
                // Set id for calls to fork
                static_id = id;

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

    ~Forkpool() {
        std::cout << "POWER DOWN\n";
        _stop.store(true);
        _notifyer.notify_all();
    }

    void exploit_task(std::size_t id, task_t& task) {
        if (task) {
            if (_actives.fetch_add(1) == 0 && _thieves.load() == 0) {
                _notifyer.notify_one();
            }

            do {
                std::cerr << static_id << " starts\n";
                task->resume();
                std::cerr << static_id << " completes\n";
                task = _deque[id].pop();
            } while (task);

            _actives.fetch_sub(1);

            std::cerr << static_id << " becomes inactive\n";
        }
    }

    void steal_task(std::size_t id, task_t& task) {
        for (std::size_t i = 0; i < 10 * _thread.size(); ++i) {
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
        _thieves.fetch_add(1);
    steal_task:
        if (steal_task(id, task); task) {
            std::cerr << static_id << " steals task\n";
            if (_thieves.fetch_sub(1) == 1) {
                _notifyer.notify_one();
            }
            return true;
        }

        auto key = _notifyer.prepare_wait();

        if (!_deque.back().empty()) {
            std::cerr << static_id << " tries to steal from the master\n";
            _notifyer.cancel_wait();
            task = _deque.back().steal();
            if (task) {
                if (_thieves.fetch_sub(1) == 1) {
                    _notifyer.notify_one();
                }
                return true;
            } else {
                goto steal_task;
            }
        }

        if (_stop.load()) {
            std::cerr << static_id << " powers down\n";
            _notifyer.cancel_wait();
            _notifyer.notify_all();
            _thieves.fetch_sub(1);
            return false;
        }

        if (_thieves.fetch_sub(1) == 1 && _actives.load() > 0) {
            _notifyer.cancel_wait();
            goto wait_for_task;
        }

        _notifyer.wait(key);

        std::cerr << static_id << " wakes up\n";

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