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

// -------------------- Forward decls -------------------- //

class async_stack;

class frame_block;

// ----------------------------------------------- //

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

// ----------------------------------------------- //

/**
 * @brief This namespace contains `inline thread_local constinit` variables and functions to manipulate them.
 */
namespace tls {

template <thread_context Context>
constinit inline thread_local Context *ctx = nullptr;

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
 * @brief A small bookkeeping struct allocated immediately before/after each coroutine frame.
 */
class frame_block : detail::immovable<frame_block>, public debug_block {
public:
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
  explicit constexpr frame_block([[maybe_unused]] detail::empty tag) noexcept : m_prev{0}, m_coro{0} {};

  /**
   * @brief Resume a stolen task.
   *
   * When this function returns this worker will have run out of tasks
   * and their `tls::asp` will be pointing at a sentinel.
   */
  void resume_stolen() noexcept;

  /**
   * @brief Resume an external task.
   *
   * When this function returns this worker will have run out of tasks
   * and their asp will be pointing at a sentinel.
   */
  template <thread_context Context>
  void resume_external() noexcept;

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
    bool parent_on_asp;  ///< `true` if the parent is on the same stack as task that was just popped.
  };

  [[nodiscard]] auto parent() const noexcept -> parent_t {

    LF_ASSERT(!is_sentinel());

    auto *parent = std::bit_cast<frame_block *>(from_offset(m_prev));

    if (parent->is_sentinel()) [[unlikely]] {
      return {parent->read_sentinel_parent(), false};
    }

    return {parent, true};
  }

  /**
   * @brief Destroy the coroutine on the top of this threads async stack, sets `tls::asp`.
   */
  static auto pop_asp() -> parent_t {

    frame_block *top = tls::asp;
    LF_ASSERT(top);

    // Destroy the coroutine (this does not effect top)
    LF_ASSERT(top->is_regular());
    top->get_coro().destroy();

    tls::asp = std::bit_cast<frame_block *>(top->from_offset(top->m_prev));

    LF_ASSERT(is_aligned(tls::asp, alignof(frame_block)));

    if (tls::asp->is_sentinel()) [[unlikely]] {
      return {tls::asp->read_sentinel_parent(), false};
    }
    return {tls::asp, true};
  }

  /**
   * @brief Perform a `.load(order)` on the atomic join counter.
   */
  [[nodiscard]] constexpr auto load_joins(std::memory_order order) const noexcept -> std::uint16_t {
    return m_join.load(order);
  }

  /**
   * @brief Perform a `.fetch_sub(val, order)` on the atomic join counter.
   */
  constexpr auto fetch_sub_joins(std::uint16_t val, std::memory_order order) noexcept -> std::uint16_t {
    return m_join.fetch_sub(val, order);
  }

  /**
   * @brief Get the number of times this frame has been stolen.
   */
  [[nodiscard]] constexpr auto steals() const noexcept -> std::uint16_t { return m_steal; }

  /**
   * @brief Check if a non-sentinel frame is a root frame.
   */
  [[nodiscard]] constexpr auto is_root() const noexcept -> bool {
    LF_ASSERT(!is_sentinel());
    return !is_regular();
  }

