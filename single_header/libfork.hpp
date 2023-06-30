#ifndef EDCA974A_808F_4B62_95D5_4D84E31B8911
#define EDCA974A_808F_4B62_95D5_4D84E31B8911

#ifndef E8D38B49_7170_41BC_90E9_6D6389714304
#define E8D38B49_7170_41BC_90E9_6D6389714304

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <type_traits>
#include <utility>

#ifndef D66428B1_3B80_45ED_A7C2_6368A0903810
#define D66428B1_3B80_45ED_A7C2_6368A0903810

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <semaphore>
#include <type_traits>
#include <utility>

#ifndef C5DCA647_8269_46C2_B76F_5FA68738AEDA
#define C5DCA647_8269_46C2_B76F_5FA68738AEDA

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstddef>
#include <new>
#include <utility>
#include <version>

/**
 * @file macro.hpp
 *
 * @brief A collection of internal macros + configuration macros.
 */

// NOLINTBEGIN Sometime macros are the only way to do things...

namespace lf::detail {

/**
 * @brief The cache line size of the current architecture.
 */
#ifdef __cpp_lib_hardware_interference_size
inline constexpr std::size_t k_cache_line = std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t k_cache_line = 64;
#endif

/**
 * @brief An empty type that is not copiable or movable.
 */
struct immovable {
  immovable() = default;
  immovable(const immovable &) = delete;
  immovable(immovable &&) = delete;
  auto operator=(const immovable &) -> immovable & = delete;
  auto operator=(immovable &&) -> immovable & = delete;
  ~immovable() = default;
};

} // namespace lf::detail

/**
 * @brief Use to decorate lambdas and ``operator()`` (alongside ``LF_STATIC_CONST``) and with ``static`` if supported.
 */
#ifdef __cpp_static_call_operator
  #define LF_STATIC_CALL static
#else
  #define LF_STATIC_CALL
#endif

/**
 * @brief Use with ``LF_STATIC_CALL`` to decorate ``operator()`` with ``const`` if supported.
 */
#ifdef __cpp_static_call_operator
  #define LF_STATIC_CONST
#else
  #define LF_STATIC_CONST const
#endif

// clang-format off

/**
 * @brief Use like BOOST_HOF_RETURNS to define a function/lambda with all the noexcept/requires/decltype specifiers.
 * 
 */
#define LF_HOF_RETURNS(expr) noexcept(noexcept(expr)) -> decltype(auto) requires requires { expr; } { return expr;}

// clang-format on

/**
 * @brief Lift an overload-set/template into a constrained lambda.
 */
#define LF_LIFT(overload_set) [](auto &&...args) LF_STATIC_CALL LF_HOF_RETURNS(overload_set(std::forward<decltype(args)>(args)...))

/**
 * @brief Detects if the compiler has exceptions enabled.
 *
 * Overridable by defining ``LF_COMPILER_EXCEPTIONS``.
 */
#ifndef LF_COMPILER_EXCEPTIONS
  #if defined(__cpp_exceptions) || (defined(_MSC_VER) && defined(_CPPUNWIND)) || defined(__EXCEPTIONS)
    #define LF_COMPILER_EXCEPTIONS 1
  #else
    #define LF_COMPILER_EXCEPTIONS 0
  #endif
#endif

/**
 * @brief If truthy then coroutines propagate exceptions, if false then termination is triggered.
 *
 *  * Overridable by defining ``LF_PROPAGATE_EXCEPTIONS``.
 */
#ifndef LF_PROPAGATE_EXCEPTIONS
  #define LF_PROPAGATE_EXCEPTIONS LF_COMPILER_EXCEPTIONS
#endif

#if !LF_COMPILER_EXCEPTIONS && LF_PROPAGATE_EXCEPTIONS
  #error "Cannot propagate exceptions without exceptions enabled!"
#endif

/**
 * @brief A wrapper for C++23's ``[[assume(expr)]]`` attribute.
 *
 * Reverts to compiler specific implementations if the attribute is not
 * available.
 *
 * \rst
 *
 *  .. warning::
 *
 *    Using some intrinsics (i.e. GCC's ``__builtin_unreachable()``) this has
 *    different semantics than ``[[assume(expr)]]`` as it WILL evaluate the
 *    expression at runtime. Hence you should conservatively only use this macro
 *    if ``expr`` is side-effect free and cheap to evaluate.
 *
 * \endrst
 */
#if __has_cpp_attribute(assume)
  #define LF_ASSUME(expr) [[assume(bool(expr))]]
#elif defined(__clang__)
  #define LF_ASSUME(expr) __builtin_assume(bool(expr))
#elif defined(__GNUC__) && !defined(__ICC)
  #define LF_ASSUME(expr)      \
    if (bool(expr)) {          \
    } else {                   \
      __builtin_unreachable(); \
    }
#elif defined(_MSC_VER) || defined(__ICC)
  #define LF_ASSUME(expr) __assume(bool(expr))
#else
  #warning "No LF_ASSUME() implementation for this compiler."
  #define LF_ASSUME(expr) \
    do {                  \
    } while (false)
#endif

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  ``LF_ASSUME(expr)`` otherwise ``assert(expr)``.
 */
#ifndef NDEBUG
  #include <cassert>
  #define LF_ASSERT(expr) assert(expr)
#else
  #define LF_ASSERT(expr) LF_ASSUME(expr)
#endif

/**
 * @brief A customizable logging macro.
 *
 * By default this is a no-op. Defining ``LF_LOGGING`` will enable a default
 * logging implementation which prints to ``std::cout``. Overridable by defining your
 * own ``LF_LOG`` macro. Formats like ``std::format()``.
 */
#ifndef LF_LOG
  #ifdef LF_LOGGING
    #include <iostream>
    #include <mutex>
    #include <thread>
    #include <type_traits>

    #ifdef __cpp_lib_format
      #include <format>
      #define LF_FORMAT(message, ...) std::format((message)__VA_OPT__(, ) __VA_ARGS__)
    #else
      #define LF_FORMAT(message, ...) (message)
    #endif

    #ifdef __cpp_lib_syncbuf
      #include <syncstream>
      #define LF_SYNC_COUT std::osyncstream(std::cout) << std::this_thread::get_id()
    #else
      #define LF_SYNC_COUT std::cout << std::this_thread::get_id()
    #endif

    #define LF_LOG(message, ...)                                                         \
      do {                                                                               \
        if (!std::is_constant_evaluated()) {                                             \
          LF_SYNC_COUT << ": " << LF_FORMAT(message __VA_OPT__(, ) __VA_ARGS__) << '\n'; \
        }                                                                                \
      } while (false)
  #else
    #define LF_LOG(head, ...) \
      do {                    \
      } while (false)
  #endif
#endif

// clang-format on

// NOLINTEND

#endif /* C5DCA647_8269_46C2_B76F_5FA68738AEDA */


