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

#include "libfork/core/coroutine.hpp"
#include "libfork/core/macro.hpp"
#include "libfork/core/utility.hpp"

/**
 * @file stack.hpp
 *
 * @brief Provides the core elements of the async cactus-stack an thread-local memory.
 *
 * This is almost entirely an implementation detail, the `frame_block *` interface is unsafe.
 */

namespace lf {

// -------------------- async stack -------------------- //

inline namespace ext {

class async_stack;

} // namespace ext

namespace impl {

static constexpr std::size_t k_stack_size = 4 * k_kibibyte * LF_ASYNC_STACK_SIZE;

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
 *
 * These can be alloacted on the heap just like any other object when a `thread_context`
 * needs them.
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

// ----------------------------------------------- //

/**
 * @brief This namespace contains `inline thread_local constinit` variables and functions to manipulate them.
 */
namespace tls {

template <typename Context>
constinit inline thread_local Context *ctx = nullptr;

constinit inline thread_local std::byte *asp = nullptr;

LF_TLS_CLANG_INLINE inline auto get_asp() noexcept -> std::byte * { return asp; }

LF_TLS_CLANG_INLINE inline void set_asp(std::byte *new_asp) noexcept { asp = new_asp; }

template <typename Context>
LF_TLS_CLANG_INLINE inline auto get_ctx() noexcept -> Context * {
  return ctx<Context>;
}

/**
 * @brief Set `tls::asp` to point at `frame`.
 */
template <typename Context>
LF_TLS_CLANG_INLINE inline void push_asp(std::byte *top) {
  LF_LOG("Thread eats a stack");
  LF_ASSERT(tls::asp);
  std::byte *prev = std::exchange(tls::asp, top);
  LF_ASSERT(prev != top);
  async_stack *stack = bytes_to_stack(prev);
  tls::ctx<Context>->stack_push(stack);
}

} // namespace tls

// ----------------------------------------------- //

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

/**
 * @brief A small bookkeeping struct which is a member of each task's promise.
 */
struct frame_block : private immovable<frame_block>, debug_block {
  /**
   * @brief Resume a stolen task.
   *
   * When this function returns this worker will have run out of tasks.
   */
  void resume_stolen() noexcept {
    LF_LOG("Call to resume on stolen task");
    LF_ASSERT(tls::get_asp());
    m_steal += 1;
    coro().resume();
  }
  /**
   * @brief Resume an external task.
   *
   * When this function returns this worker will have run out of tasks
   * and their asp will be pointing at a sentinel.
   */
  template <typename Context>
  inline void resume_external() noexcept {

    LF_LOG("Call to resume on external task");

    LF_ASSERT(tls::get_asp());

    if (!is_root()) {
      tls::push_asp<Context>(top());
    } else {
      LF_LOG("External was root");
    }

    coro().resume();

    LF_ASSERT(tls::get_asp());
    LF_ASSERT(!non_null(tls::get_ctx<Context>())->task_pop());
  }

// protected:
/**
 * @brief Construct a frame block.
 *
 * Pass ``top == nullptr`` if this is on the heap. Non-root tasks will need to call ``set_parent(...)``.
 */
#ifndef LF_COROUTINE_OFFSET
  frame_block(stdx::coroutine_handle<> coro, std::byte *top) : m_coro{coro}, m_top(top) { LF_ASSERT(coro); }
#else
  frame_block(stdx::coroutine_handle<>, std::byte *top) : m_top(top) {}
#endif

  /**
   * @brief Set the pointer to the parent frame.
   */
  void set_parent(frame_block *parent) noexcept {
    LF_ASSERT(!m_parent);
    m_parent = non_null(parent);
  }

  /**
   * @brief Get a pointer to the parent frame.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto parent() const noexcept -> frame_block * {
    LF_ASSERT(!is_root());
    return non_null(m_parent);
  }

  /**
   * @brief Get a pointer to the top of the top of the async-stack this frame was allocated on.
   *
   * Only valid if this is not a root frame.
   */
  [[nodiscard]] auto top() const noexcept -> std::byte * {
    LF_ASSERT(!is_root());
    return non_null(m_top);
  }

  /**
   * @brief Small return type suitable for structured binding.
   */
  struct local_t {
    bool is_root;
    std::byte *top;
  };

  /**
   * @brief Like `is_root()` and `top()` but valid for root frames.
   *
   * Note that if this is a root frame then the pointer to the top of the async-stack has an undefined value.
   */
  [[nodiscard]] auto locale() const noexcept -> local_t { return {is_root(), m_top}; }

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
  [[nodiscard]] auto load_joins(std::memory_order order) const noexcept -> std::uint32_t { return m_join.load(order); }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  auto fetch_sub_joins(std::uint32_t val, std::memory_order order) noexcept -> std::uint32_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] auto steals() const noexcept -> std::uint32_t { return m_steal; }

