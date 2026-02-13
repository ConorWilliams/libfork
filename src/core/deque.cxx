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

/**
 * @brief Verify a type is suitable for use with `std::atomic`
 *
 * This requires a `TriviallyCopyable` type satisfying both `CopyConstructible` and `CopyAssignable`.
 */
export template <typename T>
concept atomicable = std::is_trivially_copyable_v<T> &&    //
                     std::is_copy_constructible_v<T> &&    //
                     std::is_move_constructible_v<T> &&    //
                     std::is_copy_assignable_v<T> &&       //
                     std::is_move_assignable_v<T> &&       //
                     std::same_as<T, std::remove_cv_t<T>>; //

/**
 * @brief A concept that verifies a type is lock-free when used with `std::atomic`.
 */
export template <typename T>
concept lock_free = atomicable<T> && std::atomic<T>::is_always_lock_free;

/**
 * @brief Test is a type is suitable for use with `lf::deque`.
 *
 * This requires it to be `lf::lock_free` and `std::default_initializable`.
 */
export template <typename T>
concept dequeable = lock_free<T> && std::default_initializable<T>;

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
  constexpr explicit atomic_ring_buf(std::ptrdiff_t cap) : m_cap{cap}, m_mask{cap - 1} {
    LF_ASSUME(cap > 0 && std::has_single_bit(static_cast<std::size_t>(cap)));
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
  /**
   * @brief Copies elements in range ``[bottom, top)`` into a new ring buffer.
   *
   * This function allocates a new buffer and returns a pointer to it.
   * The caller is responsible for deallocating the memory.
   *
   * @param bot The bottom of the range to copy from (inclusive).
   * @param top The top of the range to copy from (exclusive).
   */
  [[nodiscard]]
  constexpr auto resize(std::ptrdiff_t bot, std::ptrdiff_t top) const -> atomic_ring_buf<T> * {

    auto *ptr = new atomic_ring_buf{2 * m_cap}; // NOLINT

    for (std::ptrdiff_t i = top; i != bot; ++i) {
      ptr->store(i, load(i));
    }

    return ptr;
  }

 private:
  /**
   * @brief An array of atomic elements.
   */
  using array_t = std::atomic<T>[]; // NOLINT
  /**
   * @brief Capacity of the buffer.
   */
  std::ptrdiff_t m_cap;
  /**
   * @brief Bit mask to perform modulo capacity operations.
   */
  std::ptrdiff_t m_mask;

#ifdef __cpp_lib_smart_ptr_for_overwrite
  std::unique_ptr<array_t> m_buf = std::make_unique_for_overwrite<array_t>(static_cast<std::size_t>(m_cap));
#else
  std::unique_ptr<array_t> m_buf = std::make_unique<array_t>(static_cast<std::size_t>(m_cap));
#endif
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
  constexpr auto operator*() noexcept -> T & {
    LF_ASSUME(code == err::none);
    return val;
  }
  /**
   * @brief Get the value like ``std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  [[nodiscard]]
  constexpr auto operator*() const noexcept -> T const & {
    LF_ASSUME(code == err::none);
    return val;
  }
  /**
   * @brief Get the value ``like std::optional``.
   *
   * Requires ``code == err::none`` .
   */
  [[nodiscard]]
  constexpr auto operator->() noexcept -> T * {
    LF_ASSUME(code == err::none);
    return std::addressof(val);
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
 * @brief An unbounded lock-free single-producer multiple-consumer work-stealing deque.
 *
 * \rst
 *
 * Implements the "Chase-Lev" deque described in the papers, `"Dynamic Circular Work-Stealing deque"
 * <https://doi.org/10.1145/1073970.1073974>`_ and `"Correct and Efficient Work-Stealing for Weak
 * Memory Models" <https://doi.org/10.1145/2442516.2442524>`_.
 *
 * Only the deque owner can perform ``pop()`` and ``push()`` operations where the deque behaves
 * like a LIFO stack. Others can (only) ``steal()`` data from the deque, they see a FIFO deque.
 * All threads must have finished using the deque before it is destructed.
 *
 *
 * Example:
 *
 * .. include:: ../../../test/source/core/deque.cpp
 *    :code:
 *    :start-after: // !BEGIN-EXAMPLE
 *    :end-before: // !END-EXAMPLE
 *
 * \endrst
 *
 * @tparam T The type of the elements in the deque.
 */
export template <dequeable T>
class deque : immovable {

  static constexpr std::ptrdiff_t k_default_capacity = 1024;
  static constexpr std::size_t k_garbage_reserve = 64;

 public:
  /**
   * @brief The type of the elements in the deque.
   */
  using value_type = T;
  /**
   * @brief Construct a new empty deque object.
   */
  constexpr deque() : deque(k_default_capacity) {}
  /**
   * @brief Construct a new empty deque object.
   *
   * @param cap The capacity of the deque (must be a power of 2).
   */
  constexpr explicit deque(std::ptrdiff_t cap);
  /**
   * @brief Get the number of elements in the deque.
   */
  [[nodiscard]]
  constexpr auto size() const noexcept -> std::size_t;
  /**
   * @brief Get the number of elements in the deque as a signed integer.
   */
  [[nodiscard]]
  constexpr auto ssize() const noexcept -> std::ptrdiff_t;
  /**
   * @brief Get the capacity of the deque.
   */
  [[nodiscard]]
  constexpr auto capacity() const noexcept -> std::ptrdiff_t;
  /**
   * @brief Check if the deque is empty.
   */
  [[nodiscard]]
  constexpr auto empty() const noexcept -> bool;
  /**
   * @brief Push an item into the deque.
   *
   * Only the owner thread can insert an item into the deque.
   * This operation can trigger the deque to resize if more space is required.
   * This may throw if an allocation is required and then fails.
   *
   * @param val Value to add to the deque.
   */
  constexpr void push(T const &val);
  /**
   * @brief Pop an item from the deque.
   *
   * Only the owner thread can pop out an item from the deque. If the buffer is empty calls `when_empty` and
   * returns the result. By default, `when_empty` is a no-op that returns a null `std::optional<T>`.
   */
  template <std::invocable F = return_nullopt<T>>
    requires std::convertible_to<T, std::invoke_result_t<F>>
  constexpr auto pop(F &&when_empty = {}) noexcept(std::is_nothrow_invocable_v<F>) -> std::invoke_result_t<F>;

  /**
   * @brief Steal an item from the deque.
   *
   * Any threads can try to steal an item from the deque. This operation can fail if the deque is
   * empty or if another thread simultaneously stole an item from the deque.
   */
  [[nodiscard]]
  constexpr auto steal() noexcept -> steal_t<T>;

  /**
   * @brief Destroy the deque object.
   *
   * All threads must have finished using the deque before it is destructed.
   */
  constexpr ~deque() noexcept;

 private:
  alignas(k_cache_line) std::atomic<std::ptrdiff_t> m_top;
  alignas(k_cache_line) std::atomic<std::ptrdiff_t> m_bottom;
  alignas(k_cache_line) std::atomic<atomic_ring_buf<T> *> m_buf;
  std::vector<std::unique_ptr<atomic_ring_buf<T>>> m_garbage;

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <dequeable T>
constexpr deque<T>::deque(std::ptrdiff_t cap) : m_top(0),
                                                m_bottom(0),
                                                m_buf(new atomic_ring_buf<T>{cap}) {
  m_garbage.reserve(k_garbage_reserve);
}

template <dequeable T>
constexpr auto deque<T>::size() const noexcept -> std::size_t {
  return static_cast<std::size_t>(ssize());
}

template <dequeable T>
constexpr auto deque<T>::ssize() const noexcept -> std::ptrdiff_t {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(relaxed);
  return std::max(bottom - top, std::ptrdiff_t{0});
}

template <dequeable T>
constexpr auto deque<T>::capacity() const noexcept -> std::ptrdiff_t {
  return m_buf.load(relaxed)->capacity();
}

template <dequeable T>
constexpr auto deque<T>::empty() const noexcept -> bool {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(relaxed);
  return top >= bottom;
}

template <dequeable T>
constexpr auto deque<T>::push(T const &val) -> void {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(acquire);
  atomic_ring_buf<T> *buf = m_buf.load(relaxed);

  if (buf->capacity() < (bottom - top) + 1) {
    // Deque is full, build a new one.
    atomic_ring_buf<T> *bigger = buf->resize(bottom, top);

    [&]() noexcept {
      // This should never throw as we reserve 64 slots.
      m_garbage.emplace_back(std::exchange(buf, bigger));
    }();
    m_buf.store(buf, relaxed);
  }

  // Construct new object, this does not have to be atomic as no one can steal this item until
  // after we store the new value of bottom, ordering is maintained by surrounding atomics.
  buf->store(bottom, val);

  std::atomic_thread_fence(release);
  m_bottom.store(bottom + 1, relaxed);
}

template <dequeable T>
template <std::invocable F>
  requires std::convertible_to<T, std::invoke_result_t<F>>
constexpr auto
deque<T>::pop(F &&when_empty) noexcept(std::is_nothrow_invocable_v<F>) -> std::invoke_result_t<F> {

  std::ptrdiff_t const bottom = m_bottom.load(relaxed) - 1; //
  atomic_ring_buf<T> *buf = m_buf.load(relaxed);            //
  m_bottom.store(bottom, relaxed);                          // Stealers can no longer steal.

  std::atomic_thread_fence(seq_cst);

  std::ptrdiff_t top = m_top.load(relaxed);

  if (top <= bottom) {
    // Non-empty deque
    if (top == bottom) {
      // The last item could get stolen, by a stealer that loaded bottom before our write above.
      if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
        // Failed race, thief got the last item.
        m_bottom.store(bottom + 1, relaxed);
        return std::invoke(std::forward<F>(when_empty));
      }
      m_bottom.store(bottom + 1, relaxed);
    }
    // Can delay load until after acquiring slot as only this thread can push(),
    // This load is not required to be atomic as we are the exclusive writer.
    return buf->load(bottom);
  }
  m_bottom.store(bottom + 1, relaxed);
  return std::invoke(std::forward<F>(when_empty));
}

template <dequeable T>
constexpr auto deque<T>::steal() noexcept -> steal_t<T> {
  std::ptrdiff_t top = m_top.load(acquire);
  std::atomic_thread_fence(seq_cst);
  std::ptrdiff_t const bottom = m_bottom.load(acquire);

  if (top < bottom) {
    // Must load *before* acquiring the slot as slot may be overwritten immediately after
    // acquiring. This load is NOT required to be atomic even-though it may race with an overwrite
    // as we only return the value if we win the race below guaranteeing we had no race during our
    // read. If we loose the race then 'x' could be corrupt due to read-during-write race but as T
    // is trivially destructible this does not matter.
    T tmp = m_buf.load(consume)->load(top);

    static_assert(std::is_trivially_destructible_v<T>, "concept 'atomicable' should guarantee this already");

    if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
      return {.code = err::lost, .val = {}};
    }
    return {.code = err::none, .val = tmp};
  }
  return {.code = err::empty, .val = {}};
}

template <dequeable T>
constexpr deque<T>::~deque() noexcept {
  delete m_buf.load(); // NOLINT
}

} // namespace lf
