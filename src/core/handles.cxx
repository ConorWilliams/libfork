export module libfork.core:handles;

import std;

import :frame;
import :utility;
import :concepts;

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

/**
 * @brief [TODO:description]
 *
 * @tparam T The (potentially incomplete) worker context.
 */
export template <typename T>
class frame_handle {
 public:
  constexpr frame_handle() = default;

  // Template to prevent circular dependency
  template <std::same_as<checkpoint_t<allocator_t<T>>> U>
  constexpr frame_handle(key_t, frame_type<U> *ptr) noexcept : m_ptr{ptr} {}

  constexpr auto operator==(frame_handle const &) const noexcept -> bool = default;

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  template <std::same_as<checkpoint_t<allocator_t<T>>> U>
  [[nodiscard]]
  constexpr auto get(key_t) const noexcept -> auto * {
    return static_cast<frame_type<U> *>(m_ptr);
  }

 private:
  frame_base *m_ptr = nullptr;
};

// =================== Await =================== //

// TODO: when a steal handle resumes a task and increments the number of forks
// it should check and potentially crash if the number of forks exceeds the
// maximum determined by uint16_max

export template <typename T>
class await_handle {
 public:
  constexpr await_handle() = default;
  constexpr await_handle(key_t, frame_base *ptr) noexcept : m_ptr{ptr} {}

  constexpr auto operator==(await_handle const &) const noexcept -> bool = default;

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  [[nodiscard]]
  constexpr auto get(key_t) const noexcept -> frame_base * {
    return m_ptr;
  }

 private:
  frame_base *m_ptr = nullptr;
};

} // namespace lf
