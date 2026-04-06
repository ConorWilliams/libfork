export module libfork.core:handles;

import std;

import libfork.utils;
import :frame;
import :concepts;
import :thread_locals;

namespace lf {

// =================== Frame =================== //

// TODO: api + test this is lock-free
//
// What is the API:
//  - You can push/pop it
//  - You can convert it to a "steal handle" -> which you can/must resume?
//
// What properties does it have:
//  - It is trivially copyable/constructible/destructible
//  - It has a null value, you can test if it is null
//  - You can store it in an atomic and it is lock-free

export template <typename T>
class handle {
 public:
  constexpr handle() = default;
  constexpr auto operator==(handle const &) const noexcept -> bool = default;
  constexpr explicit operator bool() const noexcept { return m_ptr != nullptr; }

  // Template to prevent circular dependency
  constexpr handle(key_t, frame_base *ptr) noexcept : m_ptr{ptr} {}

 private:
  [[nodiscard]]
  constexpr friend auto get(key_t, handle other) noexcept -> frame_base * {
    return other.m_ptr;
  }

  frame_base *m_ptr = nullptr;
};

/**
 * @brief [TODO:description]
 *
 * @tparam T The (potentially incomplete) worker context.
 */
export template <typename T>
struct steal_handle : handle<T> {
  using handle<T>::handle;
};

// =================== Await =================== //

// TODO: when a steal handle resumes a task and increments the number of forks
// it should check and potentially crash if the number of forks exceeds the
// maximum determined by uint16_max

export template <typename T>
struct sched_handle : handle<T> {

  using handle<T>::handle;

  constexpr auto resume_on(this sched_handle self, T *context) noexcept -> void {
    LF_ASSUME(context == get_context<T>());
    // get(key(), self);

    //
  }
};

} // namespace lf