#ifndef FE9C96B0_5DDD_4438_A3B0_E77BD54F8673
#define FE9C96B0_5DDD_4438_A3B0_E77BD54F8673

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <version>

/**
 * @file coroutine.hpp
 *
 * @brief Includes \<coroutine\> or \<experimental/coroutine\> depending on the compiler.
 */

#ifndef __has_include
  #error "Missing __has_include macro!"
#endif

// NOLINTBEGIN

#if __has_include(<coroutine>) // Check for a standard library
  #include <coroutine>
namespace lf {
namespace stdx = std;
}
#elif __has_include(<experimental/coroutine>) // Check for an experimental version
  #include <experimental/coroutine>
namespace lf {
namespace stdx = std::experimental;
}
#else
  #error "Missing <coroutine> header!"
#endif

// NOLINTEND

#endif /* FE9C96B0_5DDD_4438_A3B0_E77BD54F8673 */

#ifndef B88D7FC0_BD59_4276_837B_3CAAAB9FC2EF
#define B88D7FC0_BD59_4276_837B_3CAAAB9FC2EF

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <exception>
#include <utility>



/**
 * @file exception.hpp
 *
 * @brief A small type that encapsulates exceptions thrown by children.
 */

namespace lf::detail {

static_assert(std::is_empty_v<immovable>);

#if LF_PROPAGATE_EXCEPTIONS

/**
 * @brief A thread safe std::exception_ptr.
 */
class exception_packet : immovable {
public:
  /**
   * @brief Test if currently storing an exception.
   */
  explicit operator bool() const noexcept { return static_cast<bool>(m_exception); }

  /**
   * @brief Stash an exception, thread-safe.
   */
  void unhandled_exception() noexcept {
    if (!m_ready.test_and_set(std::memory_order_acq_rel)) {
      LF_LOG("Exception saved");
      m_exception = std::current_exception();
      LF_ASSERT(m_exception);
    } else {
      LF_LOG("Exception discarded");
    }
  }

  /**
   * @brief Rethrow if any children threw an exception.
   *
   * This should be called only when a thread is at a join point.
   */
  void rethrow_if_unhandled() {
    if (m_exception) {
      LF_LOG("Rethrowing exception");
      // We are the only thread that can touch this until a steal, which provides the required syncronisation.
      m_ready.clear(std::memory_order_relaxed);
      std::rethrow_exception(std::exchange(m_exception, {}));
    }
  }

private:
  std::exception_ptr m_exception = nullptr;    ///< Exceptions thrown by children.
  std::atomic_flag m_ready = ATOMIC_FLAG_INIT; ///< Whether the exception is ready.
};

#else

class exception_packet {
public:
  explicit constexpr operator bool() const noexcept { return false; }

  void unhandled_exception() noexcept {
  #if LF_COMPILER_EXCEPTIONS
    throw;
  #else
    std::terminate();
  #endif
  }

  void rethrow_if_unhandled() noexcept {}
};

#endif

} // namespace lf::detail

#endif /* B88D7FC0_BD59_4276_837B_3CAAAB9FC2EF */

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





/**
 * @file stack.hpp
 *
 * @brief The ``lf::virtual_stack`` class and associated utilities.
 */

namespace lf {

namespace detail {

inline constexpr std::size_t k_new_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

template <typename T, typename U>
auto r_cast(U &&expr) noexcept {
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


/**
 * @file core.hpp
 *
 * @brief Provides the promise_type's common denominator.
 */

namespace lf {

/**
 * @brief An enumeration that determines the behavior of a coroutine's promise.
 */
enum class tag {
  root,   ///< This coroutine is a root task (allocated on heap) from an ``lf::sync_wait``.
  call,   ///< Non root task (on a virtual stack) from an ``lf::call``.
  fork,   ///< Non root task (on a virtual stack) from an ``lf::fork``.
  invoke, ///< Non root task (on a virtual stack) from invoking directly, implicit re_throw.
};

namespace detail {

// -------------- Control block definition -------------- //

static constexpr std::int32_t k_imax = std::numeric_limits<std::int32_t>::max();

template <typename T>
struct root_block_t {

  exception_packet exception{};
  std::binary_semaphore semaphore{0};
  std::optional<T> result{};
  [[no_unique_address]] immovable anon;

  template <typename U>
    requires std::constructible_from<std::optional<T>, U>
  constexpr auto operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) -> root_block_t & {

    LF_LOG("Root task assigns");

    LF_ASSERT(!result.has_value());
    result.emplace(std::forward<U>(expr));

    return *this;
  }
};

template <>
struct root_block_t<void> {
  exception_packet exception{};
  std::binary_semaphore semaphore{0};
  [[no_unique_address]] immovable anon;
};

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&root_block_t<long>::exception));
static_assert(std::is_pointer_interconvertible_with_class(&root_block_t<void>::exception));
#endif

class promise_base : immovable {
public:
  // Full declaration below, needs concept first
  class handle_t;

  // Either a T* for fork/call or [root/invoke]_block_t *
  constexpr void set_ret_address(void *ret) noexcept {
    m_return_address = ret;
  }
  constexpr void set_parent(stdx::coroutine_handle<promise_base> parent) noexcept {
    m_parent = parent;
  }

  [[nodiscard]] constexpr auto has_parent() const noexcept -> bool {
    return static_cast<bool>(m_parent);
  }

  // Checked access
  [[nodiscard]] constexpr auto parent() const noexcept -> stdx::coroutine_handle<promise_base> {
    LF_ASSERT(has_parent());
    return m_parent;
  }

  [[nodiscard]] constexpr auto ret_address() const noexcept -> void * {
    LF_ASSERT(m_return_address);
    return m_return_address;
  }

  [[nodiscard]] constexpr auto steals() noexcept -> std::int32_t & {
    return m_steal;
  }

  [[nodiscard]] constexpr auto joins() noexcept -> std::atomic_int32_t & {
    return m_join;
  }

  constexpr void reset() noexcept {
    // This is called when taking ownership of a task at a join point.
    LF_ASSERT(m_steal != 0);

    m_steal = 0;
    // Use construct_at(...) to set non-atomically as we know we are the
    // only thread who can touch this control block until a steal which
    // would provide the required memory synchronization.
    static_assert(std::is_trivially_destructible_v<std::atomic_int32_t>);

    std::construct_at(&m_join, k_imax);
  }

