#ifndef B74D7802_9253_47C3_BBD2_00D7058AA901
#define B74D7802_9253_47C3_BBD2_00D7058AA901

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <stack>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "libfork/macro.hpp"

#include "libfork/core/exception.hpp"

/**
 * @file stack.hpp
 *
 * @brief The ``lf::virtual_stack`` class and associated utilities.
 */

namespace lf {

namespace detail {

inline constexpr std::size_t k_new_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

template <typename T, typename U>
auto r_cast(U &&expr) noexcept -> T {
  return reinterpret_cast<T>(std::forward<U>(expr)); // NOLINT
}

[[nodiscard]] inline auto aligned_alloc(std::size_t size, std::size_t alignment) -> void * {

  LF_ASSERT(size > 0);                       // Should never want to allocate no memory.
  LF_ASSERT(std::has_single_bit(alignment)); // Need power of 2 alignment.
  LF_ASSERT(size % alignment == 0);          // Size must be a multiple of alignment.

  alignment = std::max(alignment, k_new_align);

  /**
   * Whatever the alignment of an allocated pointer we need to add between 0 and alignment - 1 bytes to
   * bring us to a multiple of alignment. We also need to store the allocated pointer before the returned
   * pointer so adding sizeof(void *) guarantees enough space in-front.
   */

  std::size_t offset = alignment - 1 + sizeof(void *);

  void *original_ptr = ::operator new(size + offset);

  LF_ASSERT(original_ptr);

  std::uintptr_t raw_address = r_cast<std::uintptr_t>(original_ptr);
  std::uintptr_t ret_address = (raw_address + offset) & ~(alignment - 1);

  LF_ASSERT(ret_address % alignment == 0);

  *r_cast<void **>(ret_address - sizeof(void *)) = original_ptr; // Store original pointer

  return r_cast<void *>(ret_address);
}

inline void aligned_free(void *ptr) noexcept {
  LF_ASSERT(ptr);
  ::operator delete(*r_cast<void **>(r_cast<std::uintptr_t>(ptr) - sizeof(void *)));
}

inline constexpr std::size_t kibibyte = 1024 * 1;        // NOLINT
inline constexpr std::size_t mebibyte = 1024 * kibibyte; //
inline constexpr std::size_t gibibyte = 1024 * mebibyte;
inline constexpr std::size_t tebibyte = 1024 * gibibyte;

/**
 * @brief Base class for virtual stacks
 */
struct alignas(k_new_align) stack_mem : exception_packet { // NOLINT
  std::byte *m_ptr;
  std::byte *m_end;
#ifndef NDEBUG
  std::stack<std::pair<void *, std::size_t>> m_debug;
#endif
};

} // namespace detail

/**
 * @brief A program-managed stack for coroutine frames.
 *
 * An ``lf::virtual_stack`` can never be allocated on the heap use ``make_unique()``.
 *
 * @tparam N The number of bytes to allocate for the stack. Must be a power of two.
 */
template <std::size_t N>
  requires(std::has_single_bit(N))
class alignas(detail::k_new_align) virtual_stack : detail::stack_mem {

  // Our trivial destructibility should be equal to this.
  static constexpr bool k_trivial = std::is_trivially_destructible_v<detail::stack_mem>;

  /**
   * @brief Construct an empty stack.
   */
  constexpr virtual_stack() : stack_mem() {

    static_assert(sizeof(virtual_stack) == N, "Bad padding detected!");

    static_assert(k_trivial == std::is_trivially_destructible_v<virtual_stack>);

    if (detail::r_cast<std::uintptr_t>(this) % N != 0) {
#if LF_COMPILER_EXCEPTIONS
      throw unaligned{};
#else
      std::terminate();
#endif
    }
    m_ptr = m_buf.data();
    m_end = m_buf.data() + m_buf.size();
  }

  // Deleter(s) for ``std::unique_ptr``'s

  struct delete_1 {
    LF_STATIC_CALL void operator()(virtual_stack *ptr) LF_STATIC_CONST noexcept {
      if constexpr (!std::is_trivially_destructible_v<virtual_stack>) {
        ptr->~virtual_stack();
      }
      detail::aligned_free(ptr);
    }
  };

  struct delete_n {
    constexpr explicit delete_n(std::size_t count) noexcept : m_count(count) {}

    void operator()(virtual_stack *ptr) const noexcept {
      for (std::size_t i = 0; i < m_count; ++i) {
        ptr[i].~virtual_stack(); // NOLINT
      }
      detail::aligned_free(ptr);
    }

    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return m_count; }

  private:
    std::size_t m_count;
  };

public:
  /**
   * @brief An exception thrown when a stack is not aligned correctly.
   */
  struct unaligned : std::runtime_error {
    unaligned() : std::runtime_error("Virtual stack has the wrong alignment, did you allocate it on the stack?") {}
  };
  /**
   * @brief An exception thrown when a stack is not large enough for a requested allocation.
   */
  struct overflow : std::runtime_error {
    overflow() : std::runtime_error("Virtual stack overflows") {}
  };

  /**
   * @brief A ``std::unique_ptr`` to a single stack.
   */
  using unique_ptr_t = std::unique_ptr<virtual_stack, delete_1>;

  /**
   * @brief A ``std::unique_ptr`` to an array of stacks.
   *
   * The deleter has a ``size()`` member function that returns the number of stacks in the array.
   */
  using unique_arr_ptr_t = std::unique_ptr<virtual_stack[], std::conditional_t<k_trivial, delete_1, delete_n>>; // NOLINT

