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
export enum class category : std::uint8_t {
  call = 0,
  fork,
  root,
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

  ATOMIC_ALIGN(std::uint32_t) joins = 0;        // Atomic is 32 bits for speed
  std::uint16_t steals = 0;                     // In debug do overflow checking
  category kind = static_cast<category>(0);     // Fork/Call
  ATOMIC_ALIGN(std::uint8_t) exception_bit = 0; // Atomically set

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
  constexpr auto handle() LF_HOF(std::coroutine_handle<frame_type>::from_promise(*this))

  [[nodiscard]]
  constexpr auto atomic_joins() noexcept -> std::atomic_ref<std::uint32_t> {
    return std::atomic_ref{joins};
  }

  [[nodiscard]]
  constexpr auto atomic_except() noexcept -> std::atomic_ref<std::uint8_t> {
    return std::atomic_ref{exception_bit};
  }

  constexpr void reset_counters() noexcept {
    joins = k_u16_max;
    steals = 0;
  }
};

} // namespace lf
