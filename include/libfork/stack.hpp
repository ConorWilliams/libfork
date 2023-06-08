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
#include <stdexcept>
#include <type_traits>

#include "libfork/exception.hpp"
#include "libfork/macro.hpp"

/**
 * @file stack.hpp
 *
 * @brief The stack class and associated utilities.
 */

namespace lf {

namespace detail {

struct alignas(__STDCPP_DEFAULT_NEW_ALIGNMENT__) stack_mem : exception_packet {
  std::byte *m_ptr;
  std::byte *m_end;
};

} // namespace detail

/**
 * @brief A program-managed stack for coroutine frames.
 *
 * @tparam The number of bytes to allocate for the stack. Must be a power of two.
 */
template <std::size_t N>
  requires(std::has_single_bit(N))
class alignas(N) virtual_stack : private detail::stack_mem {
public:
  /**
   * @brief An exception thrown when the stack overflows.
   */
  struct overflow : std::runtime_error {
    overflow() : std::runtime_error("Virtual stack overflows") {}
  };

  /**
   * @brief A non-owning, non-null handle to a virtual stack.
   */
  class handle {
  public:
    /**
     * @brief Construct a handle to ``stack``.
     */
    explicit constexpr handle(virtual_stack &stack) noexcept : m_stack{&stack} {}

    /**
     * @brief Access the stack pointed at by this handle.
     */
    [[nodiscard]] constexpr auto operator*() const noexcept -> virtual_stack * { return m_stack; }

    /**
     * @brief Access the stack pointed at by this handle.
     */
    constexpr auto operator->() -> virtual_stack * { return m_stack; }

    /**
     * @brief Handles compare equal if they point to the same stack.
     */
    constexpr auto operator<=>(handle const &) const noexcept = default;

  private:
    virtual_stack *m_stack;
  };

  constexpr virtual_stack() noexcept {
    m_ptr = m_buf.data();
    m_end = m_buf.data() + m_buf.size();
  }

  virtual_stack(virtual_stack const &) = delete;
  virtual_stack(virtual_stack &&) = delete;

  auto operator=(virtual_stack const &) -> virtual_stack & = delete;
  auto operator=(virtual_stack &&) -> virtual_stack & = delete;

  ~virtual_stack() = default;

  /**
   * @brief Test if the stack is empty (and has no exception saved on it).
   */
  auto empty() const noexcept -> bool { return m_ptr == m_buf.data() && !*this; }

  /**
   * @brief Allocate ``n`` bytes on the stack.
   *
   * @param n The number of bytes to allocate.
   * @return A pointer to the allocated memory aligned to __STDCPP_DEFAULT_NEW_ALIGNMENT__.
   */
  [[nodiscard]] constexpr auto allocate(std::size_t const n) -> void * {
    //
    LIBFORK_LOG("Allocating {} bytes on the stack", n);

    auto *prev = m_ptr;

    auto *next = m_ptr + align(n); // NOLINT

    if (next >= m_end) {
      LIBFORK_LOG("Virtual stack overflows");

#if LIBFORK_COMPILER_EXCEPTIONS
      throw overflow();
#else
      std::abort();
#endif
    }

    m_ptr = next;

    return prev;
  }

  /**
   * @brief Deallocate ``n`` bytes on the stack.
   *
   * Must be called matched with the corresponding call to ``allocate`` in a FILO manner.
   *
   * @param ptr A pointer to the memory to deallocate.
   * @param n The number of bytes to deallocate.
   */
  constexpr auto deallocate(void *ptr, std::size_t n) noexcept -> void {
    //
    LIBFORK_LOG("Deallocating {} bytes from the stack", n);

    auto *prev = static_cast<std::byte *>(ptr);

    // Rudimentary check that deallocate is called in a FILO manner.
    LIBFORK_ASSERT(prev >= m_buf.data());
    LIBFORK_ASSERT(m_ptr - align(n) == prev);

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

    return handle{*stack};
  }

  // ---------- Exceptions ---------- //

  using detail::exception_packet::rethrow_if_unhandled;

  using detail::exception_packet::unhandled_exception;

private:
  static constexpr std::size_t k_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

  static_assert(N > sizeof(detail::stack_mem), "Stack is too small!");

  alignas(k_align) std::array<std::byte, N - sizeof(detail::stack_mem)> m_buf;

  /**
   * @brief Round-up n to a multiple of default new alignment.
   */
  auto align(std::size_t const n) -> std::size_t {

    static_assert(std::has_single_bit(k_align));

    //        k_align = 001000  or some other power of 2
    //    k_align - 1 = 000111
    // ~(k_align - 1) = 111000

    constexpr auto mask = ~(k_align - 1);

    // (n + k_align - 1) ensures when we round-up unless n is already a multiple of k_align.

    return (n + k_align - 1) & mask;
  }
};

} // namespace lf

#endif /* B74D7802_9253_47C3_BBD2_00D7058AA901 */
