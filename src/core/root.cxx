module;
#include "libfork/__impl/assume.hpp"
export module libfork.core:root;

import std;

import :frame;
import :promise;

namespace lf {

// TODO: allocator aware!

struct get_frame_t {};

template <typename Checkpoint>
struct root_task {
  struct promise_type {

    frame_type<Checkpoint> frame{Checkpoint{}};

    struct frame_awaitable : std::suspend_never {

      frame_type<Checkpoint> *frame;

      auto await_resume() const noexcept -> frame_type<Checkpoint> * { return frame; }
    };

    constexpr auto await_transform(get_frame_t) noexcept -> frame_awaitable { return {.frame = &frame}; }

    constexpr auto get_return_object() noexcept -> root_task { return {.promise = this}; }

    constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

    constexpr static auto final_suspend() noexcept -> std::suspend_never { return {}; }

    constexpr static void return_void() noexcept {}

    [[noreturn]]
    constexpr void unhandled_exception() noexcept {
      LF_UNREACHABLE();
    }
  };

  promise_type *promise;
};

} // namespace lf
