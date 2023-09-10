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

// ----------------------------------------------- //

inline namespace LF_DEPENDENT_ABI {

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
  std::int32_t m_count = 0; ///< Number of forks/calls (debug).
#endif
};

static_assert(std::is_trivially_destructible_v<debug_block>);

// ----------------------------------------------- //

struct frame_block; ///< Forward declaration, impl below.

} // namespace LF_DEPENDENT_ABI

/**
 * @brief The async stack pointer.
 *
 * \rst
 *
 * Each thread has a pointer into its current async stack, the end of the stack is marked with a sentinel
 * `frame_block`. It is assumed that the runtime initializes this to a sentinel-guarded `async_stack`. A
 * stack looks a little bit like this:
 *
 * .. code::
 *
 *    |--------------- async_stack object ----------------|
 *
 *                 R[coro_frame].....R[coro_frame].......SP
 *                 |---------------->|------------------> |--> Pointer to parent (some other stack)
 *
 * Key:
 * - `R` = regular `frame_block`.
 * - `S` = sentinel `frame_block`.
 * - `.` = padding.
 *
 * \endrst
 */
constinit inline thread_local frame_block *asp = nullptr;

// ----------------------------------------------- //

inline namespace LF_DEPENDENT_ABI {

/**
 * @brief A small bookkeeping struct allocated immediately before/after each coroutine frame.
 */
struct frame_block : immovable<frame_block>, debug_block {
  /**
   * @brief A lightweight nullable handle to a `frame_block` that contains the public API for thieves.
   */
  struct thief_handle {
    /**
     * @brief Resume a stolen task.
     */
    void resume() const noexcept {
      LF_LOG("Call to resume on stolen task");

      // Link the sentinel to the parent.
      LF_ASSERT(asp);
      asp->write_sentinel_parent(m_frame_block);

      LF_ASSERT(m_frame_block);
      LF_ASSERT(m_frame_block->is_regular()); // Only regular tasks should be stolen.
      m_frame_block->m_steal += 1;
      m_frame_block->get_coro().resume();
    }

    explicit operator bool() const noexcept { return m_frame_block != nullptr; }

    // TODO: make private
    // private:
    frame_block *m_frame_block = nullptr;
  };

  /**
   * @brief For root blocks.
   */
  explicit constexpr frame_block(stdx::coroutine_handle<> coro) noexcept : m_prev{0}, m_coro(offset(coro.address())) {}

  /**
   * @brief For regular blocks -- need to call `set_coro` after construction.
   */
  explicit constexpr frame_block(frame_block *parent) : m_prev{offset(parent)}, m_coro{uninitialized} {}

  /**
   * @brief For sentinel blocks.
   *
   * The `tag` parameter is unused, but is required to disambiguate the constructor without introducing a
   * default constructor that maybe called by accident.
   */
  explicit constexpr frame_block([[maybe_unused]] empty tag) noexcept : m_prev{0}, m_coro{0} {};

  /**
   * @brief Set the offset to coro.
   *
   * Only regular blocks should call this as rooted blocks can set it at construction.
   */
  constexpr void set_coro(stdx::coroutine_handle<> coro) noexcept {
    LF_ASSERT(m_coro == uninitialized && m_prev != 0 && m_prev != uninitialized);
    m_coro = offset(coro.address());
  }

  /**
   * @brief Get a handle to the adjacent/paired coroutine frame.
   */
  [[nodiscard]] auto get_coro() const noexcept -> stdx::coroutine_handle<> {
    LF_ASSERT(!is_sentinel());
    return stdx::coroutine_handle<>::from_address(from_offset(m_coro));
  }

  /**
   * @brief The result of a `frame_block::destroy` call, suitable for structured binding.
   */
  struct parent_t {
    frame_block *parent; ///< The parent of the destroyed coroutine on top of the stack.
    bool was_last;       ///< `true` if the destroyed coroutine was the last on the stack.
  };

  /**
   * @brief Destroy the coroutine on the top of this threads async stack, sets asp.
   */
  static auto pop_asp() -> parent_t {

    frame_block *top = asp;
    LF_ASSERT(top);

    // Destroy the coroutine (this does not effect top)
    LF_ASSERT(top->is_regular());
    top->get_coro().destroy();

    asp = std::bit_cast<frame_block *>(top->from_offset(top->m_prev));

    LF_ASSERT(is_aligned(asp, alignof(frame_block)));

    if (asp->is_sentinel()) [[unlikely]] {
      return {asp->read_sentinel_parent(), true};
    }
    return {asp, false};
  }

  [[nodiscard]] constexpr auto joins() -> std::atomic_uint16_t & { return m_join; }

private:
  static constexpr std::int16_t uninitialized = 1;

  /**
   * m_prev and m_coro are both offsets to external values hence values 0..sizeof(frame_block) are invalid.
   * Hence, we use 1 to mark uninitialized values.
   *
   * Rooted has:   m_prev == 0 and m_coro != 0
   * Sentinel has: m_prev == 0 and m_coro == 0,
   * Regular has:  m_prev != 0 and m_coro != 0
   */

  std::atomic_uint16_t m_join = k_u16_max; ///< Number of children joined (with offset).
  std::uint16_t m_steal = 0;               ///< Number of steals.
  std::int16_t m_prev;                     ///< Distance to previous frame.
  std::int16_t m_coro;                     ///< Offset from `this` to paired coroutine's void handle.

  constexpr auto is_initialised() const noexcept -> bool {
    static_assert(sizeof(frame_block) > uninitialized, "Required for uninitialized to be invalid offset");
    return m_prev != uninitialized && m_coro != uninitialized;
  }

  auto is_sentinel() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_coro == 0;
  }

  auto is_regular() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_prev != 0;
  }

  static constexpr auto is_aligned(void *address, std::size_t align) noexcept -> bool {
    return std::bit_cast<std::uintptr_t>(address) % align == 0;
  }

  /**
   * @brief Compute the offset from `this` to `external`.
   *
   * Satisfies: `external == from_offset(offset(external))`
   */
  [[nodiscard]] constexpr auto offset(void *external) const noexcept -> std::int16_t {
    LF_ASSERT(external);

    std::ptrdiff_t offset = byte_cast(external) - byte_cast(this);

    LF_ASSERT(k_i16_min <= offset && offset <= k_i16_max);  // Check fits in i16.
    LF_ASSERT(offset < 0 || sizeof(frame_block) <= offset); // Check is not internal.

    return static_cast<std::int16_t>(offset);
  }

  /**
   * @brief Compute an external pointer a distance offset from `this`
   *
   * Satisfies: `external == from_offset(offset(external))`
   */
  [[nodiscard]] constexpr auto from_offset(std::int16_t offset) const noexcept -> void * {
    // Check offset is not internal.
    LF_ASSERT(offset < 0 || sizeof(frame_block) <= offset);

    return const_cast<std::byte *>(byte_cast(this) + offset); // Const cast is safe as pointer is external.
  }

  /**
   * @brief Write the parent below a sentinel frame.
   */
  void write_sentinel_parent(frame_block *parent) noexcept {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    new (address) frame_block *{parent};
  }

  /**
   * @brief Read the parent below a sentinel frame.
   */
  auto read_sentinel_parent() noexcept -> frame_block * {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    // Strict aliasing ok as from_offset(...) guarantees external.
    return *std::bit_cast<frame_block **>(address);
  }
};