  // Increase the debug counter
  constexpr void debug_inc() noexcept {
#ifndef NDEBUG
    LF_ASSERT(m_debug_count < std::numeric_limits<std::int32_t>::max());
    ++m_debug_count;
#endif
  }
  // Fetch the debug count
  constexpr auto debug_count() const noexcept -> std::int64_t {
#ifndef NDEBUG
    return m_debug_count;
#else
    return 0;
#endif
  }
  // Reset the debug counter
  constexpr void debug_reset() noexcept {
#ifndef NDEBUG
    m_debug_count = 0;
#endif
  }

private:
  stdx::coroutine_handle<promise_base> m_parent = {}; ///< Parent task (roots don't have one).
  void *m_return_address = nullptr;                   ///< root_block * || T *
  std::int32_t m_steal = 0;                           ///< Number of steals.
  std::atomic_int32_t m_join = k_imax;                ///< Number of children joined (obfuscated).
#ifndef NDEBUG
  std::int64_t m_debug_count = 0; ///< Number of forks/calls (debug).
#endif
};

// -------------- promise_base -------------- //

template <typename>
struct is_virtual_stack_impl : std::false_type {};

template <std::size_t N>
struct is_virtual_stack_impl<virtual_stack<N>> : std::true_type {
  static_assert(sizeof(virtual_stack<N>) == N);
};

template <typename T>
concept is_virtual_stack = is_virtual_stack_impl<T>::value;

} // namespace detail

/**
 * @brief A handle to a task with a resume() member function.
 */
using task_handle = typename detail::promise_base::handle_t;

/**
 * @brief A concept which requires a type to define a ``stack_type`` which must be a specialization of ``lf::virtual_stack``.
 */
template <typename T>
concept defines_stack = requires { typename T::stack_type; } && detail::is_virtual_stack<typename T::stack_type>;

// clang-format off

/**
 * @brief A concept which defines the context interface.
 *
 * A context owns a LIFO stack of ``lf::virtual_stack``s and a LIFO stack of tasks.
 * The stack of ``lf::virtual_stack``s is expected to never be empty, it should always
 * be able to return an empty ``lf::virtual_stack``.
 *
 * \rst
 *
 * Specifically this requires:
 *
 * .. code::
 *
 *      typename Context::stack_type;
 *
 *      requires // Context::stack_type is a specialization of lf::virtual_stack //;
 *
 *      // Access the thread_local context.
 *      { Context::context() } -> std::same_as<Context &>;
 *
 *      // Check the maximum parallelism.
 *      { Context::max_threads() } -> std::same_as<std::size_t>;
 *
 *      // Access the top stack.
 *      { ctx.stack_top() } -> std::convertible_to<typename Context::stack_type::handle>;
 *      // Remove the top stack.
 *      { ctx.stack_pop() };
 *      // Insert stack at the top.
 *      { ctx.stack_push(stack) };
 *
 *      // Remove and return the top task.
 *      { ctx.task_pop() } -> std::convertible_to<std::optional<task_handle>>;
 *      // Insert task at the top.
 *      { ctx.task_push(handle) };
 *
 *
 * \endrst
 */
template <typename Context>
concept thread_context = defines_stack<Context> && requires(Context ctx, typename Context::stack_type::handle stack, task_handle handle) {
  { Context::context() } -> std::same_as<Context &>; 

  { ctx.max_threads() } -> std::same_as<std::size_t>; 

  { ctx.stack_top() } -> std::convertible_to<typename Context::stack_type::handle>; 
  { ctx.stack_pop() };                                                              
  { ctx.stack_push(stack) };                                                        

  { ctx.task_pop() } -> std::convertible_to<std::optional<task_handle>>; 
  { ctx.task_push(handle) };                                             
};

// clang-format on

// -------------- Define forward decls -------------- //

class detail::promise_base::handle_t : private stdx::coroutine_handle<promise_base> {
public:
  handle_t() = default; ///< To make us a trivial type.

  void resume() noexcept {
    LF_LOG("Call to resume on stolen task");
    LF_ASSERT(*this);

    stdx::coroutine_handle<promise_base>::promise().m_steal += 1;
    stdx::coroutine_handle<promise_base>::resume();
  }

private:
  template <typename R, typename T, thread_context Context, tag Tag>
  friend struct promise_type;

  explicit handle_t(stdx::coroutine_handle<promise_base> handle) : stdx::coroutine_handle<promise_base>{handle} {}
};

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */


#ifndef FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0
#define FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>





#ifndef E91EA187_42EF_436C_A3FF_A86DE54BCDBE
#define E91EA187_42EF_436C_A3FF_A86DE54BCDBE

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>






/**
 * @file task.hpp
 *
 * @brief Implementation of the core ``lf::task`` type.
 */

namespace lf {

/**
 * @brief Test if a type is a stateless class.
 */
template <typename T>
concept stateless = std::is_class_v<T> && std::is_trivial_v<T> && std::is_empty_v<T>;

// ------------------------ Forward decl ------------------------ //

namespace detail {

template <typename, typename, thread_context, tag>
struct promise_type;

} // namespace detail

template <typename T = void>
  requires(!std::is_rvalue_reference_v<T>)
class task;

template <stateless Fn>
struct [[nodiscard]] async_fn;

template <stateless Fn>
struct [[nodiscard]] async_mem_fn;

/**
 * @brief The first argument to all async functions will be passes a type derived from a specialization of this class.
 */
template <typename R, tag Tag, typename AsyncFn, typename... Self>
  requires(sizeof...(Self) <= 1)
struct first_arg_t;

// ------------------------ Interfaces ------------------------ //

// clang-format off

/**
 * @brief Disable rvalue references for T&& template types if an async function is forked.
 *
 * This is to prevent the user from accidentally passing a temporary object to an async function that
 * will then destructed in the parent task before the child task returns.
 */
template <typename T, typename Self>
concept no_forked_rvalue = Self::tag_value != tag::fork || std::is_reference_v<T>;

/**
 * @brief The first argument to all coroutines must conform to this concept and be derived from ``lf::first_arg_t``
 */
template <typename Arg>
concept first_arg =  requires {

  typename Arg::lf_first_arg;  ///< Explicit opt-in.

  typename Arg::return_address_t; ///< The type of the return address pointer

  typename Arg::context_type; /* -> */ requires thread_context<typename Arg::context_type>;

  { Arg::context() } -> std::same_as<typename Arg::context_type &>;

  typename Arg::underlying_fn;  /* -> */ requires stateless<typename Arg::underlying_fn>;
  
  { Arg::tag_value } -> std::convertible_to<tag>;
};

namespace detail {

template <typename T>
concept not_first_arg = !first_arg<std::remove_cvref_t<T>>;

// clang-format on

// ------------------------ Packet ------------------------ //

template <typename>
struct is_task_impl : std::false_type {};

template <typename T>
struct is_task_impl<task<T>> : std::true_type {};

template <typename T>
concept is_task = is_task_impl<T>::value;

template <typename R, tag Tag, typename TaskValueType>
concept result_matches = std::is_void_v<R> || Tag == tag::root || Tag == tag::invoke || std::assignable_from<R &, TaskValueType>;

// clang-format off

template <typename Head, typename... Tail>
concept valid_packet = first_arg<Head> && requires(typename Head::underlying_fn fun, Head head, Tail &&...tail) {
  //  
  { std::invoke(fun, std::move(head), std::forward<Tail>(tail)...) } -> is_task;

} && result_matches<typename Head::return_address_t, Head::tag_value, typename std::invoke_result_t<typename Head::underlying_fn, Head, Tail...>::value_type>;




//&& result_matches<R, Head::tag_value, typename std::invoke_result_t<typename Head::underlying_fn, Head, Tail...>::value_type>;

// clang-format on

struct empty {};

/**
 * @brief An awaitable type (in a task) that triggers a fork/call/invoke.
 *
 * This will store a patched version of Head that includes the return type.
 */
template <typename Head, typename... Tail>
  requires valid_packet<Head, Tail...>
struct [[nodiscard]] packet {
  //
  using return_type = typename Head::return_address_t;
  using task_type = typename std::invoke_result_t<typename Head::underlying_fn, Head, Tail...>;
  using value_type = typename task_type::value_type;
  using promise_type = typename stdx::coroutine_traits<task_type, Head, Tail &&...>::promise_type;
  using handle_type = typename stdx::coroutine_handle<promise_type>;

