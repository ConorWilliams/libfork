module;
#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:frame;

import std;

import libfork.utils;

namespace lf {
// =================== Cancellation =================== //

struct cancellation {
  cancellation *parent = nullptr;
  std::atomic<std::uint32_t> stop = 0;
};

// =================== Frame =================== //

// TODO: remove this and other exports

export enum category : std::uint8_t {
  call = 0,
  fork = 1,
};

export struct frame_base {};

// TODO: make everything (deque etc) allocator aware...

export template <typename Checkpoint>
struct frame_type : frame_base {

  // == Member variables == //

  // TODO: add checked accessors for all the things (including except etc)

  // Only set if an exception is thrown, otherwise uninit
  uninitialized<std::exception_ptr> except;

  frame_type *parent;
  cancellation *cancel;

  [[no_unique_address]]
  Checkpoint stack_ckpt;

  // TODO: in theory the 2 bits of information in kind_and_except
  // could be compressed into one of the pointers?

  ATOMIC_ALIGN(std::uint32_t) joins = 0;          // Atomic is 32 bits for speed
  std::uint16_t steals = 0;                       // In debug do overflow checking
  ATOMIC_ALIGN(std::uint8_t) kind_and_except = 0; // Stores fork/call in low bit, exception flag in next bit

  // == Member functions == //

  // Explicitly post construction, this allows the compiler to emit a single
  // instruction for the zero init then an instruction for the joins init,
  // instead of three instructions.
  constexpr frame_type(Checkpoint &&ckpt) noexcept : stack_ckpt(std::move(ckpt)) { joins = k_u16_max; }

  [[nodiscard]]
  constexpr auto is_cancelled() const noexcept -> bool {
    // TODO: Should exception trigger cancellation?
    for (cancellation *ptr = cancel; ptr != nullptr; ptr = ptr->parent) {
      // TODO: if users can't use cancellation outside of fork-join
      // then this can be relaxed
      if (ptr->stop.load(std::memory_order_acquire) == 1) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]]
  constexpr auto has_exception() const noexcept -> bool {
    return kind_and_except >= 2;
  }

  /**
   * @brief Returns true if caller was first to set exception bit.
   */
  [[nodiscard]]
  constexpr auto atomic_set_exception() noexcept -> bool {
    // No synchronization is done via exception_bit, hence we can use relaxed atomics
    // and rely on the usual fork/join synchronization to ensure memory ordering.
    return atomic_except().fetch_or(0b00000010, std::memory_order_relaxed) < 2;
  }

  [[nodiscard]]
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))

  [[nodiscard]]
  constexpr auto atomic_joins() noexcept -> std::atomic_ref<std::uint32_t> {
    return std::atomic_ref{joins};
  }

  constexpr void reset_counters() noexcept {
    joins = k_u16_max;
    steals = 0;
  }

 private:
  [[nodiscard]]
  constexpr auto atomic_except() noexcept -> std::atomic_ref<std::uint8_t> {
    return std::atomic_ref{kind_and_except};
  }
};

} // namespace lf
