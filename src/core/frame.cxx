module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:frame;

import std;

import :concepts;

namespace lf {

// TODO: remove this and other exports
export enum class category : std::uint8_t {
  call = 0,
  root,
  fork,
};

struct cancellation {};

// =================== Frame =================== //

export template <typename Context>
struct frame_type {

  using context_type = Context;
  using allocator_type = allocator_t<Context>;
  using checkpoint_type = checkpoint_t<allocator_type>;

  frame_type *parent = nullptr;
  cancellation *cancel = nullptr;
  [[no_unique_address]]
  checkpoint_type stack_ckpt;

  std::uint32_t merges = 0;       // Atomic is 32 bits for speed
  std::uint16_t steals = 0;       // In debug do overflow checking
  category kind = category::call; // Fork/Call/Just/Root
  std::uint8_t exception_bit = 0; // Atomically set

  [[nodiscard]]
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))
};

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
  // constexpr frame_handle(std::nullptr_t) noexcept : m_ptr(nullptr) {}
  constexpr frame_handle(lock, frame_type<T> *ptr) noexcept : m_ptr(ptr) {}

  explicit operator bool() const noexcept { return m_ptr != nullptr; }

  // private:
  frame_type<T> *m_ptr;
};

// =================== First arg =================== //

} // namespace lf
