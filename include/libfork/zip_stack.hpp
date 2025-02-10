#ifndef C4F946B7_9F01_49F3_B8F7_B9235E901BD7
#define C4F946B7_9F01_49F3_B8F7_B9235E901BD7

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "libfork/macro.hpp"
#include "libfork/utility.hpp"

/**
 * @file zip_stack.hpp
 *
 * @brief ...
 *
 *  |<-------------------------------
 *  | ^         ^         ^         ^
 *  | |         |         |         |
 * [R L...] <- [L...] <- [L...] <- [L...]
 *  |                               ^
 *  v                               |
 *  ------------------------------->|
 *
 * Weak has ptr to root, can reclaim by fetching cache.
 *
 * Strong has ptr to root and stack_span:
 *  - Alloc fast-path, bump stack_span
 *  - Dealloc fast-path, bump stack_span
 *  - Alloc slow path, (del top if empty), alloc new stacklet (maybe a new root)
 *  - Dealloc slow path, del top, move top to prev stacklet
 *
 * All the stacklets of offset from each other to introduce hysteresis.
 */

namespace lf {

namespace detail {

/**
 * @brief A region of memory that is part of a zip-stack.
 *
 * Aligning to `k_new_align` ensures that the stacklet's actual stack has the
 * same alignment.
 */
struct alignas(k_new_align) stacklet {

  auto is_root() const noexcept -> bool { return prev != nullptr; }

  std::byte *lo; ///< The start of the stacklet's stack.
  std::byte *hi; ///< The end of the stacklet's stack.

  stacklet *prev; ///< Singly linked list (past), null if root.
};

struct allocation_record {
  void *ptr = nullptr;   ///< The start of the allocation.
  std::size_t count = 0; ///< The size of the allocation (bytes).
};

struct stack_debug {

  stack_debug() = default;
  stack_debug(stack_debug const &) = delete;
  stack_debug(stack_debug &&) = delete;
  auto operator=(stack_debug const &) -> stack_debug & = delete;
  auto operator=(stack_debug &&) -> stack_debug & = delete;

  std::vector<allocation_record> allocations; ///< Order of allocations.
  int weak_count = 0;                         ///< Number of weak pointers.
  bool owned = true;                          ///< If a zip_stack exists then this is owned.

  ~stack_debug() {
    LF_ASSERT(weak_count == 0, "Weak count is non-zero at destruction");
    LF_ASSERT(allocations.empty(), "Allocations are not empty at destruction");
    LF_ASSERT(owned, "Root stacklet is not owned at destruction");
  }
};

// Keep stack aligned.
static_assert(sizeof(stacklet) >= k_new_align);
static_assert(sizeof(stacklet) % k_new_align == 0);

} // namespace detail

/**
 * @class stack
 * @brief The root stack
 *
 */
class stack : detail::stacklet {

  std::byte *m_sp; ///< Cache of the stack pointer (for release).
  stacklet *m_top; ///< The top stacklet of the zip-stack.

#ifndef NDEBUG
  detail::stack_debug m_debug; ///< Debug information (only stored in the root).
#endif

  /**
   * @brief Construct a new root stacklet object.
   *
   * @param lo The start of the stacklet's stack.
   * @param hi The end of the stacklet's stack.
   */
  stack(std::byte *lo, std::byte *hi) noexcept
      : stacklet{.lo = lo, .hi = hi, .prev = nullptr},
        m_sp{lo},
        m_top{this} {}

  /**
   * @brief Allocate a new zip-stacklet with a stack of size of at least `count`
   * bytes.
   */
  friend auto root(std::size_t count) -> stack *;

  /**
   * @brief Allocate a new zip-stacklet with a stack of size of at least
   * `count` bytes and attach it to the top of this zip-stacklet chain.
   *
   * Returns a pointer to the new top stacklet.
   */
  auto push(std::size_t count) -> stacklet *;

  /**
   * @brief Free the top stacklet.
   *
   * This requires that the root stacklet is not the top stacklet.
   */
  void pop() noexcept;