  /**
   * @brief Check if any frame is a sentinel frame.
   */
  [[nodiscard]] constexpr auto is_sentinel() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_coro == 0;
  }

  /**
   * @brief Reset the join and steal counters, must be outside a fork-join region.
   */
  void reset() noexcept {
    LF_ASSERT(!is_sentinel());
    LF_ASSERT(m_steal != 0); // Reset not needed if steal is zero.

    m_steal = 0;

    static_assert(std::is_trivially_destructible_v<decltype(m_join)>);
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    std::construct_at(&m_join, detail::k_u16_max);
  }

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

  std::atomic_uint16_t m_join = detail::k_u16_max; ///< Number of children joined (with offset).
  std::uint16_t m_steal = 0;                       ///< Number of steals.
  std::int16_t m_prev;                             ///< Distance to previous frame.
  std::int16_t m_coro;                             ///< Offset from `this` to paired coroutine's void handle.

  [[nodiscard]] constexpr auto is_initialised() const noexcept -> bool {
    static_assert(sizeof(frame_block) > uninitialized, "Required for uninitialized to be invalid offset");
    return m_prev != uninitialized && m_coro != uninitialized;
  }

  [[nodiscard]] constexpr auto is_regular() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_prev != 0;
  }

  [[nodiscard]] static constexpr auto is_aligned(void *address, std::size_t align) noexcept -> bool {
    return std::bit_cast<std::uintptr_t>(address) % align == 0;
  }

  /**
   * @brief Compute the offset from `this` to `external`.
   *
   * Satisfies: `external == from_offset(offset(external))`
   */
  [[nodiscard]] constexpr auto offset(void *external) const noexcept -> std::int16_t {
    LF_ASSERT(external);

    std::ptrdiff_t offset = detail::byte_cast(external) - detail::byte_cast(this);

    LF_ASSERT(detail::k_i16_min <= offset && offset <= detail::k_i16_max); // Check fits in i16.
    LF_ASSERT(offset < 0 || sizeof(frame_block) <= offset);                // Check is not internal.

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
  void write_sentinels_parent(frame_block *parent) noexcept {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    new (address) frame_block *{parent};
  }

  /**
   * @brief Read the parent below a sentinel frame.
   */
  [[nodiscard]] auto read_sentinel_parent() noexcept -> frame_block * {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    // Strict aliasing ok as from_offset(...) guarantees external.
    return *std::bit_cast<frame_block **>(address);
  }
};

static_assert(alignof(frame_block) <= detail::k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

// ----------------------------------------------- //

namespace detail {}

/**
 * @brief A fraction of a thread's cactus stack.
 */
class async_stack : detail::immovable<async_stack> {
public:
  /**
   * @brief Construct a new `async_stack`.
   */
  async_stack() noexcept {
    static_assert(std::is_standard_layout_v<async_stack>);
    static_assert(alignof(frame_block) <= detail::k_new_align);
    static_assert(alignof(frame_block) <= alignof(frame_block *));

    static_assert(sizeof(async_stack) == k_size, "Spurious padding in async_stack!");

    // Initialize the sentinel with space for a pointer behind it.
    new (static_cast<void *>(m_buf + k_sentinel_offset)) frame_block{lf::detail::empty{}};
  }

  /**
   * @brief Get a pointer to the sentinel `frame_block` on the stack.
   */
  auto sentinel() noexcept -> frame_block * { return std::launder(std::bit_cast<frame_block *>(m_buf + k_sentinel_offset)); }

  /**
   * @brief Convert a pointer to a stack's sentinel `frame_block` to a pointer to the stack.
   */
  static auto unsafe_from_sentinel(frame_block *sentinel) noexcept -> async_stack * {
#ifdef __cpp_lib_is_pointer_interconvertible
    static_assert(std::is_pointer_interconvertible_with_class(&async_stack::m_buf));
#endif
    return std::bit_cast<async_stack *>(std::launder(byte_cast(sentinel) - k_sentinel_offset));
  }

private:
  static constexpr std::size_t k_size = detail::k_kibibyte * LF_ASYNC_STACK_SIZE;
  static constexpr std::size_t k_sentinel_offset = k_size - sizeof(frame_block) - sizeof(frame_block *);

  alignas(detail::k_new_align) std::byte m_buf[k_size]; // NOLINT
};

// ----------------------------------------------- //

namespace tls {

/**
 * @brief Set `tls::asp` to point at `frame`.
 *
 * It must currently be pointing at a sentinel.
 */
template <thread_context Context>
inline void eat(frame_block *frame) {
  LF_LOG("Thread eats a stack");
  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());
  frame_block *prev = std::exchange(tls::asp, frame);
  async_stack *stack = async_stack::unsafe_from_sentinel(prev);
  ctx<Context>->stack_push(stack);
}

} // namespace tls