static_assert(alignof(frame_block) <= k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

} // namespace LF_DEPENDENT_ABI

// ----------------------------------------------- //

/**
 * @brief A fraction of a thread's cactus stack.
 */
class async_stack : immovable<async_stack> {
public:
  /**
   * @brief Construct a new `async_stack`.
   */
  async_stack() noexcept {
    // Initialize the sentinel with space for a pointer behind it.
    new (static_cast<void *>(m_buf + sentinel_offset)) frame_block{lf::detail::empty{}};
  }

  /**
   * @brief Get a pointer to the sentinel `frame_block` on the stack.
   */
  auto sentinel() noexcept -> frame_block * { return std::launder(std::bit_cast<frame_block *>(m_buf + sentinel_offset)); }

  /**
   * @brief Convert a pointer to a stack's sentinel `frame_block` to a pointer to the stack.
   */
  static auto unsafe_from_sentinel(frame_block *sentinel) noexcept -> async_stack * {
#ifdef __cpp_lib_is_pointer_interconvertible
    static_assert(std::is_pointer_interconvertible_with_class(&async_stack::m_buf));
#endif
    return std::bit_cast<async_stack *>(std::launder(byte_cast(sentinel) - sentinel_offset));
  }

private:
  static constexpr std::size_t k_size = k_kibibyte * LF_ASYNC_STACK_SIZE;
  static constexpr std::size_t sentinel_offset = k_size - sizeof(frame_block) - sizeof(frame_block *);