  [[no_unique_address]] std::conditional_t<std::is_void_v<return_type>, detail::empty, std::add_lvalue_reference_t<return_type>> ret;
  [[no_unique_address]] Head context;
  [[no_unique_address]] std::tuple<Tail &&...> args;

  [[no_unique_address]] immovable _anon = {}; // NOLINT

  /**
   * @brief Call the underlying async function and return a handle to it, sets the return address if ``R != void``.
   */
  auto invoke_bind(stdx::coroutine_handle<promise_base> parent) && -> handle_type {

    LF_ASSERT(parent || Head::tag_value == tag::root);

    auto unwrap = [&]<class... Args>(Args &&...xargs) -> handle_type {
      return handle_type::from_address(std::invoke(typename Head::underlying_fn{}, std::move(context), std::forward<Args>(xargs)...).m_handle);
    };

    handle_type child = std::apply(unwrap, std::move(args));

    child.promise().set_parent(parent);

    if constexpr (!std::is_void_v<return_type>) {
      child.promise().set_ret_address(ret);
    }

    return child;
  }
};

} // namespace detail

// ----------------------------- Task ----------------------------- //

/**
 * @brief The return type for libfork's async functions/coroutines.
 */
template <typename T>
  requires(!std::is_rvalue_reference_v<T>)
class task {
public:
  using value_type = T; ///< The type of the value returned by the coroutine (cannot be a reference, use ``std::reference_wrapper``).

private:
  template <typename Head, typename... Tail>
    requires detail::valid_packet<Head, Tail...>
  friend struct detail::packet;

  template <typename, typename, thread_context, tag>
  friend struct detail::promise_type;

  // Promise constructs, packets accesses.
  explicit constexpr task(void *handle) noexcept : m_handle{handle} {
    LF_ASSERT(handle != nullptr);
  }

  void *m_handle = nullptr; ///< The handle to the coroutine.
};

// ----------------------------- Async function defs ----------------------------- //

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct [[nodiscard]] async_fn {
  /**
   * @brief Use with explicit template-parameter.
   */
  consteval async_fn() = default;

  /**
   * @brief Implicitly constructible from an invocable, deduction guide generated from this.
   */
  explicit(false) consteval async_fn([[maybe_unused]] Fn invocable_which_returns_a_task) {} // NOLINT

  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   *
   * An invoke should not be triggered inside a ``fork``/``call``/``join`` region as the exceptions
   * will be muddled, use ``lf:call`` instead.
   */
  template <typename... Args>
  LF_STATIC_CALL constexpr auto operator()(Args &&...args) LF_STATIC_CONST noexcept -> detail::packet<first_arg_t<void, tag::invoke, async_fn<Fn>>, Args...> {
    return {{}, {}, {std::forward<Args>(args)...}};
  }
};

/**
 * @brief Wraps a stateless callable that returns an ``lf::task``.
 */
template <stateless Fn>
struct [[nodiscard]] async_mem_fn {
  /**
   * @brief Use with explicit template-parameter.
   */
  consteval async_mem_fn() = default;
  /**
   * @brief Implicitly constructible from an invocable, deduction guide generated from this.
   */
  explicit(false) consteval async_mem_fn([[maybe_unused]] Fn invocable_which_returns_a_task) {} // NOLINT
  /**
   * @brief Wrap the arguments into an awaitable (in an ``lf::task``) that triggers an invoke.
   *
   * An invoke should not be triggered inside a ``fork``/``call``/``join`` region as the exceptions
   * will be muddled, use ``lf::call`` instead.
   */
  template <detail::not_first_arg Self, typename... Args>
  LF_STATIC_CALL constexpr auto operator()(Self &&self, Args &&...args) LF_STATIC_CONST noexcept -> detail::packet<first_arg_t<void, tag::invoke, async_mem_fn<Fn>, Self>, Args...> {
    return {{}, {std::forward<Self>(self)}, {std::forward<Args>(args)...}};
  }
};

// ----------------------------- first_arg_t impl ----------------------------- //

namespace detail {

/**
 * @brief A type that satisfies the ``thread_context`` concept.
 */
struct dummy_context {
  using stack_type = virtual_stack<128>;

  static auto context() -> dummy_context &;

  auto max_threads() -> std::size_t;

  auto stack_top() -> typename stack_type::handle;
  auto stack_pop() -> void;
  auto stack_push(typename stack_type::handle) -> void;

  auto task_pop() -> std::optional<task_handle>;
  auto task_push(task_handle) -> void;
};

static_assert(thread_context<dummy_context>, "dummy_context is not a thread_context");

template <typename R, stateless F, tag Tag>
struct first_arg_base {
  using lf_first_arg = std::true_type;
  using context_type = dummy_context;
  using return_address_t = R;
  static auto context() -> context_type &;
  using underlying_fn = F;
  static constexpr tag tag_value = Tag;
};

// static_assert(first_arg<first_arg_base<decltype([] {}), tag::invoke>>, "first_arg_base is not a first_arg_t!");

} // namespace detail

//  * If ``AsyncFn`` is an ``async_fn`` then this will derive from ``async_fn``. If ``AsyncFn`` is an ``async_mem_fn``
//  * then this will wrap a pointer to a class and will supply the appropriate ``*`` and ``->`` operators.
//  *
//  * The full type of the first argument will also have a static ``context()`` member function that will defer to the
//  * thread context's ``context()`` member function.

/**
 * @brief A specialization of ``first_arg_t`` for asynchronous global functions.
 *
 * This derives from the global function to allow to allow for use as a y-combinator.
 */
template <typename R, tag Tag, stateless F>
struct first_arg_t<R, Tag, async_fn<F>> : detail::first_arg_base<R, F, Tag>, async_fn<F> {};

/**
 * @brief A specialization of ``first_arg_t`` for asynchronous member functions.
 *
 * This wraps a pointer to an instance of the parent type to allow for use as
 * an explicit ``this`` parameter.
 */
template <typename R, tag Tag, stateless F, typename Self>
struct first_arg_t<R, Tag, async_mem_fn<F>, Self> : detail::first_arg_base<R, F, Tag> {
  /**
   * @brief Identify if this is a forked task passed an rvalue self parameter.
   */
  static constexpr bool is_forked_rvalue = Tag == tag::fork && !std::is_reference_v<Self>;

