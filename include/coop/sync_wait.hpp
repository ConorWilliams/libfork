#pragma once

#include <atomic>
#include <cassert>
#include <iostream>
#include <utility>

#include "coop/meta.hpp"
#include "coop/task.hpp"

namespace riften {

namespace detail {

// Here we can guarantee task will not dangle as sync_wait will not return until task is complete
template <awaitable T> task<await_result_t<T>, inline_scheduler, true> wrap(T&& task) {
    co_return co_await std::forward<T>(task);
}

}  // namespace detail

template <typename T, Scheduler S> T sync_wait(task<T, S, true> const& task) { return task.get(); }

template <typename T, Scheduler S> T sync_wait(task<T, S, true>&& task) { return std::move(task).get(); }

template <awaitable A> await_result_t<A> sync_wait(A&& task) {
    return detail::wrap(std::forward<A>(task)).get();
}

}  // namespace riften