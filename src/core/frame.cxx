module;
#include "libfork/__impl/utils.hpp"
export module libfork.core:frame;

import std;

import :concepts;
import :constants;

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

  frame_type *parent;
  cancellation *cancel;

  [[no_unique_address]]
  checkpoint_type stack_ckpt;

  ATOMIC_ALIGN(std::uint32_t) joins = 0;        // Atomic is 32 bits for speed
  std::uint16_t steals = 0;                     // In debug do overflow checking
  category kind = static_cast<category>(0);     // Fork/Call/Just/Root
  ATOMIC_ALIGN(std::uint8_t) exception_bit = 0; // Atomically set

  // Explicitly post construction, this allows the compiler to emit a single
  // instruction for the zero init then an instruction for the joins init,
  // instead of three instructions.
  constexpr frame_type() noexcept { joins = k_u16_max; }

  [[nodiscard]]
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))

  [[nodiscard]]
  constexpr auto atomic_fetch_sub_joins(std::uint32_t n, std::memory_order order) noexcept -> std::uint32_t {
    return std::atomic_ref{joins}.fetch_sub(n, order);
  }

  constexpr void reset() noexcept {
    joins = k_u16_max;
    steals = 0;
  }
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
  frame_type<T> *m_ptr = nullptr;
};

// =================== First arg =================== //

} // namespace lf