  /**
   * @brief If forking a temporary we copy a value to the coroutine frame, otherwise we store a reference.
   */
  using self_type = std::conditional_t<is_forked_rvalue, std::remove_const_t<Self>, Self &&>;

  /**
   * @brief Construct an ``lf::async_mem_fn_for`` from a reference to ``self``.
   */
  explicit(false) constexpr first_arg_t(Self &&self) : m_self{std::forward<Self>(self)} {} // NOLINT

  /**
   * @brief Access the class instance.
   */
  [[nodiscard]] constexpr auto operator*() & noexcept -> std::remove_reference_t<Self> & { return m_self; }

  /**
   * @brief Access the class with a value category corresponding to forwarding a forwarding-reference.
   */
  [[nodiscard]] constexpr auto operator*() && noexcept -> Self && {
    return std::forward<std::conditional_t<is_forked_rvalue, std::remove_const_t<Self>, Self>>(m_self);
  }

  /**
   * @brief Access the underlying ``this`` pointer.
   */
  [[nodiscard]] constexpr auto operator->() noexcept -> std::remove_reference_t<Self> * { return std::addressof(m_self); }

private:
  self_type m_self;
};

} // namespace lf

#endif /* E91EA187_42EF_436C_A3FF_A86DE54BCDBE */


/**
 * @file promise.hpp
 *
 * @brief The promise_type for tasks.
 */

namespace lf::detail {

// -------------------------------------------------------------------------- //

/**
 * @brief An awaitable type (in a task) that triggers a join.
 */
struct join_t {};

#ifndef NDEBUG
  #define FATAL_IN_DEBUG(expr, message)                         \
    do {                                                        \
      if (!(expr)) {                                            \
        []() noexcept { throw std::runtime_error(message); }(); \
      }                                                         \
    } while (false)
#else
  #define FATAL_IN_DEBUG(expr, message) \
    do {                                \
    } while (false)
#endif

// Add in a get/set functions that return a reference to the return object.
template <typename Ret>
struct shim_ret_obj : promise_base {

  using ret_ref = std::conditional_t<std::is_void_v<Ret>, int, std::add_lvalue_reference_t<Ret>>;

  template <typename = void>
    requires(!std::is_void_v<Ret>)
  [[nodiscard]] constexpr auto get_return_address_obj() noexcept -> ret_ref {
    return *static_cast<Ret *>(ret_address());
  }

  template <typename = void>
    requires(!std::is_void_v<Ret>)
  [[nodiscard]] constexpr auto set_ret_address(ret_ref obj) noexcept {
    promise_base::set_ret_address(std::addressof(obj));
  }
};

// Specialization for both non-void
template <typename Ret, typename T, tag Tag>
struct mixin_return : shim_ret_obj<Ret> {
  template <typename U>
    requires std::constructible_from<T, U> && (std::is_void_v<Ret> || std::is_assignable_v<std::add_lvalue_reference_t<Ret>, U>)
  void return_value([[maybe_unused]] U &&expr) noexcept(std::is_void_v<Ret> || std::is_nothrow_assignable_v<std::add_lvalue_reference_t<Ret>, U>) {
    if constexpr (!std::is_void_v<Ret>) {
      this->get_return_address_obj() = std::forward<U>(expr);
    }
  }
};

// Specialization for void returning tasks.
template <typename Ret, tag Tag>
struct mixin_return<Ret, void, Tag> : shim_ret_obj<Ret> {
  static constexpr void return_void() noexcept {}
};

// Adds a context_type type alias to T.
template <thread_context Context, typename Head>
struct with_context : Head {
  using context_type = Context;
  static auto context() -> Context & { return Context::context(); }
};

template <typename R, thread_context Context, typename Head>
struct shim_with_context : with_context<Context, Head> {
  using return_address_t = R;
};

struct regular_void {};

template <typename Ret, typename T, thread_context Context, tag Tag>
struct promise_type : mixin_return<Ret, T, Tag> {
private:
  template <typename Head, typename... Tail>
  constexpr auto add_context_to_packet(packet<Head, Tail...> &&pack) -> packet<with_context<Context, Head>, Tail...> {
    return {pack.ret, {std::move(pack.context)}, std::move(pack.args)};
  }

public:
  using value_type = T;
  using stack_type = typename Context::stack_type;

  [[nodiscard]] static auto operator new(std::size_t const size) -> void * {
    if constexpr (Tag == tag::root) {
      // Use global new.
      return ::operator new(size);
    } else {
      // Use the stack that the thread owns which may not be equal to the parent's stack.
      return Context::context().stack_top()->allocate(size);
    }
  }

  static auto operator delete(void *const ptr, std::size_t const size) noexcept -> void {
    if constexpr (Tag == tag::root) {
#ifdef __cpp_sized_deallocation
      ::operator delete(ptr, size);
#else
      ::operator delete(ptr);
#endif
    } else {
      // When destroying a task we must be the running on the current threads stack.
      LF_ASSERT(stack_type::from_address(ptr) == Context::context().stack_top());
      stack_type::from_address(ptr)->deallocate(ptr, size);
    }
  }

  auto get_return_object() -> task<T> {
    return task<T>{stdx::coroutine_handle<promise_type>::from_promise(*this).address()};
  }

  static auto initial_suspend() -> stdx::suspend_always { return {}; }

  void unhandled_exception() noexcept {

    FATAL_IN_DEBUG(this->debug_count() == 0, "Unhandled exception without a join!");

    if constexpr (Tag == tag::root) {
      LF_LOG("Unhandled exception in root task");
      // Put in our remote root-block.
      this->get_return_address_obj().exception.unhandled_exception();
    } else if (!this->parent().promise().has_parent()) {
      LF_LOG("Unhandled exception in child of root task");
      // Put in parent (root) task's remote root-block.
      // This reinterpret_cast is safe because of the static_asserts in core.hpp.
      reinterpret_cast<exception_packet *>(this->parent().promise().ret_address())->unhandled_exception(); // NOLINT
    } else {
      LF_LOG("Unhandled exception in root's grandchild or further");
      // Put on stack of parent task.
      stack_type::from_address(&this->parent().promise())->unhandled_exception();
    }
  }

  auto final_suspend() noexcept {
    struct final_awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> child) const noexcept -> stdx::coroutine_handle<> {

        if constexpr (Tag == tag::root) {
          LF_LOG("Root task at final suspend, releases sem");

          // Finishing a root task implies our stack is empty and should have no exceptions.
          LF_ASSERT(Context::context().stack_top()->empty());

          child.promise().get_return_address_obj().semaphore.release();

          child.destroy();
          return stdx::noop_coroutine();
        }

