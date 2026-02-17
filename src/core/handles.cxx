export module libfork.core:handles;

import std;

import :frame;

namespace lf {

// =================== Handle =================== //

struct lock {};

inline constexpr lock key = {};

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
class frame_handle {
 public:
  constexpr frame_handle() = default;
  constexpr frame_handle(lock, frame_type<T> *ptr) noexcept : m_ptr(ptr) {}

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  // private:
  frame_type<T> *m_ptr = nullptr;
};

} // namespace lf
