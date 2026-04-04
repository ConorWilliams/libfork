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
      [[nodiscard]]
      constexpr auto await_resume() const noexcept -> frame_type<Checkpoint> * {
        return frame;
      }
    };

    constexpr auto await_transform([[maybe_unused]] get_frame_t tag) noexcept -> frame_awaitable {
      return {.frame = &frame};
    }

    struct call_awaitable : std::suspend_always {
      frame_type<Checkpoint> *child;
      constexpr auto await_suspend([[maybe_unused]] coro<promise_type> root) const noexcept -> coro<> {
        return child->handle();
      }
    };

    constexpr auto await_transform(frame_type<Checkpoint> *child) noexcept -> call_awaitable {
      return {.child = child};
    }

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