        LF_LOG("Task reaches final suspend");

        // Must copy onto stack before destroying child.
        stdx::coroutine_handle<promise_base> const parent_h = child.promise().parent();
        // Can no longer touch child.
        child.destroy();

        if constexpr (Tag == tag::call || Tag == tag::invoke) {
          LF_LOG("Inline task resumes parent");
          // Inline task's parent cannot have been stolen, no need to reset control block.
          return parent_h;
        }

        Context &context = Context::context();

        if (std::optional<task_handle> parent_task_handle = context.task_pop()) {
          // No-one stole continuation, we are the exclusive owner of parent, just keep ripping!
          LF_LOG("Parent not stolen, keeps ripping");
          LF_ASSERT(parent_h.address() == parent_task_handle->address());
          // This must be the same thread that created the parent so it already owns the stack.
          // No steals have occurred so we do not need to call reset().;
          return parent_h;
        }

        // We are either: the thread that created the parent or a thread that completed a forked task.

        // Note: emptying stack implies finished a stolen task or finished a task forked from root.

        // Cases:
        // 1. We are fork_from_root_t
        //    - Every task forked from root is the the first task on a stack -> stack is empty now.
        //      Parent (root) is not on a stack so do not need to take/release control
        // 2. We are fork_t
        //    - Stack is empty -> we cannot be the thread that created the parent as it would be on our stack.
        //    - Stack is non-empty -> we must be the creator of the parent

        // If we created the parent then our current stack is non empty (unless the parent is a root task).
        // If we did not create the parent then we just cleared our current stack and it is now empty.

        LF_LOG("Task's parent was stolen");

        promise_base &parent_cb = parent_h.promise();

        // Register with parent we have completed this child task.
        if (parent_cb.joins().fetch_sub(1, std::memory_order_release) == 1) {
          // Acquire all writes before resuming.
          std::atomic_thread_fence(std::memory_order_acquire);

          // Parent has reached join and we are the last child task to complete.
          // We are the exclusive owner of the parent therefore, we must continue parent.

          LF_LOG("Task is last child to join, resumes parent");

          if (parent_cb.has_parent()) {
            // Must take control of stack if we do not already own it.
            auto parent_stack = stack_type::from_address(&parent_cb);
            auto thread_stack = context.stack_top();

            if (parent_stack != thread_stack) {
              LF_LOG("Thread takes control of parent's stack");
              LF_ASSUME(thread_stack->empty());
              context.stack_push(parent_stack);
            }
          }

          // Must reset parents control block before resuming parent.
          parent_cb.reset();

          return parent_h;
        }

        // Parent has not reached join or we are not the last child to complete.
        // We are now out of jobs, must yield to executor.

        LF_LOG("Task is not last to join");

        if (parent_cb.has_parent()) {
          // We are unable to resume the parent, if we were its creator then we should pop a stack
          // from our context as the resuming thread will take ownership of the parent's stack.
          auto parent_stack = stack_type::from_address(&parent_cb);
          auto thread_stack = context.stack_top();

          if (parent_stack == thread_stack) {
            LF_LOG("Thread releases control of parent's stack");
            context.stack_pop();
          }
        }

        LF_ASSUME(context.stack_top()->empty());

        return stdx::noop_coroutine();
      }
    };

    FATAL_IN_DEBUG(this->debug_count() == 0, "Fork/Call without a join!");

    LF_ASSERT(this->steals() == 0);            // Fork without join.
    LF_ASSERT(this->joins().load() == k_imax); // Destroyed in invalid state.