  /**
   * @brief Check if this is a root frame.
   */
  [[nodiscard]] auto is_root() const noexcept -> bool { return m_parent == nullptr; }

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
    std::construct_at(&m_join, k_u32_max);
  }

 private:
#ifndef LF_COROUTINE_OFFSET
  stdx::coroutine_handle<> m_coro;
#endif

  std::byte *m_top;                        ///< Needs to be separate in-case allocation elided.
  frame_block *m_parent = nullptr;         ///< Same ^
  std::atomic_uint32_t m_join = k_u32_max; ///< Number of children joined (with offset).
  std::uint32_t m_steal = 0;               ///< Number of steals.
};

static_assert(alignof(frame_block) <= k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

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
  //  static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
  //   auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
  //   LF_ASSERT(std::has_single_bit(align));
  //   LF_ASSERT(align > 0);
  //   return std::max(align, impl::k_new_align);
  // }

 protected:
  explicit promise_alloc_stack(stdx::coroutine_handle<> self) noexcept : frame_block{self, tls::get_asp()} {}

 public:
  /**
   * @brief Allocate the coroutine on the current `async_stack`.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] LF_TLS_CLANG_INLINE static auto operator new(std::size_t size) -> void * {
    LF_ASSERT(tls::asp);
    tls::asp -= (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Allocating {} bytes on stack from {}", size, (void *)tls::asp);
    return tls::asp;
  }

  /**
   * @brief Deallocate the coroutine on the current `async_stack`.
   */
  LF_TLS_CLANG_INLINE static void operator delete(void *ptr, std::size_t size) {
    LF_ASSERT(ptr == tls::asp);
    tls::asp += (size + impl::k_new_align - 1) & ~(impl::k_new_align - 1);
    LF_LOG("Deallocating {} bytes on stack to {}", size, (void *)tls::asp);
  }
};

} // namespace impl

inline namespace ext {

/**
 * @brief A type safe wrapper around a handle to a stealable coroutine.
 *
 * Instances of this type will be passed to a worker's `thread_context`.
 *
 * \rst
 *
 * .. warning::
 *
 *    A pointer to an ``task_h`` must never be dereferenced, only ever passed to ``resume()``.
 *
 * \endrst
 */
template <typename Context>
struct task_h {
  /**
   * @brief Only a worker who has called `worker_init(Context *)` can resume this task.
   */
  friend void resume(task_h *ptr) noexcept {
    LF_ASSERT(impl::tls::get_ctx<Context>());
    LF_ASSERT(impl::tls::get_asp());
    std::bit_cast<impl::frame_block *>(ptr)->resume_stolen();
  }
};

/**
 * @brief A type safe wrapper around a handle to a coroutine that is at a submission point.
 *
 * Instances of this type (wrapped in an `lf::intrusive_list`s node) will be passed to a worker's `thread_context`.
 *
 * \rst
 *
 * .. warning::
 *
 *    A pointer to an ``submit_h`` must never be dereferenced, only ever passed to ``resume()``.
 *
 * \endrst
 */
template <typename Context>
struct submit_h {
  /**
   * @brief Only a worker who has called `worker_init(Context *)` can resume this task.
   */
  friend void resume(submit_h *ptr) noexcept {
    LF_ASSERT(impl::tls::get_ctx<Context>());
    LF_ASSERT(impl::tls::get_asp());
    std::bit_cast<impl::frame_block *>(ptr)->template resume_external<Context>();
  }
};

// --------------------------------------------------------- //

/**
 * @brief Initialize thread-local variables before a worker can resume submitted tasks.
 *
 * \rst
 *
 * .. warning::
 *    These should be cleaned up with ``worker_finalize(...)``.
 *
 * \endrst
 */
template <typename Context>
LF_NOINLINE void worker_init(Context *context) noexcept {

  LF_LOG("Initializing worker");

  LF_ASSERT(context);

  impl::tls::ctx<Context> = context;
  impl::tls::asp = impl::stack_as_bytes(context->stack_pop());
}

/**
 * @brief Clean-up thread-local variable before destructing a worker's context.
 *
 * \rst
 *
 * .. warning::
 *    These must be initialized with ``worker_init(...)``.
 *
 * \endrst
 */
template <typename Context>
LF_NOINLINE void worker_finalize(Context *context) {

  LF_LOG("Finalizing worker");

  LF_ASSERT(context == impl::tls::ctx<Context>);
  LF_ASSERT(impl::tls::asp);

  context->stack_push(impl::bytes_to_stack(impl::tls::asp));
}

// ------------------------------------------------- //

} // namespace ext

} // namespace lf

#endif /* C5C3AA77_D533_4A89_8D33_99BD819C1B4C */