 public:
  stack(stack const &) = delete;
  stack(stack &&) = delete;
  auto operator=(stack const &) -> stack & = delete;
  auto operator=(stack &&) -> stack & = delete;

  ~stack() { LF_ASSERT(m_top == static_cast<stacklet *>(this), "destroyed with allocations"); }

  // Forward decl
  class handle;

  /**
   * @brief A weak handle to a zip-stack.
   *
   * Weak handles can be obtained from a zip-stack and stored in a region of
   * memory allocated on the zip-stack. Hence, their lifetime should be entirely
   * nested withing the lifetime of allocation and thus also nested within the
   * lifetime of the zip-stack they refer to. Weak zip-stack pointers remains
   * valid throughout calls to release/acquire on the underlying zip-stack.
   */
  class weak_handle {
   public:
    /**
     * @brief Convert to an owning handle to the underlying zip-stack.
     *
     * This consumes the weak handle and leaves it in a null state.
     *
     * Requires the underlying zip-stack has been released and is unowned.
     */
    [[nodiscard]] auto acquire() && noexcept -> handle {
      // Will drop weak count via RAII
      std::unique_ptr root = std::move(m_root);

      LF_ASSERT(root, "acquire on null");
      LF_JUST_ASSERT(!std::exchange(root->m_debug.owned, true), "acquire on owned");

      return handle{root.get()};
    }

    /**
     * @brief Check if this refers to the same zip-stack as `other`.
     */
    [[nodiscard]] auto operator==(handle const &other) const noexcept -> bool {
      return m_root.get() == other.m_root.get();
    }

   private:
    friend class handle;
    /**
     * @brief Construct a new weak handle.
     */
    explicit weak_handle(stack *ptr) noexcept : m_root{ptr} {
      LF_ASSERT(m_root, "weak handle must be non-null");
      LF_JUST_ASSERT(++m_root->m_debug.weak_count > 0);
    }

    /**
     * @brief Decrements internal counters.
     */
    struct deleter {
      static void operator()(stack *ptr) noexcept {
        LF_JUST_ASSERT(--ptr->m_debug.weak_count >= 0);
      }
    };

    std::unique_ptr<stack, deleter> m_root;
  };

  /**
   * @brief An owning handle to a zip-stack.
   */
  class handle {
   public:
    /**
     * @brief Construct a new null zip-stack
     */
    handle() = default;

    /**
     * @brief Check if the zip-stack is null.
     */
    [[nodiscard]] operator bool() const noexcept -> bool { return static_cast<bool>(m_root); }

    /**
     * @brief Construct a weak handle to this zip-stack.
     *
     * Requires that this zip-stack is non-null.
     */
    [[nodiscard]] auto weak() noexcept -> weak_handle { return weak_handle{m_root.get()}; }

    /**
     * @brief Release ownership of the zip-stack.
     *
     * This requires that the zip-stack is non-null.
     *
     * After this operation the zip-stack is null, A weak handle must be
     * acquired at some point to destroy the zip-stack.
     */

    void release() && noexcept {
      LF_ASSERT(m_root, "release of null");
      LF_JUST_ASSERT(std::exchange(m_root->m_debug.owned, true), "release unowned");

      m_root->m_sp = m_sp;
      // Ok to ignore return, it is the callers responsibility to have made a
      // weak handle.
      auto _ = m_root.release();

      // Reset to ensure empty.
      m_lo = nullptr;
      m_sp = nullptr;
      m_hi = nullptr;
      LF_ASSERT(empty());
      LF_ASSERT(unused() == 0);
    }

