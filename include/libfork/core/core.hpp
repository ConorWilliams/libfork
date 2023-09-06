#ifndef D66428B1_3B80_45ED_A7C2_6368A0903810
#define D66428B1_3B80_45ED_A7C2_6368A0903810

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>

#include "libfork/core/coroutine.hpp"

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

/**
 * @file core.hpp
 *
 * @brief Provides async cactus-stack and it's control blocks.
 */

namespace lf {

// ----------------------------------------------- //

/**
 * @brief A fraction of a thread's cactus stack.
 */
struct alignas(detail::k_new_align) async_stack : detail::immovable<async_stack>, std::array<std::byte, detail::k_mebibyte> {
  static consteval auto size() noexcept -> std::size_t {
    return detail::k_mebibyte;
  }
};

static_assert(std::is_standard_layout_v<async_stack>);
static_assert(sizeof(async_stack) == async_stack::size(), "Spurious padding in async_stack!");
static_assert(async_stack::size() >= detail::k_kibibyte, "Stack is too small!");

// ----------------------------------------------- //

namespace detail {

/**
 * @brief A small control structure that a root tasks use to communicate with the main thread.
 */
template <typename T>
struct root_block;

template <>
struct root_block<void> : immovable<root_block<void>> {
  std::binary_semaphore semaphore{0};
};

template <typename T>
struct root_block : root_block<void> {

  std::optional<T> result{};

  template <typename U>
    requires std::constructible_from<std::optional<T>, U>
  constexpr auto operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) -> root_block & {

    LF_LOG("Root task assigns");

    LF_ASSERT(!result.has_value());

    result.emplace(std::forward<U>(expr));

    return *this;
  }
};

// ----------------------------------------------- //

inline namespace LF_DEPENDANT_ABI {

/**
 * @brief A base class that (compile-time) conditionally adds debugging information.
 */
struct debug_block {
  // Increase the debug counter
  constexpr void debug_inc() noexcept {
#ifndef NDEBUG
    ++m_count;
#endif
  }

  // Fetch the debug count
  [[nodiscard]] constexpr auto debug_count() const noexcept -> std::int32_t {
#ifndef NDEBUG
    return m_count;
#else
    return 0;
#endif
  }

  // Reset the debug counter
  constexpr void debug_reset() noexcept {
#ifndef NDEBUG
    m_count = 0;
#endif
  }

#ifndef NDEBUG
private:
  std::int32_t m_count = 0;            ///< Number of forks/calls (debug).
  std::array<std::int32_t, 3> m_pad{}; ///< In the future we can add more symbols for async-stack tracing.
#endif
};

} // namespace LF_DEPENDANT_ABI

static_assert(sizeof(debug_block) % k_new_align == 0);
static_assert(std::is_trivially_destructible_v<debug_block>);

// ----------------------------------------------- //

/**
 * @brief A small bookkeeping struct allocated immediately before each coroutine frame.
 */
class frame_block : immovable<frame_block>, debug_block {
public:
  /**
   * @brief A lightweight nullable handle to a `frame_block` that contains the public API.
   */
  class handle {
  public:
    /**
     * @brief Resume a stolen task.
     */
    void resume() noexcept {
      LF_LOG("Call to resume on stolen task");
      m_frame_block->m_steal += 1;
      m_frame_block->get_self().resume();
    }

  private:
    frame_block *m_frame_block = nullptr;

    friend class frame_block;
  };

  static constexpr auto from_handle(frame_block *frame) { return handle{frame}; }

  // For root blocks
  explicit constexpr frame_block(stdx::coroutine_handle<> self) noexcept {
    post_init(self, nullptr);
  }

  // For non-root blocks -- require a call to post-init later.
  explicit constexpr frame_block(std::uintptr_t prev_frame_offset) : m_prev_frame_offset(prev_frame_offset) {
    LF_ASSUME(prev_frame_offset <= k_u16_max);
    LF_ASSUME(prev_frame_offset >= sizeof(frame_block));
  }

  /**
   * @brief Set the handle to the adjacent/paired coroutine frame and the parent.
   */
  void post_init(stdx::coroutine_handle<> self, frame_block *parent) noexcept {

    std::uintptr_t self_offset = as_integer(self.address()) - as_integer(this);
    LF_ASSUME(self_offset <= k_u16_max);
    LF_ASSUME(self_offset >= sizeof(frame_block));
    m_self_offset = static_cast<std::uint16_t>(self_offset);

    m_parent = parent;
  }

