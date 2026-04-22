export module libfork.core:handles;

import std;

import libfork.utils;

import :frame;

namespace lf {

// =================== Untyped handles =================== //

class handle {
 public:
  constexpr handle() = default;
  constexpr handle(key_t, frame_base *ptr) noexcept : m_ptr{ptr} {}
  constexpr auto operator==(handle const &) const noexcept -> bool = default;
  constexpr explicit operator bool() const noexcept { return m_ptr != nullptr; }

 private:
  [[nodiscard]]
  constexpr friend auto get(key_t, handle h) noexcept -> frame_base * {
    return h.m_ptr;
  }

  frame_base *m_ptr = nullptr;
};

/**
 * @brief An untyped steal-handle.
 *
 * For use by context policies that need to store handles in an untyped manner.
 */
export struct untyped_steal_handle : handle {
  using handle::handle;
};

/**
 * @brief An untyped schedule-handle.
 *
 * For use by context policies that need to store handles in an untyped manner.
 */
export struct untyped_sched_handle : handle {
  using handle::handle;
};

// =================== Tagged handles =================== //

/**
 * @brief A handle to a task that can be stolen and resumed with `execute`.
 *
 * The coroutine behind this task is always suspended at fork point.
 */
export template <typename T>
struct steal_handle : untyped_steal_handle {
  using untyped_steal_handle::untyped_steal_handle;
};

/**
 * @brief A handle to a task that can be resumed with `execute`.
 *
 * The coroutine behind this task is either not-yet-started or suspended at a context-switch.
 */
export template <typename T>
struct sched_handle : untyped_sched_handle {
  using untyped_sched_handle::untyped_sched_handle;
};

} // namespace lf
