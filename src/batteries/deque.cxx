module;
#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
export module libfork.batteries:deque;

import std;

import libfork.core;
import libfork.utils;

namespace lf {

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
export struct deque_full_error final : libfork_exception {
  [[nodiscard]]
  constexpr auto what() const noexcept -> const char * override {
    return "push failed because deque is full";
  }
};

/**
 * @brief A basic wrapper around a c-style array that provides modulo load/stores.
 *
 * This class is designed for internal use only. It provides a c-style API that is
 * used efficiently by deque for low level atomic operations.
 *
 * @tparam T The type of the elements in the array.
 */
template <dequeable T, allocator_of<std::atomic<T>> Allocator>
struct atomic_ring_buf {
 private:
  using traits = std::allocator_traits<Allocator>;
  using pointer = traits::pointer;

 public:
  using diff_type = traits::difference_type;
  using size_type = traits::size_type;

  /**
   * @brief Construct a new ring buff object
   *
   * @param cap The capacity of the buffer, MUST be a power of 2.
   * @param alloc The allocator used to allocate the buffer.
   */
  constexpr atomic_ring_buf(diff_type cap, Allocator const &alloc)
      : m_alloc{alloc},
        m_cap{cap},
        m_mask{cap - 1} {

    LF_ASSUME(cap > 0 && std::has_single_bit(safe_cast<size_type>(cap)));

    m_buf = traits::allocate(m_alloc, safe_cast<size_type>(cap));

    diff_type i = 0;

    LF_TRY {
      // Begin the lifetime of each atomic.
      for (; i < cap; ++i) {
        traits::construct(m_alloc, std::to_address(m_buf + i));
      }
    } LF_CATCH_ALL {
      clean_up(i);
      LF_RETHROW;
    }
  }

  atomic_ring_buf(atomic_ring_buf const &) = delete;
  atomic_ring_buf(atomic_ring_buf &&) = delete;
  auto operator=(atomic_ring_buf const &) -> atomic_ring_buf & = delete;
  auto operator=(atomic_ring_buf &&) -> atomic_ring_buf & = delete;

  constexpr ~atomic_ring_buf() noexcept { clean_up(m_cap); }

  /**
   * @brief Get the capacity of the buffer.
   */
  [[nodiscard]]
  constexpr auto capacity() const noexcept -> diff_type {
    return m_cap;
  }
  /**
   * @brief Store ``val`` at ``index % this->capacity()``.
   */
  constexpr auto store(diff_type index, T const &val) noexcept -> void {
    LF_ASSUME(index >= 0);
    std::to_address(m_buf + (index & m_mask))->store(val, std::memory_order_relaxed);
  }
  /**
   * @brief Load value at ``index % this->capacity()``.
   */
  [[nodiscard]]
  constexpr auto load(diff_type index) const noexcept -> T {
    LF_ASSUME(index >= 0);
    return std::to_address(m_buf + (index & m_mask))->load(std::memory_order_relaxed);
  }

 private:
  /**
   * @brief Destroy the first `n` elements and deallocate the buffer.
   */
  constexpr void clean_up(diff_type n) noexcept {

    LF_ASSUME(0 <= n && n <= m_cap);

    for (diff_type i = n - 1; i >= 0; --i) {
      traits::destroy(m_alloc, std::to_address(m_buf + i));
    }
    traits::deallocate(m_alloc, m_buf, safe_cast<size_type>(m_cap));
  }

  [[no_unique_address]]
  Allocator m_alloc;
  pointer m_buf{};
  diff_type m_cap;
  diff_type m_mask;
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
  [[nodiscard]]
  static constexpr auto operator()() noexcept -> std::optional<T> {
    return {};
  }
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
class deque {
 public:
  using diff_type = atomic_ring_buf<T, Allocator>::diff_type;
  using size_type = atomic_ring_buf<T, Allocator>::size_type;

  using value_type = T;
  using allocator_type = Allocator;

  deque(deque const &) = delete;
  deque(deque &&) = delete;
  auto operator=(deque const &) -> deque & = delete;
  auto operator=(deque &&) -> deque & = delete;

  /**
   * @brief A non-owning handle that can be used to steal items from the deque.
   *
   * All non-owner interactions with the deque should be made through this handle.
   */
  class thief_handle {

    friend class deque;

    explicit thief_handle(deque *queue) noexcept
        : m_queue{queue} {
      LF_ASSUME(queue != nullptr);
    }