  auto get_prev_frame() const noexcept -> frame_block * {
    LF_ASSUME(m_prev_frame_offset >= sizeof(frame_block));
    return std::bit_cast<frame_block *>(byte_cast(this) + m_prev_frame_offset);
  }

  /**
   * @brief Get a handle to the adjacent/paired coroutine frame.
   */
  auto get_self() const noexcept -> stdx::coroutine_handle<> {
    LF_ASSUME(m_self_offset >= sizeof(frame_block));
    return stdx::coroutine_handle<>::from_address(std::bit_cast<void *>(byte_cast(this) + m_self_offset));
  }

  /**
   * @brief Get parent (checked).
   */
  auto get_parent() const noexcept -> frame_block * {
    LF_ASSUME(has_parent());
    return m_parent;
  }

  auto has_parent() const noexcept -> bool {
    return m_parent != nullptr;
  }

private:
  /**
   * We could store the offsets real pointers and avoid the addition/subtraction.
   *
   * R = root, F = fork/call.
   */
  std::atomic_uint16_t m_join = k_u16_max; ///< [R/F] Number of children joined (with offset).
  std::uint16_t m_steal = 0;               ///< [R/F] Number of steals.
  std::uint16_t m_prev_frame_offset = 0;   ///< [F]   For future aligned coroutine new/delete.
  std::uint16_t m_self_offset = 0;         ///< [R/F] Offset from `this` to paired coroutine's void handle address.
  frame_block *m_parent = nullptr;         ///< [R/F] Parent task (roots don't have one).
};

static_assert(alignof(frame_block) <= k_new_align);
static_assert(sizeof(frame_block) % k_new_align == 0);
static_assert(std::is_trivially_destructible_v<frame_block>);

// ----------------------------------------------- //

constinit inline thread_local frame_block *asp = nullptr; ///< Async Stack Pointer.

inline void set_asp(async_stack *stack) {
  // Ok without launder as we will not read asp pointer, only increment.
  LF_ASSUME(stack);
  asp = std::bit_cast<frame_block *>(stack->data() + async_stack::size());
}

inline auto from_asp() noexcept -> async_stack * {
  LF_ASSUME(asp);
  LF_ASSUME(as_integer(asp) >= async_stack::size());
  return std::bit_cast<async_stack *>(byte_cast(asp) - async_stack::size());
}

// ----------------------------------------------- //

/**
 * @brief A promise base class that allocates on an `async_stack`
 */
struct promise_alloc {
private:
  // Convert an alignment to a std::uintptr_t, ensure its is a power of two greater and >= k_new_align.
  constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
    auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
    LF_ASSUME(std::has_single_bit(align));
    return std::max(align, k_new_align);
  }

public:
  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine frame.
   *
   * This will update `asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) noexcept -> void * {
    return promise_alloc::operator new(size, std::align_val_t{k_new_align});
  }

  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine frame.
   *
   * This will update `asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size, std::align_val_t al) noexcept -> void * {

    std::uintptr_t align = unwrap(al);

    frame_block *prev_asp = asp;

    LF_ASSUME(prev_asp);

    auto prev_stack_addr = as_integer(prev_asp);

    std::uintptr_t coro_frame_addr = (prev_stack_addr - size) & ~(align - 1);

    LF_ASSERT(coro_frame_addr % align == 0);

    std::uintptr_t frame_addr = coro_frame_addr - sizeof(frame_block);

    LF_ASSERT(frame_addr % alignof(frame_block) == 0);

    // Starts the lifetime of the new frame block.
    asp = new (std::bit_cast<void *>(frame_addr)) frame_block{prev_stack_addr - frame_addr};

    return std::bit_cast<void *>(coro_frame_addr);
  }

  /**
   * @brief Move `asp` to the previous frame.
   */
  static void operator delete([[maybe_unused]] void *ptr) noexcept {
    LF_ASSERT(ptr == asp - 1); // Check we are deleting the top of the stack.
    asp = asp->get_prev_frame();
  }

  /**
   * @brief Move `asp` to the previous frame.
   */
  static void operator delete(void *ptr, std::size_t, std::align_val_t) noexcept {
    promise_alloc::operator delete(ptr);
  }
};

} // namespace detail

/**
 * @brief An aliases to the internal type that controls a task.
 *
 * Pointers given/received to this type are all non-owning, the only method
 * implementors of a context are permitted to use is `resume()`.
 */
using task_block = detail::frame_block;

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