  /**
   * @brief A non-owning, non-null handle to a virtual stack.
   */
  class handle {
  public:
    /**
     * @brief Construct an empty/null handle.
     */
    constexpr handle() noexcept = default;
    /**
     * @brief Construct a handle to ``stack``.
     */
    explicit constexpr handle(virtual_stack *stack) noexcept : m_stack{stack} {}

    /**
     * @brief Access the stack pointed at by this handle.
     */
    [[nodiscard]] constexpr auto operator*() const noexcept -> virtual_stack {
      LF_ASSERT(m_stack);
      return *m_stack;
    }

    /**
     * @brief Access the stack pointed at by this handle.
     */
    constexpr auto operator->() -> virtual_stack * {
      LF_ASSERT(m_stack);
      return m_stack;
    }

    /**
     * @brief Handles compare equal if they point to the same stack.
     */
    constexpr auto operator<=>(handle const &) const noexcept = default;

  private:
    virtual_stack *m_stack = nullptr;
  };

  /**
   * @brief Allocate an aligned stack.
   *
   * @return A ``std::unique_ptr`` to the allocated stack.
   */
  [[nodiscard]] static auto make_unique() -> unique_ptr_t {
    return {new (detail::aligned_alloc(N, N)) virtual_stack(), {}};
  }

  /**
   * @brief Allocate an array of aligned stacks.
   *
   * @param count The number of elements in the array.
   * @return A ``std::unique_ptr`` to the array of stacks.
   */
  [[nodiscard]] static auto make_unique(std::size_t count) -> unique_arr_ptr_t { // NOLINT

    auto *raw = static_cast<virtual_stack *>(detail::aligned_alloc(N * count, N));

    for (std::size_t i = 0; i < count; ++i) {
      new (raw + i) virtual_stack();
    }

    if constexpr (k_trivial) {
      return {raw, delete_1{}};
    } else {
      return {raw, delete_n{count}};
    }
  }

  /**
   * @brief Test if the stack is empty (and has no exception saved on it).
   */
  auto empty() const noexcept -> bool { return m_ptr == m_buf.data() && !*this; }

  /**
   * @brief Allocate ``n`` bytes on this virtual stack.
   *
   * @param n The number of bytes to allocate.
   * @return A pointer to the allocated memory aligned to ``__STDCPP_DEFAULT_NEW_ALIGNMENT__``.
   */
  [[nodiscard]] constexpr auto allocate(std::size_t const n) -> void * {

    LF_LOG("Allocating {} bytes on the stack", n);

    auto *prev = m_ptr;

    auto *next = m_ptr + align(n); // NOLINT

    if (next >= m_end) {
      LF_LOG("Virtual stack overflows");

#if LF_COMPILER_EXCEPTIONS
      throw overflow();
#else
      std::terminate();
#endif
    }

    m_ptr = next;

#ifndef NDEBUG
    this->m_debug.push({prev, n});
#endif

    return prev;
  }

  /**
   * @brief Deallocate ``n`` bytes on from this virtual stack.
   *
   * Must be called matched with the corresponding call to ``allocate`` in a FILO manner.
   *
   * @param ptr A pointer to the memory to deallocate.
   * @param n The number of bytes to deallocate.
   */
  constexpr auto deallocate(void *ptr, std::size_t n) noexcept -> void {
    //
    LF_LOG("Deallocating {} bytes from the stack", n);

#ifndef NDEBUG
    LF_ASSERT(!this->m_debug.empty());
    LF_ASSERT(this->m_debug.top() == std::make_pair(ptr, n));
    this->m_debug.pop();
#endif

    auto *prev = static_cast<std::byte *>(ptr);

    // Rudimentary check that deallocate is called in a FILO manner.
    LF_ASSERT(prev >= m_buf.data());
    LF_ASSERT(m_ptr - align(n) == prev);

    m_ptr = prev;
  }

  /**
   * @brief Get the stack that is storing ``buf`` in its internal buffer.
   */
  [[nodiscard]] static constexpr auto from_address(void *buf) -> virtual_stack::handle {
    // This utilizes the fact that a stack is aligned to N which is a power of 2.
    //
    //        N  = 001000  or some other power of 2
    //     N - 1 = 000111
    //  ~(N - 1) = 111000

    constexpr auto mask = ~(N - 1);

    auto as_integral = reinterpret_cast<std::uintptr_t>(buf); // NOLINT
    auto address = as_integral & mask;
    auto stack = reinterpret_cast<virtual_stack *>(address); // NOLINT

    return handle{stack};
  }

  // ---------- Exceptions ---------- //

  using detail::exception_packet::rethrow_if_unhandled;

  using detail::exception_packet::unhandled_exception;

private:
  static_assert(N > sizeof(detail::stack_mem), "Stack is too small!");

  alignas(detail::k_new_align) std::array<std::byte, N - sizeof(detail::stack_mem)> m_buf;

  /**
   * @brief Round-up n to a multiple of default new alignment.
   */
  auto align(std::size_t const n) -> std::size_t {

    static_assert(std::has_single_bit(detail::k_new_align));

    //        k_new_align = 001000  or some other power of 2
    //    k_new_align - 1 = 000111
    // ~(k_new_align - 1) = 111000

    constexpr auto mask = ~(detail::k_new_align - 1);

    // (n + k_new_align - 1) ensures when we round-up unless n is already a multiple of k_new_align.

    return (n + detail::k_new_align - 1) & mask;
  }
};

} // namespace lf

#endif /* B74D7802_9253_47C3_BBD2_00D7058AA901 */