    /**
     * @brief Allocate `count` bytes of memory on the zip-stack.
     *
     * The zip stack may be null, the count must be greater than zero.
     *
     * The memory will be aligned to a multiple of
     * `__STDCPP_DEFAULT_NEW_ALIGNMENT__`.
     *
     * Deallocate the memory with `deallocate` in a FILO manor.
     */
    [[nodiscard]] auto allocate(std::size_t count) -> void * {

      LF_ASSERT(count > 0, "Cannot allocate zero bytes");
      LF_ASSERT(!m_root || empty(), "Null implies empty");
      LF_ASSERT(!m_root || unused() == 0, "Null implies allocate");

      std::size_t rounded = round(count);

      if (unused() < rounded) {
        // Cold path
        push_stacklet(rounded);
      }

      void *ptr = std::exchange(m_sp, m_sp + rounded);

#ifndef NDEBUG
      LF_TRY {
        m_root->m_debug.allocations.push_back({ptr, count});
      } LF_CATCH_ALL {
        deallocate(ptr, count);
        LF_RETHROW;
      }
#endif

      return ptr;
    }

    /**
     * @brief Deallocate memory allocated on the zip-stack.
     *
     * The zip stack must not be null.

     * The memory must have been allocated with `allocate` and must be
     * deallocated in a FILO manor.
     */
    void deallocate(void *ptr, std::size_t count) noexcept {
      //
      LF_ASSERT(m_root, "deallocate on null");

#ifndef NDEBUG
      LF_ASSERT(!m_root->m_debug.allocations.empty(), "deallocate on empty");
      auto &last = m_root->m_debug.allocations.back();
      LF_ASSERT(last.ptr == ptr, "deallocate on wrong ptr");
      LF_ASSERT(last.count == count, "deallocate with wrong count");
      m_root->m_debug.allocations.pop_back();
#endif

      std::size_t rounded = round(count);

      if (used() < rounded) {
        // Slow path
        return pop_stacklet(ptr);
      }

      m_sp -= rounded;
    }

   private:
    friend class weak_handle;

    /**
     * @brief Construct a new handle object taking ownership of the zip-stack.
     */
    explicit handle(stack *ptr) noexcept
        : m_root{ptr},
          m_lo{ptr->lo},
          m_sp{ptr->m_sp},
          m_hi{ptr->hi} {}

    /**
     * @brief Round to a multiple of '__STDCPP_DEFAULT_NEW_ALIGNMENT__'.
     */
    static constexpr auto round(std::size_t n) -> std::size_t {
      return (n + detail::k_new_align - 1) & ~(detail::k_new_align - 1);
    }

    /**
     * @brief Properly delete the paired allocation.
     */
    struct deleter {
      static void operator()(stack *ptr) noexcept;
    };

    /**
     * @brief The control block of the zip-stack.
     */
    std::unique_ptr<stack, deleter> m_root;
    /**
     * @brief A cache of the top stacklet's `m_lo`
     */
    std::byte *m_lo = nullptr;
    /**
     * @brief The current position of the stack pointer in the stack.
     */
    std::byte *m_sp = nullptr;
    /**
     * @brief A cache of the top stacklet's `m_hi`
     */
    std::byte *m_hi = nullptr;

    /**
     * @brief Capacity of the current top stacklet's stack.
     */
    [[nodiscard]] auto capacity() const noexcept -> std::size_t {
      return detail::checked_cast<std::size_t>(m_hi - m_lo);
    }
    /**
     * @brief Unused space on the top stacklet's stack.
     */
    [[nodiscard]] auto unused() const noexcept -> std::size_t {
      return detail::checked_cast<std::size_t>(m_hi - m_sp);
    }
    /**
     * @brief Used space on the top stacklet's stack.
     */
    [[nodiscard]] auto used() const noexcept -> std::size_t {
      return detail::checked_cast<std::size_t>(m_sp - m_lo);
    }
    /**
     * @brief Check if top stack is empty.
     */
    [[nodiscard]] auto empty() const noexcept -> bool { return m_sp == m_lo; }

    /**
     * @brief Allocate new stacklet cleaningig up possibly empty top.
     */
    auto push_stacklet(std::size_t count) -> void;
    /**
     * @brief Drop empty top stacklet.
     */
    auto pop_stacklet(void *ptr) noexcept -> void;
  };
};

// Keep stack aligned.
static_assert(sizeof(stack) >= detail::k_new_align);
static_assert(sizeof(stack) % detail::k_new_align == 0);

} // namespace lf

#endif /* C4F946B7_9F01_49F3_B8F7_B9235E901BD7 */
