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
 * @file stack.hpp
 *
 * @brief Provides an async cactus-stack, control blocks and memory management.
 */

namespace lf {

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

static_assert(sizeof(debug_block) % k_new_align == 0);
static_assert(std::is_trivially_destructible_v<debug_block>);

// ----------------------------------------------- //

class frame_block; ///< Forward declaration, impl below.

} // namespace LF_DEPENDANT_ABI

/**
 * @brief The async stack pointer.
 *
 * Each thread has a pointer into its current async stack, the end of the stack is marked with a sentinel
 * `frame_block`. It is assumed that the runtime initializes this to a sentinel-guarded `async_stack`. A
 * stack looks a little bit like this:
 *
 * .. code::
 *
 *    |--------------- async_stack object ---------------|
 *
 *                 R[coro_frame].....R[coro_frame].......S
 *                 |---------------->|------------------>|--> Pointer to parent (some other stack)
 *
 * `R` = regular `frame_block`, `S` = sentinel `frame_block`.
 */
constinit inline thread_local frame_block *asp = nullptr;

// ----------------------------------------------- //

inline namespace LF_DEPENDANT_ABI {

/**
 * @brief A small bookkeeping struct allocated immediately before/after each coroutine frame.
 */
class frame_block : immovable<frame_block>, debug_block {
public:
  /**
   * @brief A lightweight nullable handle to a `frame_block` that contains the public API for thieves.
   */
  class thief_handle {
  public:
    /**
     * @brief Resume a stolen task.
     */
    void resume() const noexcept {
      LF_LOG("Call to resume on stolen task");

      // Link the sentinel to the parent.
      LF_ASSUME(asp);
      LF_ASSUME(asp->m_kind == sentinel);
      asp->m_parent = m_frame_block;

      LF_ASSUME(m_frame_block);
      LF_ASSUME(m_frame_block->m_kind == regular); // Root tasks are never stolen.
      m_frame_block->m_steal += 1;
      m_frame_block->get_self().resume();
    }

    // TODO: make private
    // private:
    frame_block *m_frame_block = nullptr;
  };

private:
  [[nodiscard]] constexpr auto offset(stdx::coroutine_handle<> self) noexcept -> std::int16_t {
    LF_ASSUME(m_kind != sentinel);
    std::intptr_t offset = as_integer(self.address()) - as_integer(this);
    LF_ASSUME(k_i16_min <= offset && offset <= k_i16_max);
    LF_ASSUME(m_offset < 0 || sizeof(frame_block) <= m_offset);
    return static_cast<std::int16_t>(offset);
  }

public:
  /**
   * @brief For root blocks.
   */
  explicit constexpr frame_block(stdx::coroutine_handle<> self) noexcept
      : m_kind{rooted},
        m_offset(offset(self)) {}

  /**
   * @brief For non-root blocks -- need to call `set_self` after construction.
   */
  explicit constexpr frame_block(frame_block *parent)
      : m_kind{regular},
        m_parent{parent} {}

  /**
   * @brief For sentinel blocks.
   *
   * The `tag` parameter is unused, but is required to disambiguate the constructor without introducing a
   * default constructor that maybe called by accident.
   */
  explicit constexpr frame_block([[maybe_unused]] empty tag) noexcept {};

  /**
   * @brief Set the offset to self.
   *
   * Only regular blocks should call this as rooted blocks can set it at construction.
   */
  constexpr void set_self(stdx::coroutine_handle<> self) noexcept {
    LF_ASSUME(m_kind == regular);
    m_offset = offset(self);
  }

  /**
   * @brief Get a handle to the adjacent/paired coroutine frame.
   */
  [[nodiscard]] auto get_self() const noexcept -> stdx::coroutine_handle<> {
    LF_ASSUME(m_kind != sentinel);
    LF_ASSUME(m_offset < 0 || sizeof(frame_block) <= m_offset);
    return stdx::coroutine_handle<>::from_address(std::bit_cast<void *>(byte_cast(this) + m_offset));
  }

  [[nodiscard]] auto is_root() const noexcept -> bool { return m_kind == rooted; }

private:
  enum : std::int16_t { rooted, sentinel, regular };

  /**
   * Rooted needs:   m_kind, m_join, m_steal, m_offset
   * Sentinel needs: m_kind, m_parent
   * Regular needs:  m_kind, m_join, m_steal, m_offset, m_parent
   */

  std::atomic_uint16_t m_join = k_u16_max; ///< Number of children joined (with offset).
  std::uint16_t m_steal = 0;               ///< Number of steals.
  std::int16_t m_kind = sentinel;          ///< Kind of frame.
  std::int16_t m_offset = 0;               ///< Offset from `this` to paired coroutine's void handle.
  frame_block *m_parent = nullptr;         ///< Parent task or sentinel frame (roots don't have one).
};

static_assert(alignof(frame_block) <= k_new_align);
static_assert(sizeof(frame_block) % k_new_align == 0);
static_assert(std::is_trivially_destructible_v<frame_block>);

} // namespace LF_DEPENDANT_ABI

