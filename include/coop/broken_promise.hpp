#pragma once

#include <stdexcept>

namespace riften {

struct broken_promise : std::runtime_error {
    broken_promise() : std::runtime_error("Task has no assosicated coroutine") {}
};

}  // namespace riften