    return final_awaitable{};
  }

public:
  template <typename R, typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<R, tag::fork, F, This...>, Args...> &&packet) {

    this->debug_inc();

    auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

    stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> parent) noexcept -> decltype(child) {
        // In case *this (awaitable) is destructed by stealer after push
        stdx::coroutine_handle stack_child = m_child;

        LF_LOG("Forking, push parent to context");

        Context::context().task_push(task_handle{promise_type::cast_down(parent)});

        return stack_child;
      }

      decltype(child) m_child;
    };

    return awaitable{{}, child};
  }

  template <typename R, typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<R, tag::call, F, This...>, Args...> &&packet) {

    this->debug_inc();

    auto my_handle = cast_down(stdx::coroutine_handle<promise_type>::from_promise(*this));

    stdx::coroutine_handle child = add_context_to_packet(std::move(packet)).invoke_bind(my_handle);

    struct awaitable : stdx::suspend_always {
      [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept -> decltype(child) {
        return m_child;
      }
      decltype(child) m_child;
    };

    return awaitable{{}, child};
  }

  /**
   * @brief An invoke should never occur within an async scope as the exceptions will get muddled
   */
  template <typename F, typename... This, typename... Args>
  [[nodiscard]] constexpr auto await_transform(packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet) {

    FATAL_IN_DEBUG(this->debug_count() == 0, "Invoke within async scope!");

    using value_type_child = typename packet<first_arg_t<void, tag::invoke, F, This...>, Args...>::value_type;

    using wrapped_value_type = std::conditional_t<std::is_reference_v<value_type_child>, std::reference_wrapper<std::remove_reference_t<value_type_child>>, value_type_child>;

    using return_type = std::conditional_t<std::is_void_v<value_type_child>, regular_void, std::optional<wrapped_value_type>>;

    using packet_type = packet<shim_with_context<return_type, Context, first_arg_t<void, tag::invoke, F, This...>>, Args...>;

    static_assert(std::same_as<value_type_child, typename packet_type::value_type>, "An async function's value_type must be return_address_t independent!");

    struct awaitable : stdx::suspend_always {

      explicit constexpr awaitable(promise_type *in_self, packet<first_arg_t<void, tag::invoke, F, This...>, Args...> &&in_packet) : self(in_self), m_child(packet_type{m_res, {std::move(in_packet.context)}, std::move(in_packet.args)}.invoke_bind(cast_down(stdx::coroutine_handle<promise_type>::from_promise(*self)))) {}

      [[nodiscard]] constexpr auto await_suspend([[maybe_unused]] stdx::coroutine_handle<promise_type> parent) noexcept -> typename packet_type::handle_type {
        return m_child;
      }

      [[nodiscard]] constexpr auto await_resume() -> value_type_child {

        LF_ASSERT(self->steals() == 0);

        // Propagate exceptions.
        if constexpr (LF_PROPAGATE_EXCEPTIONS) {
          if constexpr (Tag == tag::root) {
            self->get_return_address_obj().exception.rethrow_if_unhandled();
          } else {
            stack_type::from_address(self)->rethrow_if_unhandled();
          }
        }

        if constexpr (!std::is_void_v<value_type_child>) {
          LF_ASSERT(m_res.has_value());
          return std::move(*m_res);
        }
      }

      return_type m_res;
      promise_type *self;
      typename packet_type::handle_type m_child;
    };

    return awaitable{this, std::move(in_packet)};
  }

  constexpr auto await_transform([[maybe_unused]] join_t join_tag) noexcept {
    struct awaitable {
    private:
      constexpr void take_stack_reset_control() const noexcept {
        // Steals have happened so we cannot currently own this tasks stack.
        LF_ASSUME(self->steals() != 0);

        if constexpr (Tag != tag::root) {

          LF_LOG("Thread takes control of task's stack");

          Context &context = Context::context();

          auto tasks_stack = stack_type::from_address(self);
          auto thread_stack = context.stack_top();

          LF_ASSERT(thread_stack != tasks_stack);
          LF_ASSERT(thread_stack->empty());

          context.stack_push(tasks_stack);
        }

        // Some steals have happened, need to reset the control block.
        self->reset();
      }

    public:
      [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
        // If no steals then we are the only owner of the parent and we are ready to join.
        if (self->steals() == 0) {
          LF_LOG("Sync ready (no steals)");
          // Therefore no need to reset the control block.
          return true;
        }
        // Currently:            joins() = k_imax - num_joined
        // Hence:       k_imax - joins() = num_joined

        // Could use (relaxed) + (fence(acquire) in truthy branch) but, it's
        // better if we see all the decrements to joins() and avoid suspending
        // the coroutine if possible.
        auto joined = k_imax - self->joins().load(std::memory_order_acquire);

        if (self->steals() == joined) {
          LF_LOG("Sync is ready");

          take_stack_reset_control();

          return true;
        }

        LF_LOG("Sync not ready");
        return false;
      }

      [[nodiscard]] constexpr auto await_suspend(stdx::coroutine_handle<promise_type> task) noexcept -> stdx::coroutine_handle<> {
        // Currently        joins  = k_imax - num_joined
        // We set           joins  = joins() - (k_imax - num_steals)
        //                         = num_steals - num_joined

        // Hence            joined = k_imax - num_joined
        //         k_imax - joined = num_joined

        //  Consider race condition on write to m_context.

        auto steals = self->steals();
        auto joined = self->joins().fetch_sub(k_imax - steals, std::memory_order_release);

        if (steals == k_imax - joined) {
          // We set n after all children had completed therefore we can resume the task.

          // Need to acquire to ensure we see all writes by other threads to the result.
          std::atomic_thread_fence(std::memory_order_acquire);

          LF_LOG("Wins join race");

          take_stack_reset_control();

          return task;
        }
        // Someone else is responsible for running this task and we have run out of work.
        LF_LOG("Looses join race");

        // We cannot currently own this stack.

        if constexpr (Tag != tag::root) {
          LF_ASSERT(stack_type::from_address(self) != Context::context().stack_top());
        }
        LF_ASSERT(Context::context().stack_top()->empty());

        return stdx::noop_coroutine();
      }

      constexpr void await_resume() const {
        LF_LOG("join resumes");
        // Check we have been reset.
        LF_ASSERT(self->steals() == 0);
        LF_ASSERT(self->joins() == k_imax);

        self->debug_reset();

        if constexpr (Tag != tag::root) {
          LF_ASSERT(stack_type::from_address(self) == Context::context().stack_top());
        }

        // Propagate exceptions.
        if constexpr (LF_PROPAGATE_EXCEPTIONS) {
          if constexpr (Tag == tag::root) {
            self->get_return_address_obj().exception.rethrow_if_unhandled();
          } else {
            stack_type::from_address(self)->rethrow_if_unhandled();
          }
        }
      }

      promise_type *self;
    };

    return awaitable{this};
  }

private:
  template <typename Promise>
  static auto cast_down(stdx::coroutine_handle<Promise> this_handle) -> stdx::coroutine_handle<promise_base> {

    // Static checks that UB is OK...

    static_assert(alignof(Promise) == alignof(promise_base), "Promise_type must be aligned to U!");

#ifdef __cpp_lib_is_pointer_interconvertible
    static_assert(std::is_pointer_interconvertible_base_of_v<promise_base, Promise>);
#endif

    stdx::coroutine_handle cast_handle = stdx::coroutine_handle<promise_base>::from_address(this_handle.address());

    // Run-time check that UB is OK.
    LF_ASSERT(cast_handle.address() == this_handle.address());
    LF_ASSERT(stdx::coroutine_handle<>{cast_handle} == stdx::coroutine_handle<>{this_handle});
    LF_ASSERT(static_cast<void *>(&cast_handle.promise()) == static_cast<void *>(&this_handle.promise()));

    return cast_handle;
  }
};

#undef FATAL_IN_DEBUG

} // namespace lf::detail

#endif /* FF9F3B2C_DC2B_44D2_A3C2_6E40F211C5B0 */



/**
 * @file core.hpp
 *
 * @brief Meta header which includes all ``lf::task``, ``lf::fork``, ``lf::call``, ``lf::join`` and ``lf::sync_wait`` machinery.
 */

// clang-format off

#ifndef LF_DOXYGEN_SHOULD_SKIP_THIS

/**
 * @brief Specialize coroutine_traits for task<...> from functions.
 */
template <typename T, lf::first_arg Head, typename... Args>
struct lf::stdx::coroutine_traits<lf::task<T>, Head, Args...> {
  using promise_type = ::lf::detail::promise_type<typename Head::return_address_t, T, typename Head::context_type, Head::tag_value>;
};

/**
 * @brief Specialize coroutine_traits for task<...> from member functions.
 */
template <typename T, lf::detail::not_first_arg Self, lf::first_arg Head, typename... Args>
struct lf::stdx::coroutine_traits<lf::task<T>, Self, Head, Args...> {
  using promise_type = lf::detail::promise_type<typename Head::return_address_t, T, typename Head::context_type, Head::tag_value>;
};

#endif /* LF_DOXYGEN_SHOULD_SKIP_THIS */