// ----------------------------------------------- //

/**
 * @brief A fraction of a thread's cactus stack.
 */
class alignas(k_new_align) async_stack : immovable<async_stack>, std::array<std::byte, k_mebibyte> {
public:
  /**
   * @brief Construct a new `async_stack`.
   */
  async_stack() noexcept {
    // Initialize the sentinel.
    new (data() + size() - sizeof(frame_block)) frame_block{lf::detail::empty{}};
  }

  /**
   * @brief The size of an `async_stack` (number of bytes it can store).
   */
  static constexpr auto size() noexcept -> std::size_t { return k_mebibyte; }

  /**
   * @brief Get a pointer to the sentinel `frame_block` on the stack.
   */
  auto sentinel() noexcept -> frame_block * {
    return std::launder(std::bit_cast<frame_block *>(data() + size() - sizeof(frame_block)));
  }

  /**
   * @brief Convert a pointer to a stacks sentinel `frame_block` to a pointer to the stack.
   */
  static auto unsafe_from_sentinel(frame_block *sentinel) noexcept -> async_stack * {
    return std::bit_cast<async_stack *>(std::launder(byte_cast(sentinel) + sizeof(async_stack) - size()));
  }
};

static_assert(std::is_standard_layout_v<async_stack>);
static_assert(sizeof(async_stack) == async_stack::size(), "Spurious padding in async_stack!");
static_assert(async_stack::size() >= k_kibibyte, "Stack is too small!");

// ----------------------------------------------- //

enum class where {
  stack,
  heap,
};

// ----------------------------------------------- //

inline namespace LF_DEPENDANT_ABI {

/**
 * @brief A base class for promises that allocates on an `async_stack`
 */
template <where W>
class promise_alloc : std::conditional<W == where::stack, empty, frame_block> {

  // Convert an alignment to a std::uintptr_t, ensure its is a power of two and >= k_new_align.
  constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
    auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
    LF_ASSUME(std::has_single_bit(align));
    return std::max(align, k_new_align);
  }

protected:
  constexpr promise_alloc(std::coroutine_handle<> self) noexcept
    requires(W == where::stack)
      : frame_block{self} {}

  constexpr promise_alloc(std::coroutine_handle<> self) noexcept
    requires(W == where::stack)
      : frame_block{self} {}

public:
  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) noexcept -> void * {
    return promise_alloc::operator new(size, std::align_val_t{k_new_align});
  }

  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
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

} // namespace LF_DEPENDANT_ABI

} // namespace detail

/**
 * @brief An aliases to the internal type that controls a task.
 *
 * Pointers given/received to this type are all non-owning, the only method
 * implementors of a context are permitted to use is `resume()`.
 */
using task_ptr = detail::frame_block::handle;

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
