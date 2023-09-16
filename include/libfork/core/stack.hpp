#ifndef C5C3AA77_D533_4A89_8D33_99BD819C1B4C
#define C5C3AA77_D533_4A89_8D33_99BD819C1B4C

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
#include <type_traits>
#include <utility>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/core/coroutine.hpp"

/**
 * @file stack.hpp
 *
 * @brief Provides an async cactus-stack, control blocks and memory management.
 */

namespace lf {

// -------------------- async stack -------------------- //

inline namespace ext {

class async_stack;

} // namespace ext

namespace impl {

static constexpr std::size_t k_stack_size = k_kibibyte * LF_ASYNC_STACK_SIZE;

/**
 * @brief Get a pointer to the end of a stack's buffer.
 */
auto stack_as_bytes(async_stack *stack) noexcept -> std::byte *;
/**
 * @brief Convert a pointer to a stack's sentinel `frame_block` to a pointer to the stack.
 */
auto bytes_to_stack(std::byte *bytes) noexcept -> async_stack *;

} // namespace impl

inline namespace ext {

/**
 * @brief A fraction of a thread's cactus stack.
 */
class async_stack : impl::immovable<async_stack> {
  alignas(impl::k_new_align) std::byte m_buf[impl::k_stack_size]; // NOLINT

  friend auto impl::stack_as_bytes(async_stack *stack) noexcept -> std::byte *;

  friend auto impl::bytes_to_stack(std::byte *bytes) noexcept -> async_stack *;
};

static_assert(std::is_standard_layout_v<async_stack>);
static_assert(sizeof(async_stack) == impl::k_stack_size, "Spurious padding in async_stack!");

} // namespace ext

namespace impl {

inline auto stack_as_bytes(async_stack *stack) noexcept -> std::byte * { return std::launder(stack->m_buf + k_stack_size); }

inline auto bytes_to_stack(std::byte *bytes) noexcept -> async_stack * {
#ifdef __cpp_lib_is_pointer_interconvertible
  static_assert(std::is_pointer_interconvertible_with_class(&async_stack::m_buf));
#endif
  return std::launder(std::bit_cast<async_stack *>(bytes - k_stack_size));
}

} // namespace impl

// ---------------------------------------- //

inline namespace ext {

struct frame_block;

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::async_stack``s and a LIFO stack of
 * tasks. The stack of ``lf::async_stack``s is expected to never be empty, it
 * should always be able to return an empty ``lf::async_stack``.
 */
template <typename Context>
concept thread_context = requires(Context ctx, async_stack *stack, frame_block *ext, frame_block *task) {
  { ctx.max_threads() } -> std::same_as<std::size_t>;        // The maximum number of threads.
  { ctx.submit(ext) };                                       // Submit an external task to the context.
  { ctx.task_pop() } -> std::convertible_to<frame_block *>;  // If the stack is empty, return a null pointer.
  { ctx.task_push(task) };                                   // Push a non-null pointer.
  { ctx.stack_pop() } -> std::convertible_to<async_stack *>; // Return a non-null pointer
  { ctx.stack_push(stack) };                                 // Push a non-null pointer
};

} // namespace ext

// ----------------------------------------------- //

/**
 * @brief This namespace contains `inline thread_local constinit` variables and functions to manipulate them.
 */
namespace impl::tls {

template <thread_context Context>
constinit inline thread_local Context *ctx = nullptr;

constinit inline thread_local std::byte *asp = nullptr;

} // namespace impl::tls

// ----------------------------------------------- //

namespace impl {

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

} // namespace impl

// ----------------------------------------------- //

inline namespace ext {

/**
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */
struct frame_block : private impl::immovable<frame_block>, impl::debug_block {
  /**
   * @brief Resume a stolen task.
   *
   * When this function returns this worker will have run out of tasks.
   */
  void resume_stolen() noexcept {
    LF_LOG("Call to resume on stolen task");
    LF_ASSERT(impl::tls::asp);
    m_steal += 1;
    coro().resume();
  }
  /**
   * @brief Resume an external task.
   *
   * When this function returns this worker will have run out of tasks
   * and their asp will be pointing at a sentinel.
   */
  template <thread_context Context>
  inline void resume_external() noexcept;

// protected:
/**
 * @brief Construct a frame block.
 *
 * Pass ``top == nullptr`` if this is on the heap. Non-root tasks will need to call ``set_parent(...)``.
 */
#ifndef LF_COROUTINE_OFFSET
  constexpr frame_block(stdx::coroutine_handle<> coro, std::byte *top) : m_coro{coro}, m_top(top) { LF_ASSERT(coro); }
#else
  constexpr frame_block(stdx::coroutine_handle<>, std::byte *top) : m_top(top) {}
#endif

  /**
   * @brief Set the pointer to the parent frame.
   */
  void set_parent(frame_block *parent) noexcept {
    LF_ASSERT(!m_parent);
    m_parent = impl::non_null(parent);
  }

  /**
   * @brief Get a pointer to the parent frame.
   */
  [[nodiscard]] auto parent() const noexcept -> frame_block * { return impl::non_null(m_parent); }

  /**
   * @brief Get a pointer to the top of the top of the async-stack this frame was allocated on.
   */
  [[nodiscard]] auto top() const noexcept -> std::byte * { return impl::non_null(m_top); }

  /**
   * @brief Get the coroutine handle for this frames coroutine.
   */
  [[nodiscard]] auto coro() noexcept -> stdx::coroutine_handle<> {
#ifndef LF_COROUTINE_OFFSET
    return m_coro;
#else
    return stdx::coroutine_handle<>::from_address(byte_cast(this) - LF_COROUTINE_OFFSET);
#endif
  }

  /**
   * @brief Perform a `.load(order)` on the atomic join counter.
   */
  [[nodiscard]] constexpr auto load_joins(std::memory_order order) const noexcept -> std::uint32_t {
    return m_join.load(order);
  }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  constexpr auto fetch_sub_joins(std::uint32_t val, std::memory_order order) noexcept -> std::uint32_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] constexpr auto steals() const noexcept -> std::uint32_t { return m_steal; }

  /**
   * @brief Check if this is a root frame.
   */
  [[nodiscard]] constexpr auto is_root() const noexcept -> bool { return m_parent == nullptr; }

  /**
   * @brief Reset the join and steal counters, must be outside a fork-join region.
   */
  void reset() noexcept {

    LF_ASSERT(m_steal != 0); // Reset not needed if steal is zero.

    m_steal = 0;

    static_assert(std::is_trivially_destructible_v<decltype(m_join)>);
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    std::construct_at(&m_join, impl::k_u32_max);
  }

 private:
#ifndef LF_COROUTINE_OFFSET
  stdx::coroutine_handle<> m_coro;
#endif

  std::byte *m_top;                              ///< Needs to be separate in-case allocation elided.
  frame_block *m_parent = nullptr;               ///< Same ^
  std::atomic_uint32_t m_join = impl::k_u32_max; ///< Number of children joined (with offset).
  std::uint32_t m_steal = 0;                     ///< Number of steals.
};

static_assert(alignof(frame_block) <= impl::k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

} // namespace ext

// ----------------------------------------------- //

namespace impl::tls {

/**
 * @brief Set `tls::asp` to point at `frame`.
 *
 * It must currently be pointing at a sentinel.
 */
template <thread_context Context>
inline void eat(std::byte *top) {
  LF_LOG("Thread eats a stack");
  LF_ASSERT(tls::asp);
  std::byte *prev = std::exchange(tls::asp, top);
  LF_ASSERT(prev != top);
  async_stack *stack = bytes_to_stack(prev);
  ctx<Context>->stack_push(stack);
}

} // namespace impl::tls

// ----------------------------------------------- //

inline namespace ext {

template <thread_context Context>
inline void frame_block::resume_external() noexcept {

  LF_LOG("Call to resume on external task");

  LF_ASSERT(impl::tls::asp);

  if (!is_root()) {
    impl::tls::eat<Context>(top());
  } else {
    LF_LOG("External was root");
  }

  coro().resume();

  LF_ASSERT(impl::tls::asp);
}

} // namespace ext

namespace impl {

// ----------------------------------------------- //

/**
 * @brief A base class for promises that allocates on the heap.
 */
struct promise_alloc_heap : frame_block {
 protected:
  explicit promise_alloc_heap(stdx::coroutine_handle<> self) noexcept : frame_block{self, nullptr} {}
};

// ----------------------------------------------- //

/**
 * @brief A base class for promises that allocates on an `async_stack`.
 *
 * When a promise deriving from this class is constructed 'tls::asp' will be set and when it is destroyed 'tls::asp'
 * will be returned to the previous frame.
 */
struct promise_alloc_stack : frame_block {