namespace lf {

/**
 * @brief A concept which requires a type to define a ``context_type`` which satisfy ``lf::thread_context``.
 */
template <typename T>
concept defines_context = requires { typename std::decay_t<T>::context_type; } && thread_context<typename std::decay_t<T>::context_type>;

/**
 * @brief A concept which defines the requirements for a scheduler.
 * 
 * This requires a type to define a ``context_type`` which satisfies ``lf::thread_context`` and have a ``schedule()`` method
 * which accepts a ``std::coroutine_handle<>`` and guarantees some-thread will call it's ``resume()`` member.
 */
template <typename Scheduler>
concept scheduler = defines_context<Scheduler> && requires(Scheduler &&scheduler) {
  std::forward<Scheduler>(scheduler).schedule(stdx::coroutine_handle<>{});
};

// clang-format on

// NOLINTEND

namespace detail {

template <scheduler Schedule, typename Head, class... Args>
auto sync_wait_impl(Schedule &&scheduler, Head head, Args &&...args) -> typename packet<Head, Args...>::value_type {

  // using packet_t = packet<Head, Args...>;

  using value_type = typename packet<Head, Args...>::value_type;

  using wrapped_value_type = std::conditional_t<std::is_reference_v<value_type>, std::reference_wrapper<std::remove_reference_t<value_type>>, value_type>;

  struct wrap : Head {
    using return_address_t = root_block_t<wrapped_value_type>;
  };

  using packet_type = packet<wrap, Args...>;

  static_assert(std::same_as<value_type, typename packet_type::value_type>, "An async function's value_type must be return_address_t independent!");

  typename wrap::return_address_t root_block;

  auto handle = packet_type{root_block, {std::move(head)}, {std::forward<Args>(args)...}}.invoke_bind(nullptr);

#if LF_COMPILER_EXCEPTIONS
  try {
    std::forward<Schedule>(scheduler).schedule(stdx::coroutine_handle<>{handle});
  } catch (...) {
    // We cannot know whether the coroutine has been resumed or not once we pass to schedule(...).
    // Hence, we do not know whether or not to .destroy() it if schedule(...) throws.
    // Hence we mark noexcept to trigger termination.
    []() noexcept {
      throw;
    }();
  }
#else
  std::forward<Schedule>(scheduler).schedule(stdx::coroutine_handle<>{handle});
#endif

  // Block until the coroutine has finished.
  root_block.semaphore.acquire();

  root_block.exception.rethrow_if_unhandled();

  if constexpr (!std::is_void_v<value_type>) {
    LF_ASSERT(root_block.result.has_value());
    return std::move(*root_block.result);
  }
}

template <scheduler S, typename AsyncFn, typename... Self>
struct root_first_arg_t : with_context<typename std::decay_t<S>::context_type, first_arg_t<void, tag::root, AsyncFn, Self...>> {};

} // namespace detail

/**
 * @brief The entry point for synchronous execution of asynchronous functions.
 *
 * This will create the coroutine and pass its handle to ``scheduler``'s  ``schedule`` method. The caller
 * will then block until the asynchronous function has finished executing.
 * Finally the result of the asynchronous function will be returned to the caller.
 */
template <scheduler S, stateless F, class... Args>
  requires detail::valid_packet<detail::root_first_arg_t<S, async_fn<F>>, Args...>
[[nodiscard]] auto sync_wait(S &&scheduler, [[maybe_unused]] async_fn<F> function, Args &&...args) -> decltype(auto) {
  return detail::sync_wait_impl(std::forward<S>(scheduler), detail::root_first_arg_t<S, async_fn<F>>{}, std::forward<Args>(args)...);
}

/**
 * @brief The entry point for synchronous execution of asynchronous member functions.
 *
 * This will create the coroutine and pass its handle to ``scheduler``'s  ``schedule`` method. The caller
 * will then block until the asynchronous member function has finished executing.
 * Finally the result of the asynchronous member function will be returned to the caller.
 */
template <scheduler S, stateless F, class Self, class... Args>
  requires detail::valid_packet<detail::root_first_arg_t<S, async_mem_fn<F>, Self>, Args...>
[[nodiscard]] auto sync_wait(S &&scheduler, [[maybe_unused]] async_mem_fn<F> function, Self &&self, Args &&...args) -> decltype(auto) {
  return detail::sync_wait_impl(std::forward<S>(scheduler), detail::root_first_arg_t<S, async_mem_fn<F>, Self>{std::forward<Self>(self)}, std::forward<Args>(args)...);
}

/**
 * @brief An invocable (and subscriptable) wrapper that binds a return address to an asynchronous function.
 */
template <tag Tag>
struct bind_task {
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] LF_STATIC_CALL constexpr auto
  operator()(R &ret, [[maybe_unused]] async_fn<F> async) LF_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<first_arg_t<R, Tag, async_fn<F>>, Args...> {
      return {{ret}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] LF_STATIC_CALL constexpr auto operator()([[maybe_unused]] async_fn<F> async) LF_STATIC_CONST noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<first_arg_t<void, Tag, async_fn<F>>, Args...> {
      return {{}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Bind return address `ret` to an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] LF_STATIC_CALL constexpr auto operator()(R &ret, [[maybe_unused]] async_mem_fn<F> async) LF_STATIC_CONST noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self && self, Args && ...args) noexcept -> detail::packet<first_arg_t<R, Tag, async_mem_fn<F>, Self>, Args...> {
      return {{ret}, {std::forward<Self>(self)}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] LF_STATIC_CALL constexpr auto operator()([[maybe_unused]] async_mem_fn<F> async) LF_STATIC_CONST noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self && self, Args && ...args) noexcept -> detail::packet<first_arg_t<void, Tag, async_mem_fn<F>, Self>, Args...> {
      return {{}, {std::forward<Self>(self)}, {std::forward<Args>(args)...}};
    };
  }

#if defined(LF_DOXYGEN_SHOULD_SKIP_THIS) || (defined(__cpp_multidimensional_subscript) && __cpp_multidimensional_subscript >= 202211L)
  /**
   * @brief Bind return address `ret` to an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] static constexpr auto operator[](R &ret, [[maybe_unused]] async_fn<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<first_arg_t<R, Tag, async_fn<F>>, Args...> {
      return {{ret}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] static constexpr auto operator[]([[maybe_unused]] async_fn<F> async) noexcept {
    return [&]<typename... Args>(Args &&...args) noexcept -> detail::packet<first_arg_t<void, Tag, async_fn<F>>, Args...> {
      return {{}, {}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Bind return address `ret` to an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename R, typename F>
  [[nodiscard]] static constexpr auto operator[](R &ret, [[maybe_unused]] async_mem_fn<F> async) noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self && self, Args && ...args) noexcept -> detail::packet<first_arg_t<Tag, async_mem_fn<F>, Self>, Args...> {
      return {{ret}, {std::forward<Self>(self)}, {std::forward<Args>(args)...}};
    };
  }
  /**
   * @brief Set a void return address for an asynchronous member function.
   *
   * @return A functor, that will return an awaitable (in an ``lf::task``), that will trigger a fork/call .
   */
  template <typename F>
  [[nodiscard]] static constexpr auto operator[]([[maybe_unused]] async_mem_fn<F> async) noexcept {
    return [&]<detail::not_first_arg Self, typename... Args>(Self && self, Args && ...args) noexcept -> detail::packet<first_arg_t<Tag, async_mem_fn<F>, Self>, Args...> {
      return {{}, {std::forward<Self>(self)}, {std::forward<Args>(args)...}};
    };
  }
#endif
};

/**
 * @brief An awaitable (in a task) that triggers a join.
 */
inline constexpr detail::join_t join = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a fork.
 */
inline constexpr bind_task<tag::fork> fork = {};

/**
 * @brief A second-order functor used to produce an awaitable (in an ``lf::task``) that will trigger a call.
 */
inline constexpr bind_task<tag::call> call = {};

} // namespace lf

#endif /* E8D38B49_7170_41BC_90E9_6D6389714304 */


#endif /* EDCA974A_808F_4B62_95D5_4D84E31B8911 */
