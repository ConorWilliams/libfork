module;
#include "libfork/__impl/assume.hpp"
export module libfork.core:root;

import std;

import :frame;
import :promise;

namespace lf {

// TODO: allocator aware!

template <typename Checkpoint>
struct root_task {
  struct promise_type {

    constexpr auto get_return_object() noexcept -> root_task { return {.promise = this}; }

    constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    constexpr static auto final_suspend() noexcept -> std::suspend_always { return {}; }

    [[noreturn]]
    constexpr void unhandled_exception() noexcept {
      stash_current_exception(frame);
    }

    constexpr static void return_void() noexcept {}

    frame_type<Checkpoint> frame{Checkpoint{}};
  };

  promise_type *promise;
};

} // namespace lf