  // Convert an alignment to a std::uintptr_t, ensure its is a power of two and >= k_new_align.
  // constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
  //   auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
  //   LF_ASSERT(std::has_single_bit(align));
  //   LF_ASSERT(align > 0);
  //   return std::max(align, impl::k_new_align);
  // }

 protected:
  explicit promise_alloc_stack(stdx::coroutine_handle<> self) noexcept : frame_block{self, tls::asp} {}

 public:
  /**
   * @brief Allocate the coroutine on the current `async_stack`.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) -> void * {
    LF_ASSERT(tls::asp);
    tls::asp -= (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Allocating {} bytes on stack from {}", size, (void *)tls::asp);
    return tls::asp;
  }

  /**
   * @brief Deallocate the coroutine on the current `async_stack`.
   */
  static void operator delete(void *ptr, std::size_t size) {
    LF_ASSERT(ptr == tls::asp);
    tls::asp += (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Deallocating {} bytes on stack to {}", size, (void *)tls::asp);
  }
};

} // namespace impl

inline namespace ext {

template <thread_context Context>
void worker_init(Context *context) {

  LF_ASSERT(context);
  LF_ASSERT(!impl::tls::ctx<Context>);
  LF_ASSERT(!impl::tls::asp);

  impl::tls::ctx<Context> = context;
  impl::tls::asp = impl::stack_as_bytes(context->stack_pop());
}

template <thread_context Context>
void worker_finalize(Context *context) {
  LF_ASSERT(context == impl::tls::ctx<Context>);
  LF_ASSERT(impl::tls::asp);

  context->stack_push(impl::bytes_to_stack(impl::tls::asp));

  impl::tls::asp = nullptr;
  impl::tls::ctx<Context> = nullptr;
}

} // namespace ext

} // namespace lf

#endif /* C5C3AA77_D533_4A89_8D33_99BD819C1B4C */