   public:
    /**
     * @brief Check if the deque is empty.
     */
    [[nodiscard]]
    constexpr auto empty(this thief_handle self) noexcept -> bool {
      diff_type const top = self.m_queue->m_top.load(acquire);
      std::atomic_thread_fence(seq_cst);
      diff_type const bottom = self.m_queue->m_bottom.load(acquire);
      return top >= bottom;
    }
    /**
     * @brief Get the number of elements in the deque.
     */
    [[nodiscard]]
    constexpr auto size(this thief_handle self) noexcept -> size_type {
      return safe_cast<size_type>(self.ssize());
    }
    /**
     * @brief Get the number of elements in the deque as a signed integer.
     */
    [[nodiscard]]
    constexpr auto ssize(this thief_handle self) noexcept -> diff_type {
      diff_type const top = self.m_queue->m_top.load(acquire);
      std::atomic_thread_fence(seq_cst);
      diff_type const bottom = self.m_queue->m_bottom.load(acquire);
      return std::max(bottom - top, diff_type{0});
    }
    /**
     * @brief Get the capacity of the deque.
     */
    [[nodiscard]]
    constexpr auto capacity(this thief_handle self) noexcept -> diff_type {
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
      diff_type top = self.m_queue->m_top.load(acquire);
      std::atomic_thread_fence(seq_cst);
      diff_type const bottom = self.m_queue->m_bottom.load(acquire);

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
   * @brief Construct a new empty deque object.
   *
   * @param cap The capacity of the deque (will be rounded to the next power of two).
   * @param alloc Allocator used to allocate the internal buffer.
   */
  constexpr explicit deque(size_type cap, Allocator const &alloc = Allocator())
      : m_buf(round_capacity(cap), alloc) {}

  /**
   * @brief Check if the deque is empty.
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool {
    diff_type const bottom = m_bottom.load(relaxed);
    diff_type const top = m_top.load(seq_cst);
    return top >= bottom;
  }
  /**
   * @brief Get the number of elements in the deque.
   */
  [[nodiscard]]
  constexpr auto size() const noexcept -> size_type {
    return safe_cast<size_type>(ssize());
  }
  /**
   * @brief Get the number of elements in the deque as a signed integer.
   */
  [[nodiscard]]
  constexpr auto ssize() const noexcept -> diff_type {
    diff_type const bottom = m_bottom.load(relaxed);
    diff_type const top = m_top.load(seq_cst);
    return std::max(bottom - top, diff_type{0});
  }
  /**
   * @brief Get the capacity of the deque.
   */
  [[nodiscard]]
  constexpr auto capacity() const noexcept -> diff_type {
    return m_buf.capacity();
  }
  /**
   * @brief Get a non-owning `thief_handle` that can be used to steal items from the deque.
   */
  constexpr auto thief() noexcept -> thief_handle { return thief_handle{this}; }

  /**
   * @brief Push an item into the deque.
   *
   * Only the owner thread can insert an item into the deque. This will throw
   * an exception if the deque is full. This returns the number of elements in
   * the deque before the push.
   *
   * @param val Value to add to the deque.
   */
  constexpr auto push(T val) -> diff_type {

    diff_type const bottom = m_bottom.load(relaxed);
    diff_type const top = m_top.load(acquire);
    diff_type const ssize = bottom - top;

    if (m_buf.capacity() < ssize + 1) {
      LF_THROW(deque_full_error{});
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
  pop(Fn &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<Fn>) -> std::invoke_result_t<Fn> {

    diff_type const bottom = m_bottom.load(relaxed) - 1; //
    m_bottom.store(bottom, relaxed);                     // Stealers can no longer steal.

    std::atomic_thread_fence(seq_cst);

    diff_type top = m_top.load(relaxed);

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

 private:
  alignas(k_cache_line) atomic_ring_buf<T, Allocator> m_buf;
  alignas(k_cache_line) std::atomic<diff_type> m_top{0};
  alignas(k_cache_line) std::atomic<diff_type> m_bottom{0};

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;

  /**
   * @brief Round `cap` up to the next power of two as a `diff_type`.
   */
  static constexpr auto round_capacity(size_type cap) -> diff_type {
    constexpr auto max_cap = std::bit_floor(safe_cast<size_type>(std::numeric_limits<diff_type>::max()));
    LF_ASSUME(0 < cap && cap <= max_cap);
    return safe_cast<diff_type>(std::bit_ceil(cap));
  }
};

} // namespace lf
