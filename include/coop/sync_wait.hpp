#pragma once

#include <atomic>
#include <cassert>
#include <iostream>
#include <utility>

#include "coop/hot_task.hpp"
#include "coop/meta.hpp"

namespace riften {

namespace detail {

// Here we can guarantee hot_task will not dangle as sync_wait will not return until hot_task is complete
template <awaitable T> hot_task<await_result_t<T>, inline_scheduler, true> wrap(T&& hot_task) {
    co_return co_await std::forward<T>(hot_task);
}

}  // namespace detail

template <typename T, Scheduler S> T sync_wait(hot_task<T, S, true> const& hot_task) {
    return hot_task.get();
}

template <typename T, Scheduler S> T sync_wait(hot_task<T, S, true>&& hot_task) {
    return std::move(hot_task).get();
}

template <awaitable A> await_result_t<A> sync_wait(A&& hot_task) {
    return detail::wrap(std::forward<A>(hot_task)).get();
}

}  // namespace riften