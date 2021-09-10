#pragma once

#include <atomic>
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#include "riften/detail/promise.hpp"
#include "riften/forkpool.hpp"

namespace riften {

template <typename T> class [[nodiscard]] task;
template <typename T> class [[nodiscard]] fut;

// An awaitable type (in the context of task<T>) that signifies a task should wait for its children
struct sync_tag {};
inline constexpr sync_tag sync() { return {}; }

// An awaitable type (in the context of task<T>) that signifies a task should post its continuation,
// garanteed to be non-null.
template <typename T> struct fork_tag : std::coroutine_handle<typename task<T>::promise_type> {};

// A task manages a coroutine frame that is scheduled at initial suspend by the customisation point
// Scheduler. A hot task begins executing on the scheduler immidiatly upon creation and is scheduled via
// child stealing, hot tasks do NOT assume strict fork/join
template <typename T> class [[nodiscard]] task {
  public:
    class promise_type : public detail::promise_result<T> {
      private:
        struct final_awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> task) noexcept {
                //
                if (std::optional task_handle = Forkpool::pop()) {
                    // No-one stole continuation, just keep rippin!
                    return *task_handle;
                }

                if (!task.promise()._parent_n) {
                    // This must be a root task and we must be out of tasks
                    return std::noop_coroutine();
                }

                // Else: register with parent we have completed this child task
                std::uint64_t n = (task.promise()._parent_n)->fetch_sub(1, std::memory_order_acq_rel);

                if (n == 1) {
                    // Parent has called sync and we are the last child task to complete, hence we continue
                    // parent.
                    return task.promise()._parent;
                } else {
                    // Parent has not called sync, so we are out of jobs, return to stealing
                    return std::noop_coroutine();
                }
            }
        };

      public:
        task get_return_object() noexcept {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() const noexcept { return {}; }

        final_awaitable final_suspend() const noexcept { return {}; }

        template <typename U> auto await_transform(fork_tag<U> child) {
            //
            struct awaitable : std::suspend_always {
                //
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> parent) {
                    //
                    _child.promise().set_parent(parent);

                    // In-case *this (awaitable) is destructed by stealer after schedule
                    std::coroutine_handle<> on_stack_handle = _child;

                    Forkpool::schedule({parent, std::addressof(parent.promise()._alpha)});

                    return on_stack_handle;
                }

                fut<U> await_resume() const noexcept { return fut<U>{_child}; }

                std::coroutine_handle<typename task<U>::promise_type> _child;
            };

            return awaitable{std::move(child)};
        }

        auto await_transform(sync_tag) {
            struct awaitable {
                constexpr bool await_ready() const noexcept { return _ready; }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> task) {
                    // Currently        n = i_max - num_joined
                    // If we perform    n <- n - (imax - num_stolen)
                    // then             n = num_stolen - num_joined
                    // and a

                    std::uint64_t tmp = i_max - task.promise()._alpha;

                    std::uint64_t n = task.promise._n.fetch_sub(tmp, std::memory_order_acq_rel);

                    if (i_max - n == _alpha) {
                        // We set n after all children had completed therefore we can resume task
                        return task;
                    } else {
                        // Someone else is responsible for running this task and we have run out of work
                        return std::noop_coroutine();
                    }
                }

                constexpr void await_resume() const noexcept {}

                bool _ready;
            };

            // Check if num-joined == num-stolen
            return awaitable{!_alpha || _alpha == i_max - _n.load(std::memory_order_acquire)};
        }

        std::atomic_uint64_t* n() noexcept { return std::addressof(_n); }

        template <typename U>
        void set_parent(std::coroutine_handle<typename task<U>::promise_type> parent) noexcept {
            _parent = parent;
            _parent_n = parent.promise().n();
        }

      private:
        std::coroutine_handle<> _parent = nullptr;
        std::atomic_uint64_t* _parent_n = nullptr;

        alignas(hardware_destructive_interference_size) std::atomic_uint64_t _alpha = 0;
        alignas(hardware_destructive_interference_size) std::atomic_uint64_t _n = i_max;

        static constexpr std::uint64_t i_max = std::numeric_limits<std::uint64_t>::max();
    };

    //////////////////////////////////////////////////////////////////////////////////////

    // Initialise empty task
    constexpr task() : _coroutine{nullptr} {}

    // No assignment/copy constructor, tasks are 'unique'
    task(const task&) = delete;
    task& operator=(task const&) = delete;

    // but, they can be moved.
    task(task&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    task& operator=(task&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    friend void swap(task& lhs, task& rhs) noexcept { std::swap(lhs._coroutine, rhs._coroutine); }

    ~task() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

    decltype(auto) launch() && {
        //

        Forkpool::schedule(_coroutine);
        _coroutine.promise()._done.wait(true);
        return std::move(_coroutine.promise()).get();
    }

    auto fork() && {
        //
        struct awaitable : std::suspend_always {
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) {
                //
                std::coroutine_handle<promise_type> on_stack_coro = _coroutine_of_task;

                on_stack_coro.promise().set_continuation(handle);

                try {
                    handle.promise()._forks.fetch_add(1, std::memory_order_release);
                    Forkpool::schedule(handle);
                    return on_stack_coro;
                } catch (...) {
                    // Scheduling failed therefore, we are responsible for cleaning up coroutine as we have
                    // only valid handle to _coroutine
                    destroy(on_stack_coro);
                    throw;
                }
            }

            auto await_resume() { return fut<T>(_coroutine_of_task); }

            std::coroutine_handle<promise_type> _coroutine_of_task;
        };

        if (_coroutine) {
            return awaitable{{}, std::exchange(_coroutine, nullptr)};
        } else {
            throw broken_promise{};
        }
    }

  private:
    friend class promise_type;
    // Only promise can construct a filled task
    explicit task(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            LOG_DEBUG("task .destroy() calls destroy");
            handle.destroy();
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

template <typename T> class [[nodiscard]] fut {
  private:
    using promise_type = task<T>::promise_type;

  public:
    // Initialise empty fut
    constexpr fut() : _coroutine{nullptr} {}

    explicit fut(std::coroutine_handle<promise_type> coro) noexcept : _coroutine(coro) {}

    // No assignment/copy constructor, futs are 'unique'
    fut(const fut&) = delete;
    fut& operator=(fut const&) = delete;

    // but, they can be moved.
    fut(fut&& other) noexcept : _coroutine(std::exchange(other._coroutine, nullptr)) {}

    fut& operator=(fut&& other) noexcept {
        if (this != &other) {
            destroy(std::exchange(_coroutine, std::exchange(other._coroutine, nullptr)));
        }
    }

    friend void swap(fut& lhs, fut& rhs) noexcept { std::swap(lhs._coroutine, rhs._coroutine); }

    ~fut() noexcept { destroy(std::exchange(_coroutine, nullptr)); }

    decltype(auto) operator*() const& {
        if (_coroutine) {
            return _coroutine.promise().get();
        } else {
            throw broken_promise{};
        }
    }

    decltype(auto) operator*() && {
        //

        if (_coroutine) {
            std::move(_coroutine.promise()).get();
        } else {
            throw broken_promise{};
        }
    }

  private:
    // Release coroutine owned by handle
    static void destroy(std::coroutine_handle<promise_type> handle) noexcept {
        if (handle) {
            LOG_DEBUG("fut destructor calls destroy");
            handle.destroy();
            return;
        }
    }

    std::coroutine_handle<promise_type> _coroutine;
};

}  // namespace riften
