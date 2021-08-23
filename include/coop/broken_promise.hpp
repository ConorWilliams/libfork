#pragma once

#include <stdexcept>

namespace riften {

struct broken_promise : std::logic_error {
    broken_promise() : std::logic_error("Task has been detached from its promise/coroutine") {}
};

}  // namespace riften