// ----------------------------------------------- //

/**
 * @brief Resume a stolen task.
 *
 * When this function returns this worker will have run out of tasks
 * and their `tls::asp` will be pointing at a sentinel.
 */
inline void frame_block::resume_stolen() noexcept {
  LF_LOG("Call to resume on stolen task");

  // Link the sentinel to the parent.
  LF_ASSERT(tls::asp);
  tls::asp->write_sentinels_parent(this);

  LF_ASSERT(is_regular()); // Only regular tasks should be stolen.
  m_steal += 1;
  get_coro().resume();

  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());
}

// ----------------------------------------------- //

/**
 * @brief Resume an external task.
 *
 * When this function returns this worker will have run out of tasks
 * and their asp will be pointing at a sentinel.
 */
template <thread_context Context>
inline void frame_block::resume_external() noexcept {

  LF_LOG("Call to resume on external task");

  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());

  LF_ASSERT(!is_sentinel()); // Only regular/root tasks are external

  if (!is_root()) {
    tls::eat<Context>(this);
  }

  get_coro().resume();

  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());
}

// ----------------------------------------------- //

class promise_alloc_heap : frame_block {
protected:
  explicit promise_alloc_heap(std::coroutine_handle<> self) noexcept : frame_block{self} { tls::asp = this; }
};

// ----------------------------------------------- //

/**
 * @brief A base class for promises that allocates on an `async_stack`.
 *
 * When a promise deriving from this class is constructed 'tls::asp' will be set and when it is destroyed 'tls::asp'
 * will be returned to the previous frame.
 */
class promise_alloc_stack {

  // Convert an alignment to a std::uintptr_t, ensure its is a power of two and >= k_new_align.
  constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
    auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
    LF_ASSERT(std::has_single_bit(align));
    LF_ASSERT(align > 0);
    return std::max(align, detail::k_new_align);
  }

protected:
  explicit promise_alloc_stack(std::coroutine_handle<> self) noexcept {
    LF_ASSERT(tls::asp);
    tls::asp->set_coro(self);
  }

public:
  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) noexcept -> void * {
    return promise_alloc_stack::operator new(size, std::align_val_t{detail::k_new_align});
  }

  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `tls::asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size, std::align_val_t al) noexcept -> void * {

    std::uintptr_t align = unwrap(al);

    frame_block *prev_asp = tls::asp;

    LF_ASSERT(prev_asp);

    auto prev_stack_addr = std::bit_cast<std::uintptr_t>(prev_asp);

    std::uintptr_t coro_frame_addr = (prev_stack_addr - size) & ~(align - 1);

    LF_ASSERT(coro_frame_addr % align == 0);

    std::uintptr_t frame_addr = coro_frame_addr - sizeof(frame_block);

    LF_ASSERT(frame_addr % alignof(frame_block) == 0);

    // Starts the lifetime of the new frame block.
    tls::asp = new (std::bit_cast<void *>(frame_addr)) frame_block{prev_asp};

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

template <thread_context Context>
void worker_init(Context *context) {
  LF_ASSERT(context);
  LF_ASSERT(!tls::ctx<Context>);
  LF_ASSERT(!tls::asp);

  tls::ctx<Context> = context;
  tls::asp = context->stack_pop()->sentinel();
}

template <thread_context Context>
void worker_finalize(Context *context) {
  LF_ASSERT(context == tls::ctx<Context>);
  LF_ASSERT(tls::asp);
  LF_ASSERT(tls::asp->is_sentinel());

  context->stack_push(async_stack::unsafe_from_sentinel(tls::asp));

  tls::asp = nullptr;
  tls::ctx<Context> = nullptr;
}

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */
