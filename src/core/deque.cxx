module;
#include "libfork/__impl/assume.hpp"
// #include "libfork/__impl/compiler.hpp"
// #include "libfork/__impl/exception.hpp"
// #include "libfork/__impl/utils.hpp"
export module libfork.core:deque;

import std;

import :utility;
import :concepts;
import :constants;

namespace lf {

// TODO: test if perf is better if we bound the queue

/**
 * @brief Test is a type is suitable for use with `lf::deque`.
 *
 * This requires it to be `lf::lock_free` and `std::default_initializable`.
 */
export template <typename T>
concept dequeable = lock_free<T> && std::default_initializable<T>;

/**
 * @brief Thrown when a push operation fails because the deque is full.
 */
export struct deque_full : std::runtime_error {
  constexpr deque_full() : std::runtime_error{"push failed because deque is full"} {}
};

/**
 * @brief A basic wrapper around a c-style array that provides modulo load/stores.
 *
 * This class is designed for internal use only. It provides a c-style API that is
 * used efficiently by deque for low level atomic operations.
 *
 * @tparam T The type of the elements in the array.
 */
template <dequeable T>
struct atomic_ring_buf {
  /**
   * @brief Construct a new ring buff object
   *
   * @param cap The capacity of the buffer, MUST be a power of 2.
   */
  constexpr explicit atomic_ring_buf(std::ptrdiff_t cap)
      : m_buf{std::make_unique_for_overwrite<std::atomic<T>[]>(safe_cast<std::size_t>(cap))},
        m_cap{cap},
        m_mask{cap - 1} {
    LF_ASSUME(cap > 0 && std::has_single_bit(safe_cast<std::size_t>(cap)));
  }
  /**
   * @brief Get the capacity of the buffer.
   */
  [[nodiscard]]
  constexpr auto capacity() const noexcept -> std::ptrdiff_t {
    return m_cap;
  }
  /**
   * @brief Store ``val`` at ``index % this->capacity()``.
   */
  constexpr auto store(std::ptrdiff_t index, T const &val) noexcept -> void {
    LF_ASSUME(index >= 0);
    (m_buf.get() + (index & m_mask))->store(val, std::memory_order_relaxed); // NOLINT Avoid cast.
  }
  /**
   * @brief Load value at ``index % this->capacity()``.
   */
  [[nodiscard]]
  constexpr auto load(std::ptrdiff_t index) const noexcept -> T {
    LF_ASSUME(index >= 0);
    return (m_buf.get() + (index & m_mask))->load(std::memory_order_relaxed); // NOLINT Avoid cast.
  }

 private:
  /**
   * @brief An array of atomic elements.
   */
  std::unique_ptr<std::atomic<T>[]> m_buf;
  /**
   * @brief Capacity of the buffer.
   */
  std::ptrdiff_t m_cap;
  /**
   * @brief Bit mask to perform modulo capacity operations.
   */
  std::ptrdiff_t m_mask;
};

/**
 * @brief Error codes for ``deque`` 's ``steal()`` operation.
 */
export enum class err : std::uint8_t {
  /**
   * @brief The ``steal()`` operation succeeded.
   */
  none = 0,
  /**
   * @brief  Lost the ``steal()`` race hence, the ``steal()`` operation failed.
   */
  lost,
  /**
   * @brief The deque is empty and hence, the ``steal()`` operation failed.
   */
  empty,
};

/**
 * @brief The return type of a `lf::deque` `steal()` operation.
 *
 * This type is suitable for structured bindings. We return a custom type instead of a
 * `std::optional` to allow for more information to be returned as to why a steal may fail.
 */
export template <typename T>
struct steal_t {
  /**
   * @brief Check if the operation succeeded.
   */
  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return code == err::none;
  }
  /**
   * @brief Get the value like ``std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  [[nodiscard]]
  constexpr auto operator*() const noexcept -> T {
    LF_ASSUME(code == err::none);
    return val;
  }
  /**
   * @brief Get the value ``like std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  [[nodiscard]]
  constexpr auto operator->() const noexcept -> T const * {
    LF_ASSUME(code == err::none);
    return std::addressof(val);
  }
  /**
   * @brief The error code of the ``steal()`` operation.
   */
  err code;
  /**
   * @brief The value stolen from the deque, Only valid if ``code == err::none``.
   */
  T val;
};

/**
 * @brief A functor that returns ``std::nullopt``.
 */
export template <typename T>
struct return_nullopt {
  /**
   * @brief Returns ``std::nullopt``.
   */
  static constexpr auto operator()() noexcept -> std::optional<T> { return {}; }
};

/**
 * @brief A bounded lock-free single-producer multiple-consumer work-stealing deque.
 *
 * Implements the "Chase-Lev" deque described in the papers, `"Dynamic Circular Work-Stealing deque"
 * <https://doi.org/10.1145/1073970.1073974>`_ and `"Correct and Efficient Work-Stealing for Weak
 * Memory Models" <https://doi.org/10.1145/2442516.2442524>`_.
 *
 * Only the deque owner can perform ``pop()`` and ``push()`` operations where the deque behaves
 * like a LIFO stack. Others can (only) ``steal()`` data from the deque, they see a FIFO deque.
 * All threads must have finished using the deque before it is destructed.
 *
 * Also see:

 * - Rust: https://github.com/crossbeam-rs/crossbeam/blob/master/crossbeam-deque/src/deque.rs
 * - CDSC: https://dl.acm.org/doi/epdf/10.1145/2544173.2509514
 *
 * @tparam T The type of the elements in the deque.
 */
export template <dequeable T, allocator_of<std::atomic<T>> Allocator = std::allocator<std::atomic<T>>>
class deque : immovable {
 public:
  /**
   * @brief A non-owning handle that can be used to steal items from the deque.
   *
   * All non-owner interactions with the deque should be made through this handle.
   */
  class thief_handle {

    friend class deque;

    explicit thief_handle(deque *queue) noexcept : m_queue{queue} { LF_ASSUME(queue != nullptr); }

   public:
    /**
     * @brief Check if the deque is empty.
     */
    [[nodiscard]]
    constexpr auto empty(this thief_handle self) noexcept -> bool {
      std::ptrdiff_t const top = self.m_queue->m_top.load(acquire);
      std::atomic_thread_fence(seq_cst);
      std::ptrdiff_t const bottom = self.m_queue->m_bottom.load(acquire);
      return top >= bottom;
    }
    /**
     * @brief Get the number of elements in the deque.
     */
    [[nodiscard]]
    constexpr auto size(this thief_handle self) noexcept -> std::size_t {
      return static_cast<std::size_t>(self.ssize());
    }
    /**
     * @brief Get the number of elements in the deque as a signed integer.
     */
    [[nodiscard]]
    constexpr auto ssize(this thief_handle self) noexcept -> std::ptrdiff_t {
      std::ptrdiff_t const top = self.m_queue->m_top.load(acquire);
      std::atomic_thread_fence(seq_cst);
      std::ptrdiff_t const bottom = self.m_queue->m_bottom.load(acquire);
      return std::max(bottom - top, std::ptrdiff_t{0});
    }
    /**
     * @brief Get the capacity of the deque.
     */
    [[nodiscard]]
    constexpr auto capacity(this thief_handle self) noexcept -> std::ptrdiff_t {
      return self.m_queue->capacity();
    }
    /**
     * @brief Steal an item from the deque.
     *
     * Any threads can try to steal an item from the deque. This operation can
     * fail if the deque is empty or if another thread simultaneously stole an
     * item from the deque.
     */
    constexpr auto steal(this thief_handle self) noexcept -> steal_t<T> {
      //
      std::ptrdiff_t top = self.m_queue->m_top.load(acquire);
      std::atomic_thread_fence(seq_cst);
      std::ptrdiff_t const bottom = self.m_queue->m_bottom.load(acquire);

      if (top < bottom) {
        // Must load *before* acquiring the slot as slot may be overwritten immediately after
        // acquiring. This load is NOT required to be atomic even-though it may race with an overwrite
        // as we only return the value if we win the race below guaranteeing we had no race during our
        // read. If we loose the race then 'x' could be corrupt due to read-during-write race but as T
        // is trivially destructible this does not matter.
        T tmp = self.m_queue->m_buf.load(top);

        static_assert(std::is_trivially_destructible_v<T>, "'atomicable' should guarantee this already");

        if (!self.m_queue->m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
          return {.code = err::lost, .val = {}};
        }
        return {.code = err::none, .val = tmp};
      }
      return {.code = err::empty, .val = {}};
    }

   private:
    deque *m_queue;
  };

  /**
   * @brief The type of the elements in the deque.
   */
  using value_type = T;
  /**
   * @brief Construct a new empty deque object.
   *
   * @param cap The capacity of the deque (will be rounded to the next power of two).
   */
  constexpr explicit deque(std::size_t cap) : m_buf(safe_cast<std::ptrdiff_t>(std::bit_ceil(cap))) {}
  /**
   * @brief Check if the deque is empty.
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    std::ptrdiff_t const bottom = m_bottom.load(relaxed);
    std::ptrdiff_t const top = m_top.load(seq_cst);
    return top >= bottom;
  }
  /**
   * @brief Get the number of elements in the deque.
   */
  [[nodiscard]]
  constexpr auto size() const noexcept -> std::size_t {
    return static_cast<std::size_t>(ssize());
  }
  /**
   * @brief Get the number of elements in the deque as a signed integer.
   */
  [[nodiscard]]
  constexpr auto ssize() const noexcept -> std::ptrdiff_t {
    std::ptrdiff_t const bottom = m_bottom.load(relaxed);
    std::ptrdiff_t const top = m_top.load(seq_cst);
    return std::max(bottom - top, std::ptrdiff_t{0});
  }
  /**
   * @brief Get the capacity of the deque.
   */
  [[nodiscard]]
  constexpr auto capacity() const noexcept -> std::ptrdiff_t {
    return m_buf.capacity();
  }
  /**
   * @brief Push an item into the deque.
   *
   * Only the owner thread can insert an item into the deque. This will throw
   * an exception if the deque is full. This returns the number of elements in
   * the deque before the push.
   *
   * @param val Value to add to the deque.
   */
  constexpr auto push(T val) -> std::ptrdiff_t;
  /**
   * @brief Pop an item from the deque.
   *
   * Only the owner thread can pop out an item from the deque. If the buffer is
   * empty calls `when_empty` and returns the result. By default, `when_empty`
   * is a no-op that returns a null `std::optional<T>`.
   */
  template <std::invocable Fn = return_nullopt<T>>
    requires std::convertible_to<T, std::invoke_result_t<Fn>>
  constexpr auto
  pop(Fn &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<Fn>) -> std::invoke_result_t<Fn>;
  /**
   * @brief Get a non-owning `thief_handle` that can be used to steal items from the deque.
   */
  constexpr auto thief() noexcept -> thief_handle { return thief_handle{this}; }

 private:
  alignas(k_cache_line) atomic_ring_buf<T> m_buf;
  alignas(k_cache_line) std::atomic<std::ptrdiff_t> m_top{0};
  alignas(k_cache_line) std::atomic<std::ptrdiff_t> m_bottom{0};

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <dequeable T, allocator_of<std::atomic<T>> Allocator>
constexpr auto deque<T, Allocator>::push(T val) -> std::ptrdiff_t {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(acquire);
  std::ptrdiff_t const ssize = bottom - top;

  if (m_buf.capacity() < ssize + 1) {
    LF_THROW(deque_full{});
  }

  // Construct new object, this does not have to be atomic as no one can steal
  // this item until after we store the new value of bottom, ordering is
  // maintained by surrounding atomics.
  m_buf.store(bottom, val);

  std::atomic_thread_fence(release);
  m_bottom.store(bottom + 1, relaxed);

  // This was the size just before the push, upon return the size could be any
  // smaller number, down to zero, as stealers could have stolen all the
  // tasks.
  return ssize;
}

// TODO: use the allocator

template <dequeable T, allocator_of<std::atomic<T>> Allocator>
template <std::invocable Fn>
  requires std::convertible_to<T, std::invoke_result_t<Fn>>
constexpr auto deque<T, Allocator>::pop(Fn &&when_empty) noexcept(std::is_nothrow_invocable_v<Fn>)
    -> std::invoke_result_t<Fn> {

  std::ptrdiff_t const bottom = m_bottom.load(relaxed) - 1; //
  m_bottom.store(bottom, relaxed);                          // Stealers can no longer steal.

  std::atomic_thread_fence(seq_cst);

  std::ptrdiff_t top = m_top.load(relaxed);

  if (top <= bottom) {
    // Non-empty deque

    // This load is not required to be atomic as we are the exclusive writer.
    T val = m_buf.load(bottom);

    if (top == bottom) {
      // The last item could get stolen, by a stealer that loaded bottom before our write above.
      if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
        // Failed race, thief got the last item.
        m_bottom.store(bottom + 1, relaxed);
        return std::invoke(std::forward<Fn>(when_empty));
      }
      m_bottom.store(bottom + 1, relaxed);
    }
    return val;
  }
  m_bottom.store(bottom + 1, relaxed);
  return std::invoke(std::forward<Fn>(when_empty));
}

} // namespace lf
