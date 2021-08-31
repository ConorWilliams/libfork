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
template <awaitable T> task<await_result_t<T>, true> wrap(T&& task) {
    co_return co_await std::forward<T>(task);
}

}  // namespace detail

template <awaitable A> await_result_t<A> sync_wait(A&& task) {
    //

    auto wrapper_task = detail::wrap(std::forward<A>(task));

    std::coroutine_handle h = wrapper_task.make_promise();

    h.resume();

    return wrapper_task.get();
}

}  // namespace riften