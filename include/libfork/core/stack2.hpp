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
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

#include "libfork/core/aligned_alloc.hpp"

/**
 * @file stack.hpp
 *
 * @brief The ``lf::async_stack`` class and associated utilities.
 */

namespace lf {

namespace detail {

/**
 * @brief Base class for async stacks.
 */
struct alignas(k_new_align) stack_mem : immovable<stack_mem> { // NOLINT
  std::byte *m_ptr;
  std::byte *m_end;
#ifndef NDEBUG
  std::vector<std::pair<void *, std::size_t>> m_debug;
#endif
};

} // namespace detail

/**
 * @brief A program-managed stack for coroutine frames.
 *
 * An ``lf::async_stack`` can never be allocated on the heap use ``make_unique()``.
 */
class async_stack : detail::stack_mem {
  // The size of the stack.
  static constexpr std::size_t k_size = detail::mebibyte;

  /**
   * @brief Construct an empty stack.
   */
  async_stack() {

    if (r_cast<std::uintptr_t>(this) % k_size != 0) {
      LF_THROW(unaligned{});
    }
    m_ptr = m_buf.data();
    m_end = m_buf.data() + m_buf.size();
  }

  // ---------- Deleter(s) for ``std::unique_ptr``'s ---------- //

  struct deleter {
    LF_STATIC_CALL void operator()(async_stack *ptr) LF_STATIC_CONST noexcept {
      if constexpr (!std::is_trivially_destructible_v<async_stack>) {
        ptr->~async_stack();
      }
      aligned_free(ptr);
    }
  };

  struct arr_deleter {
    explicit arr_deleter(std::size_t count) noexcept : m_count(count) {}

    void operator()(async_stack *ptr) const noexcept {
      if constexpr (!std::is_trivially_destructible_v<async_stack>) {
        for (std::size_t i = 0; i < m_count; ++i) {
          ptr[i].~async_stack(); // NOLINT
        }
      }
      aligned_free(ptr);
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
    unaligned() : std::runtime_error("Async stack has the wrong alignment, did you allocate it on the stack?") {}
  };
  /**
   * @brief An exception thrown when a stack is not large enough for a requested allocation.
   */
  struct overflow : std::runtime_error {
    overflow() : std::runtime_error("Async stack overflows!") {}
  };

  /**
   * @brief A non-owning, non-null handle to a async stack.
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
    explicit constexpr handle(async_stack *stack) noexcept : m_stack{stack} {}

    /**
     * @brief Access the stack pointed at by this handle.
     */
    [[nodiscard]] constexpr auto operator*() const noexcept -> async_stack & {
      LF_ASSERT(m_stack);
      return *m_stack;
    }

    /**
     * @brief Access the stack pointed at by this handle.
     */
    constexpr auto operator->() -> async_stack * {
      LF_ASSERT(m_stack);
      return m_stack;
    }

    /**
     * @brief Handles compare equal if they point to the same stack.
     */
    [[nodiscard]] constexpr auto operator<=>(handle const &) const noexcept = default;

  private:
    async_stack *m_stack = nullptr;
  };

  /**
   * @brief A ``std::unique_ptr`` to a single stack.
   */
  using unique_ptr_t = std::unique_ptr<async_stack, deleter>;

  /**
   * @brief A ``std::unique_ptr`` to an array of stacks.
   *
   * The deleter has a ``size()`` member function that returns the number of stacks in the array.
   */
  using unique_arr_ptr_t = std::unique_ptr<async_stack[], arr_deleter>; // NOLINT

  /**
   * @brief Allocate an aligned stack.
   *
   * @return A ``std::unique_ptr`` to the allocated stack.
   */
  [[nodiscard]] static auto make_unique() -> unique_ptr_t {
    return {new (aligned_alloc(k_size, k_size)) async_stack(), {}};
  }

  /**
   * @brief Allocate an array of aligned stacks.
   *
   * @param count The number of elements in the array.
   * @return A ``std::unique_ptr`` to the array of stacks.
   */
  [[nodiscard]] static auto make_unique(std::size_t count) -> unique_arr_ptr_t {

    auto *raw = static_cast<async_stack *>(aligned_alloc(k_size * count, k_size));

    for (std::size_t i = 0; i < count; ++i) {
      new (raw + i) async_stack(); // NOLINT
    }

    return unique_arr_ptr_t{raw, arr_deleter{count}};
  }

  /**
   * @brief Test if the stack is empty (and has no exception saved on it).
   */
  [[nodiscard]] auto empty() const noexcept -> bool { return m_ptr == m_buf.data(); }

  /**
   * @brief Allocate ``n`` bytes on this async stack.
   *
   * @param n The number of bytes to allocate.
   * @return A pointer to the allocated memory aligned to ``__STDCPP_DEFAULT_NEW_ALIGNMENT__``.
   */
  [[nodiscard]] auto allocate(std::size_t n) -> void * {

    LF_LOG("Allocating {} bytes on the stack", n);

    auto *prev = m_ptr;

    auto *next = m_ptr + detail::align(n); // NOLINT

    if (next >= m_end) {
      LF_LOG("Async stack overflows");
      LF_THROW(overflow{});
    }

    m_ptr = next;

#ifndef NDEBUG
    this->m_debug.emplace_back(prev, n);
#endif

    return prev;
  }

  /**
   * @brief Deallocate ``n`` bytes on from this async stack.
   *
   * Must be called matched with the corresponding call to ``allocate`` in a FILO manner.
   *
   * @param ptr A pointer to the memory to deallocate.
   * @param n The number of bytes to deallocate.
   */
  auto deallocate(void *ptr, std::size_t n) noexcept -> void {
    //
    LF_LOG("Deallocating {} bytes from the stack", n);

#ifndef NDEBUG
    LF_ASSERT(!this->m_debug.empty());
    LF_ASSERT(this->m_debug.back() == std::make_pair(ptr, n));
    this->m_debug.pop_back();
#endif

    auto *prev = static_cast<std::byte *>(ptr);

    // Rudimentary check that deallocate is called in a FILO manner.
    LF_ASSERT(prev >= m_buf.data());
    LF_ASSERT(m_ptr - detail::align(n) == prev);

    m_ptr = prev;
  }

  /**
   * @brief Get the stack that is storing ``buf`` in its internal buffer.
   */
  [[nodiscard]] static auto from_address(void *buf) -> async_stack::handle {
    // This utilizes the fact that a stack is aligned to N which is a power of 2.
    //
    //        N  = 001000  or some other power of 2
    //     N - 1 = 000111
    //  ~(N - 1) = 111000

    constexpr auto mask = ~(k_size - 1);

    auto as_integral = reinterpret_cast<std::uintptr_t>(buf); // NOLINT
    auto address = as_integral & mask;
    auto stack = reinterpret_cast<async_stack *>(address); // NOLINT

    return handle{stack};
  }

private:
  alignas(detail::k_new_align) std::array<std::byte, detail::k_async_stack_size - sizeof(detail::stack_mem)> m_buf;
};

} // namespace lf

#endif /* B74D7802_9253_47C3_BBD2_00D7058AA901 */