  alignas(k_new_align) std::byte m_buf[k_size]; // NOLINT

  static_assert(alignof(frame_block) <= alignof(frame_block *), "As we will allocate sentinel above pointer");
};

static_assert(std::is_standard_layout_v<async_stack>);

static_assert(sizeof(async_stack) == k_kibibyte * LF_ASYNC_STACK_SIZE, "Spurious padding in async_stack!");

// ----------------------------------------------- //

inline namespace LF_DEPENDENT_ABI {

class promise_alloc_heap : frame_block {
protected:
  explicit promise_alloc_heap(std::coroutine_handle<> self) noexcept : frame_block{self} { asp = this; }
};

} // namespace LF_DEPENDENT_ABI

/**
 * @brief A base class for promises that allocates on an `async_stack`.
 *
 * When a promise deriving from this class is constructed 'asp' will be set and when it is destroyed 'asp'
 * will be returned to the previous frame.
 */
class promise_alloc_stack {

  // Convert an alignment to a std::uintptr_t, ensure its is a power of two and >= k_new_align.
  constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
    auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
    LF_ASSERT(std::has_single_bit(align));
    LF_ASSERT(align > 0);
    return std::max(align, k_new_align);
  }

protected:
  explicit promise_alloc_stack(std::coroutine_handle<> self) noexcept {
    LF_ASSERT(asp);
    asp->set_coro(self);
  }

public:
  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) noexcept -> void * {
    return promise_alloc_stack::operator new(size, std::align_val_t{k_new_align});
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

    LF_ASSERT(prev_asp);

    auto prev_stack_addr = std::bit_cast<std::uintptr_t>(prev_asp);

    std::uintptr_t coro_frame_addr = (prev_stack_addr - size) & ~(align - 1);

    LF_ASSERT(coro_frame_addr % align == 0);

    std::uintptr_t frame_addr = coro_frame_addr - sizeof(frame_block);

    LF_ASSERT(frame_addr % alignof(frame_block) == 0);

    // Starts the lifetime of the new frame block.
    asp = new (std::bit_cast<void *>(frame_addr)) frame_block{prev_asp};

    return std::bit_cast<void *>(coro_frame_addr);
  }

  /**
   * @brief A noop -- use the destroy method!
   */
  static void operator delete(void *) noexcept {}

  /**
   * @brief A noop -- use the destroy method!
   */
  static void operator delete(void *, std::size_t, std::align_val_t) noexcept {}
};

} // namespace detail

/**
 * @brief An aliases to the internal type that controls a task.
 *
 * Pointers given/received to this type are all non-owning, the only method
 * implementors of a context are permitted to use is `resume()`.
 */
using task_ptr = detail::frame_block::thief_handle;

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
