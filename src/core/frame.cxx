module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:frame;

import std;

import :concepts;

namespace lf {

template <stack_allocator T>
struct type_erased_context;

// template <stack_allocator T>
// constexpr erase()
//
// Store's a type-erased context as void*
// TODO: just changed from default_movable to stack_allocator
export template <stack_allocator T>
struct frame_type;

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
export template <default_movable T>
class frame_handle {
 public:
  constexpr frame_handle() = default;
  constexpr frame_handle(std::nullptr_t) noexcept : m_ptr(nullptr) {}
  constexpr frame_handle(lock, frame_type<T> *ptr) noexcept : m_ptr(ptr) {}

 private:
  frame_type<T> *m_ptr;
};

// TODO: remove this and other exports
export enum class category : std::uint8_t {
  call,
  root,
  fork,
};

struct cancellation;

struct frame_type {

  frame_type *parent = nullptr; // TODO: set as at root
  cancellation *cancel = nullptr;

  std::uint32_t merges = 0;       // Atomic is 32 bits for speed
  std::uint16_t steals = 0;       // In debug do overflow checking
  category kind = category::call; // Fork/Call/Just/Root
  std::uint8_t exception_bit = 0; // Atomically set

  [[nodiscard]]
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))
};

static_assert(std::is_standard_layout_v<frame_type>);

} // namespace lf
