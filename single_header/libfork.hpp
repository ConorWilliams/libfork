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

// clang-format off

template <typename R, tag Tag, typename TaskValueType>
concept result_matches = std::is_void_v<R> || Tag == tag::root || Tag == tag::invoke || std::assignable_from<R &, TaskValueType>;


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
  [[nodiscard]] constexpr auto operator*() &noexcept -> std::remove_reference_t<Self> & { return m_self; }

  /**
   * @brief Access the class with a value category corresponding to forwarding a forwarding-reference.
   */
  [[nodiscard]] constexpr auto operator*() &&noexcept -> Self && {
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

#if !defined(NDEBUG) && LF_COMPILER_EXCEPTIONS
  #define FATAL_IN_DEBUG(expr, message)      \
    do {                                     \
      if (!(expr)) {                         \
        []() noexcept {                      \
          throw std::runtime_error(message); \
        }();                                 \
      }                                      \
    } while (false)
#elif !defined(NDEBUG) && !LF_COMPILER_EXCEPTIONS
  #define FATAL_IN_DEBUG(expr, message) \
    do {                                \
      if (!(expr)) {                    \
        std::terminate();               \
      }                                 \
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


#ifndef B5AE1829_6F8A_4118_AB15_FE73F851271F
#define B5AE1829_6F8A_4118_AB15_FE73F851271F

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>
#include <random>
#include <thread>


#pragma once

// This file has been modified by C.J.Williams to act as a standalone
// version of ``folly::event_count`` utilizing C++20's atomic wait facilities.
//
// Copyright (c) Conor Williams, Meta Platforms, Inc. and its affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <atomic>
#include <bit>
#include <cstdint>
#include <functional>
#include <thread>
#include <type_traits>



/**
 * @file event_count.hpp
 *
 * @brief A standalone adaptation of ``folly::EventCount``.
 */

namespace lf {

namespace detail {

constexpr bool k_is_little_endian = std::endian::native != std::endian::little;

constexpr bool k_is_big_endian = std::endian::native != std::endian::big;

static_assert(k_is_little_endian || k_is_big_endian, "mixed endian systems are not supported");

} // namespace detail

/**
 * @brief A condition variable for lock free algorithms.
 *
 * \rst
 *
 * See http://www.1024cores.net/home/lock-free-algorithms/eventcounts for details.
 *
 * Event counts allow you to convert a non-blocking lock-free / wait-free
 * algorithm into a blocking one, by isolating the blocking logic.  You call
 * prepare_wait() before checking your condition and then either cancel_wait()
 * or wait() depending on whether the condition was true.  When another
 * thread makes the condition true, it must call notify() / notify_all() just
 * like a regular condition variable.
 *
 * If "<" denotes the happens-before relationship, consider 2 threads (T1 and
 * T2) and 3 events:
 *
 * * E1: T1 returns from prepare_wait.
 * * E2: T1 calls wait (obviously E1 < E2, intra-thread).
 * * E3: T2 calls ``notify_all()``.
 *
 * If E1 < E3, then E2's wait will complete (and T1 will either wake up,
 * or not block at all)
 *
 * This means that you can use an event_count in the following manner:
 *
 * Waiter:
 *
 * .. code::
 *
 *    if (!condition()) {  // handle fast path first
 *      for (;;) {
 *
 *        auto key = eventCount.prepare_wait();
 *
 *        if (condition()) {
 *          eventCount.cancel_wait();
 *          break;
 *        } else {
 *          eventCount.wait(key);
 *        }
 *      }
 *    }
 *
 * (This pattern is encapsulated in the ``await()`` method.)
 *
 * Poster:
 *
 * .. code::
 *
 *    make_condition_true();
 *    eventCount.notify_all();
 *
 * .. note::
 *
 *    Just like with regular condition variables, the waiter needs to
 *    be tolerant of spurious wakeups and needs to recheck the condition after
 *    being woken up.  Also, as there is no mutual exclusion implied, "checking"
 *    the condition likely means attempting an operation on an underlying
 *    data structure (push into a lock-free queue, etc) and returning true on
 *    success and false on failure.
 *
 * \endrst
 */
class event_count {
public:
  event_count() = default;
  event_count(event_count const &) = delete;
  event_count(event_count &&) = delete;
  auto operator=(event_count const &) -> event_count & = delete;
  auto operator=(event_count &&) -> event_count & = delete;
  ~event_count() = default;

  /**
   * @brief The return type of ``prepare_wait()``.
   */
  class key {
    friend class event_count;
    explicit key(uint32_t epoch) noexcept : m_epoch(epoch) {}
    std::uint32_t m_epoch;
  };
  /**
   * @brief Wake up one waiter.
   */
  auto notify_one() noexcept -> void;
  /**
   * @brief Wake up all waiters.
   */
  auto notify_all() noexcept -> void;
  /**
   * @brief Prepare to wait.
   *
   * Once this has been called, you must either call ``wait()`` with the key or ``cancel_wait()``.
   */
  [[nodiscard]] auto prepare_wait() noexcept -> key;
  /**
   * @brief Cancel a wait that was prepared with ``prepare_wait()``.
   */
  auto cancel_wait() noexcept -> void;
  /**
   * @brief Wait for a notification, this blocks the current thread.
   */
  auto wait(key in_key) noexcept -> void;

  /**
   * Wait for ``condition()`` to become true.
   *
   * Cleans up appropriately if ``condition()`` throws, and then rethrow.
   */
  template <typename Pred>
    requires std::is_invocable_r_v<bool, Pred const &>
  void await(Pred const &condition);

private:
  auto epoch() noexcept -> std::atomic<std::uint32_t> * {
    return reinterpret_cast<std::atomic<std::uint32_t> *>(&m_val) + k_epoch_offset; // NOLINT
  }

  // This requires 64-bit
  static constexpr std::size_t k_4byte = 4;
  static constexpr std::size_t k_8byte = 8;

  static_assert(sizeof(int) == k_4byte, "bad platform, need 64 bit native int");
  static_assert(sizeof(std::uint32_t) == k_4byte, "bad platform, need 32 bit ints");
  static_assert(sizeof(std::uint64_t) == k_8byte, "bad platform, need 64 bit ints");
  static_assert(sizeof(std::atomic<std::uint32_t>) == k_4byte, "bad platform, need 32 bit atomic ints");
  static_assert(sizeof(std::atomic<std::uint64_t>) == k_8byte, "bad platform, need 64 bit atomic ints");

  static constexpr size_t k_epoch_offset = detail::k_is_little_endian ? 1 : 0;

  static constexpr std::uint64_t k_add_waiter = 1;
  static constexpr std::uint64_t k_sub_waiter = static_cast<std::uint64_t>(-1);
  static constexpr std::uint64_t k_epoch_shift = 32;
  static constexpr std::uint64_t k_add_epoch = static_cast<std::uint64_t>(1) << k_epoch_shift;
  static constexpr std::uint64_t k_waiter_mask = k_add_epoch - 1;

  // m_val stores the epoch in the most significant 32 bits and the
  // waiter count in the least significant 32 bits.
  alignas(detail::k_cache_line) std::atomic<std::uint64_t> m_val = 0;
};

inline void event_count::notify_one() noexcept {
  if (m_val.fetch_add(k_add_epoch, std::memory_order_acq_rel) & k_waiter_mask) [[unlikely]] { // NOLINT
    LF_LOG("notify");
    epoch()->notify_one();
  }
}

inline void event_count::notify_all() noexcept {
  if (m_val.fetch_add(k_add_epoch, std::memory_order_acq_rel) & k_waiter_mask) [[unlikely]] { // NOLINT
    LF_LOG("notify");
    epoch()->notify_all();
  }
}

[[nodiscard]] inline auto event_count::prepare_wait() noexcept -> event_count::key {
  auto prev = m_val.fetch_add(k_add_waiter, std::memory_order_acq_rel);
  // Cast is safe because we're only using the lower 32 bits.
  return key(static_cast<std::uint32_t>(prev >> k_epoch_shift));
}

inline void event_count::cancel_wait() noexcept {
  // memory_order_relaxed would suffice for correctness, but the faster
  // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
  // (and thus system calls).
  auto prev = m_val.fetch_add(k_sub_waiter, std::memory_order_seq_cst);

  LF_ASSERT((prev & k_waiter_mask) != 0);
}

inline void event_count::wait(key in_key) noexcept {
  // Use C++20 atomic wait guarantees
  epoch()->wait(in_key.m_epoch, std::memory_order_acquire);

  // memory_order_relaxed would suffice for correctness, but the faster
  // #waiters gets to 0, the less likely it is that we'll do spurious wakeups
  // (and thus system calls)
  auto prev = m_val.fetch_add(k_sub_waiter, std::memory_order_seq_cst);

  LF_ASSERT((prev & k_waiter_mask) != 0);
}

template <class Pred>
  requires std::is_invocable_r_v<bool, Pred const &>
void event_count::await(Pred const &condition) {
  //
  if (std::invoke(condition)) {
    return;
  }
// std::invoke(condition) is the only thing that may throw, everything else is
// noexcept, so we can hoist the try/catch block outside of the loop
#if LF_COMPILER_EXCEPTIONS
  try {
#endif
    for (;;) {
      auto my_key = prepare_wait();
      if (std::invoke(condition)) {
        cancel_wait();
        break;
      }
      wait(my_key);
    }
#if LF_COMPILER_EXCEPTIONS
  } catch (...) {
    cancel_wait();
    throw;
  }
#endif
}

} // namespace lf

#ifndef C9703881_3D9C_41A5_A7A2_44615C4CFA6A
#define C9703881_3D9C_41A5_A7A2_44615C4CFA6A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

 // Only for ASSERT macro + hardware_destructive_interference_size

/**
 * @file queue.hpp
 *
 * @brief A stand-alone, production-quality implementation of the Chase-Lev lock-free
 * single-producer multiple-consumer queue.
 *
 * \rst
 *
 * Implements the queue described in the papers, `"Dynamic Circular Work-Stealing Queue"
 * <https://doi.org/10.1145/1073970.1073974>`_ and `"Correct and Efficient Work-Stealing for Weak
 * Memory Models" <https://doi.org/10.1145/2442516.2442524>`_. Both are available in `reference/
 * <https://github.com/ConorWilliams/libfork/tree/main/reference>`_.
 *
 * \endrst
 */

namespace lf {

/**
 * @brief A concept that verifies a type is suitable for use in a queue.
 */
template <typename T>
concept simple = std::is_default_constructible_v<T> && std::is_trivially_copyable_v<T> && std::atomic<T>::is_always_lock_free;

namespace detail {

/**
 * @brief A basic wrapper around a c-style array that provides modulo load/stores.
 *
 * This class is designed for internal use only. It provides a c-style API that is
 * used efficiently by queue for low level atomic operations.
 *
 * @tparam T The type of the elements in the array.
 */
template <simple T>
struct ring_buf {
  /**
   * @brief Construct a new ring buff object
   *
   * @param cap The capacity of the buffer, MUST be a power of 2.
   */
  explicit ring_buf(std::ptrdiff_t cap) : m_cap{cap}, m_mask{cap - 1} {
    LF_ASSERT(cap > 0 && std::has_single_bit(static_cast<std::size_t>(cap)));
  }
  /**
   * @brief Get the capacity of the buffer.
   */
  [[nodiscard]] auto capacity() const noexcept -> std::ptrdiff_t { return m_cap; }
  /**
   * @brief Store ``val`` at ``index % this->capacity()``.
   */
  auto store(std::ptrdiff_t index, T const &val) noexcept -> void {
    LF_ASSERT(index >= 0);
    (m_buf.get() + (index & m_mask))->store(val, std::memory_order_relaxed); // NOLINT Avoid cast to std::size_t.
  }
  /**
   * @brief Load value at ``index % this->capacity()``.
   */
  [[nodiscard]] auto load(std::ptrdiff_t index) const noexcept -> T {
    LF_ASSERT(index >= 0);
    return (m_buf.get() + (index & m_mask))->load(std::memory_order_relaxed); // NOLINT Avoid cast to std::size_t.
  }
  /**
   * @brief Copies elements in range ``[bottom, top)`` into a new ring buffer.
   *
   * This function allocates a new buffer and returns a pointer to it.
   * The caller is responsible for deallocating the memory.
   *
   * @param bottom The bottom of the range to copy from (inclusive).
   * @param top The top of the range to copy from (exclusive).
   */
  auto resize(std::ptrdiff_t bottom, std::ptrdiff_t top) const -> ring_buf<T> * { // NOLINT
    auto *ptr = new ring_buf{2 * m_cap};                                          // NOLINT
    for (std::ptrdiff_t i = top; i != bottom; ++i) {
      ptr->store(i, load(i));
    }
    return ptr;
  }

private:
  using array_t = std::atomic<T>[]; // NOLINT

  std::ptrdiff_t m_cap;  ///< Capacity of the buffer
  std::ptrdiff_t m_mask; ///< Bit mask to perform modulo capacity operations

#ifdef __cpp_lib_smart_ptr_for_overwrite
  std::unique_ptr<array_t> m_buf = std::make_unique_for_overwrite<array_t>(static_cast<std::size_t>(m_cap));
#else
  std::unique_ptr<array_t> m_buf = std::make_unique<array_t>(static_cast<std::size_t>(m_cap));
#endif
};

} // namespace detail

/**
 * @brief Error codes for ``queue`` 's ``steal()`` operation.
 */
enum class err : int {
  none = 0, ///< The ``steal()`` operation succeeded.
  lost,     ///< Lost the ``steal()`` race hence, the ``steal()`` operation failed.
  empty,    ///< The queue is empty and hence, the ``steal()`` operation failed.
};

/**
 * @brief An unbounded lock-free single-producer multiple-consumer queue.
 *
 * Only the queue owner can perform ``pop()`` and ``push()`` operations where the queue behaves
 * like a LIFO stack. Others can (only) ``steal()`` data from the queue, they see a FIFO queue.
 * All threads must have finished using the queue before it is destructed.
 *
 * \rst
 *
 * Example:
 *
 * .. include:: ../../test/source/queue.cpp
 *    :code:
 *    :start-after: // !BEGIN-EXAMPLE
 *    :end-before: // !END-EXAMPLE
 *
 * \endrst
 *
 * @tparam T The type of the elements in the queue - must be a simple type.
 */
template <simple T>
class queue : detail::immovable {
  static constexpr std::ptrdiff_t k_default_capacity = 1024;
  static constexpr std::size_t k_garbage_reserve = 32;

public:
  /**
   * @brief The type of the elements in the queue.
   */
  using value_type = T;
  /**
   * @brief Construct a new empty queue object.
   */
  queue() : queue(k_default_capacity) {}
  /**
   * @brief Construct a new empty queue object.
   *
   * @param cap The capacity of the queue (must be a power of 2).
   */
  explicit queue(std::ptrdiff_t cap);
  /**
   * @brief Get the number of elements in the queue.
   */
  [[nodiscard]] auto size() const noexcept -> std::size_t;
  /**
   * @brief Get the number of elements in the queue as a signed integer.
   */
  [[nodiscard]] auto ssize() const noexcept -> ptrdiff_t;
  /**
   * @brief Get the capacity of the queue.
   */
  [[nodiscard]] auto capacity() const noexcept -> ptrdiff_t;
  /**
   * @brief Check if the queue is empty.
   */
  [[nodiscard]] auto empty() const noexcept -> bool;
  /**
   * @brief Push an item into the queue.
   *
   * Only the owner thread can insert an item into the queue.
   * This operation can trigger the queue to resize if more space is required.
   *
   * @param val Value to add to the queue.
   */
  auto push(T const &val) noexcept -> void;
  /**
   * @brief Pop an item from the queue.
   *
   * Only the owner thread can pop out an item from the queue.
   *
   * @return ``std::nullopt`` if  this operation fails (i.e. the queue is empty).
   */
  auto pop() noexcept -> std::optional<T>;
  /**
   * @brief The return type of the ``steal()`` operation.
   *
   * This type is suitable for structured bindings. We return a custom type instead of a
   * ``std::optional`` to allow for more information to be returned as to why a steal may fail.
   */
  struct steal_t {
    /**
     * @brief Check if the operation succeeded.
     */
    constexpr explicit operator bool() const noexcept { return code == err::none; }
    /**
     * @brief Get the value like ``std::optional``.
     *
     * Requires ``code == err::none`` .
     */
    constexpr auto operator*() noexcept -> T & {
      LF_ASSERT(code == err::none);
      return val;
    }
    /**
     * @brief Get the value like ``std::optional``.
     *
     * Requires ``code == err::none`` .
     */
    constexpr auto operator*() const noexcept -> T const & {
      LF_ASSERT(code == err::none);
      return val;
    }
    /**
     * @brief Get the value ``like std::optional``.
     *
     * Requires ``code == err::none`` .
     */
    constexpr auto operator->() noexcept -> T * {
      LF_ASSERT(code == err::none);
      return std::addressof(val);
    }
    /**
     * @brief Get the value ``like std::optional``.
     *
     * Requires ``code == err::none`` .
     */
    constexpr auto operator->() const noexcept -> T const * {
      LF_ASSERT(code == err::none);
      return std::addressof(val);
    }

    err code; ///< The error code of the ``steal()`` operation.
    T val;    ///< The value stolen from the queue, Only valid if ``code == err::stolen``.
  };

  /**
   * @brief Steal an item from the queue.
   *
   * Any threads can try to steal an item from the queue. This operation can fail if the queue is
   * empty or if another thread simultaneously stole an item from the queue.
   */
  auto steal() noexcept -> steal_t;
  /**
   * @brief Destroy the queue object.
   *
   * All threads must have finished using the queue before it is destructed.
   */
  ~queue() noexcept;

private:
  alignas(detail::k_cache_line) std::atomic<std::ptrdiff_t> m_top;
  alignas(detail::k_cache_line) std::atomic<std::ptrdiff_t> m_bottom;
  alignas(detail::k_cache_line) std::atomic<detail::ring_buf<T> *> m_buf;

  alignas(detail::k_cache_line) std::vector<std::unique_ptr<detail::ring_buf<T>>> m_garbage; // Store old buffers here.

  // Convenience aliases.
  static constexpr std::memory_order relaxed = std::memory_order_relaxed;
  static constexpr std::memory_order consume = std::memory_order_consume;
  static constexpr std::memory_order acquire = std::memory_order_acquire;
  static constexpr std::memory_order release = std::memory_order_release;
  static constexpr std::memory_order seq_cst = std::memory_order_seq_cst;
};

template <simple T>
queue<T>::queue(std::ptrdiff_t cap) : m_top(0), m_bottom(0), m_buf(new detail::ring_buf<T>{cap}) {
  m_garbage.reserve(k_garbage_reserve);
}

template <simple T>
auto queue<T>::size() const noexcept -> std::size_t {
  return static_cast<std::size_t>(ssize());
}

template <simple T>
auto queue<T>::ssize() const noexcept -> std::ptrdiff_t {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return std::max(bottom - top, ptrdiff_t{0});
}

template <simple T>
auto queue<T>::capacity() const noexcept -> ptrdiff_t {
  return m_buf.load(relaxed)->capacity();
}

template <simple T>
auto queue<T>::empty() const noexcept -> bool {
  ptrdiff_t const bottom = m_bottom.load(relaxed);
  ptrdiff_t const top = m_top.load(relaxed);
  return top >= bottom;
}

template <simple T>
auto queue<T>::push(T const &val) noexcept -> void {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed);
  std::ptrdiff_t const top = m_top.load(acquire);
  detail::ring_buf<T> *buf = m_buf.load(relaxed);

  if (buf->capacity() < (bottom - top) + 1) {
    // Queue is full, build a new one.
    m_garbage.emplace_back(std::exchange(buf, buf->resize(bottom, top)));
    m_buf.store(buf, relaxed);
  }

  // Construct new object, this does not have to be atomic as no one can steal this item until
  // after we store the new value of bottom, ordering is maintained by surrounding atomics.
  buf->store(bottom, val);

  std::atomic_thread_fence(release);
  m_bottom.store(bottom + 1, relaxed);
}

template <simple T>
auto queue<T>::pop() noexcept -> std::optional<T> {
  std::ptrdiff_t const bottom = m_bottom.load(relaxed) - 1;
  detail::ring_buf<T> *buf = m_buf.load(relaxed);

  m_bottom.store(bottom, relaxed); // Stealers can no longer steal.

  std::atomic_thread_fence(seq_cst);
  std::ptrdiff_t top = m_top.load(relaxed);

  if (top <= bottom) {
    // Non-empty queue
    if (top == bottom) {
      // The last item could get stolen, by a stealer that loaded bottom before our write above.
      if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
        // Failed race, thief got the last item.
        m_bottom.store(bottom + 1, relaxed);
        return std::nullopt;
      }
      m_bottom.store(bottom + 1, relaxed);
    }
    // Can delay load until after acquiring slot as only this thread can push(),
    // This load is not required to be atomic as we are the exclusive writer.
    return buf->load(bottom);
  }
  m_bottom.store(bottom + 1, relaxed);
  return std::nullopt;
}

template <simple T>
auto queue<T>::steal() noexcept -> steal_t {
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

    static_assert(std::is_trivially_destructible_v<T>, "concept 'simple' should guarantee this already");

    if (!m_top.compare_exchange_strong(top, top + 1, seq_cst, relaxed)) {
      return {.code = err::lost, .val = {}};
    }
    return {.code = err::none, .val = tmp};
  }
  return {.code = err::empty, .val = {}};
}

template <simple T>
queue<T>::~queue() noexcept {
  delete m_buf.load(); // NOLINT
}

} // namespace lf

#endif /* C9703881_3D9C_41A5_A7A2_44615C4CFA6A */

#ifndef CA0BE1EA_88CD_4E63_9D89_37395E859565
#define CA0BE1EA_88CD_4E63_9D89_37395E859565

// The code in this file is adapted from the original implementation:
// http://prng.di.unimi.it/xoshiro256starstar.c

// Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

// To the extent possible under law, the author has dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.

// See <http://creativecommons.org/publicdomain/zero/1.0/>.

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>



/**
 * \file random.hpp
 *
 * @brief Pseudo random number generators (PRNG).
 */

namespace lf {

/**
 * @brief A \<random\> compatible implementation of the xoshiro256** 1.0 PRNG
 *
 * \rst
 *
 * From `the original <https://prng.di.unimi.it/>`_:
 *
 * This is xoshiro256** 1.0, one of our all-purpose, rock-solid generators. It has excellent
 * (sub-ns) speed, a state (256 bits) that is large enough for any parallel application, and it
 * passes all tests we are aware of.
 *
 * \endrst
 */
class xoshiro {
public:
  using result_type = std::uint64_t; ///< Required by named requirement: UniformRandomBitGenerator

  /**
   * @brief Construct and seed the PRNG with the default seed.
   */
  constexpr xoshiro() noexcept : xoshiro(default_seed) {}

  /**
   * @brief Construct and seed the PRNG from ``std::random_device``.
   */
  explicit xoshiro(std::random_device &device) : xoshiro({
                                                     random_bits(device),
                                                     random_bits(device),
                                                     random_bits(device),
                                                     random_bits(device),
                                                 }) {}

  /**
   * @brief Construct and seed the PRNG from a ``std::random_device``.
   */
  explicit xoshiro(std::random_device &&device) : xoshiro(device) {}

  /**
   * @brief Construct and seed the PRNG.
   *
   * @param seed The PRNG's seed, must not be everywhere zero.
   */
  explicit constexpr xoshiro(std::array<result_type, 4> const &seed) : m_state{seed} {
    if (seed == std::array<result_type, 4>{0, 0, 0, 0}) {
      LF_ASSERT(false);
    }
  }

  /**
   * @brief Get the minimum value of the generator.
   *
   * @return The minimum value that ``xoshiro::operator()`` can return.
   */
  static constexpr auto min() noexcept -> result_type { return std::numeric_limits<result_type>::lowest(); }

  /**
   * @brief Get the maximum value of the generator.
   *
   * @return The maximum value that ``xoshiro::operator()`` can return.
   */
  static constexpr auto max() noexcept -> result_type { return std::numeric_limits<result_type>::max(); }

  /**
   * @brief Generate a random bit sequence and advance the state of the generator.
   *
   * @return A pseudo-random number.
   */
  constexpr auto operator()() noexcept -> result_type {
    result_type const result = rotl(m_state[1] * 5, 7) * 9;

    result_type const temp = m_state[1] << 17; // NOLINT

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= temp;

    m_state[3] = rotl(m_state[3], 45); // NOLINT (magic-numbers)

    return result;
  }

  /**
   * @brief This is the jump function for the generator.
   *
   * It is equivalent to 2^128 calls to operator(); it can be used to generate 2^128 non-overlapping
   * sub-sequences for parallel computations.
   */
  constexpr auto jump() noexcept -> void {
    // NOLINTNEXTLINE (magic-numbers)
    jump_impl({0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c});
  }

  /**
   * @brief This is the long-jump function for the generator.
   *
   * It is equivalent to 2^192 calls to operator(); it can be used to generate 2^64 starting points,
   * from each of which jump() will generate 2^64 non-overlapping sub-sequences for parallel
   * distributed computations.
   */
  constexpr auto long_jump() noexcept -> void {
    // NOLINTNEXTLINE (magic-numbers)
    jump_impl({0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635});
  }

private:
  /**
   * @brief The default seed for the PRNG.
   */
  static constexpr std::array<result_type, 4> default_seed = {
      0x8D0B73B52EA17D89,
      0x2AA426A407C2B04F,
      0xF513614E4798928A,
      0xA65E479EC5B49D41,
  };

  std::array<result_type, 4> m_state;

  /**
   * @brief Utility function.
   */
  static constexpr auto rotl(result_type const val, int const bits) noexcept -> result_type {
    return (val << bits) | (val >> (64 - bits)); // NOLINT
  }

  /**
   * @brief Utility function to upscale random::device result_type to xoshiro's result_type.
   */
  static auto random_bits(std::random_device &device) -> result_type {
    //
    constexpr auto chars_in_rd = sizeof(std::random_device::result_type);

    static_assert(sizeof(result_type) % chars_in_rd == 0);

    result_type bits = 0;

    for (std::size_t i = 0; i < sizeof(result_type) / chars_in_rd; i++) {
      bits <<= CHAR_BIT * chars_in_rd;
      bits += device();
    }

    return bits;
  }

  constexpr void jump_impl(std::array<result_type, 4> const &jump_array) noexcept {
    //
    std::array<result_type, 4> s = {0, 0, 0, 0}; // NOLINT

    for (result_type const jump : jump_array) {
      for (int bit = 0; bit < 64; ++bit) {  // NOLINT
        if (jump & result_type{1} << bit) { // NOLINT
          s[0] ^= m_state[0];
          s[1] ^= m_state[1];
          s[2] ^= m_state[2];
          s[3] ^= m_state[3];
        }
        operator()();
      }
    }
    m_state = s;
  }
};

} // namespace lf

#endif /* CA0BE1EA_88CD_4E63_9D89_37395E859565 */

#ifndef F4C3CE1A_F0F7_485D_8D54_473CCE8294DC
#define F4C3CE1A_F0F7_485D_8D54_473CCE8294DC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <stdexcept>



/**
 * @file thread_local.hpp
 *
 * @brief Provides a utility class for managing static inline thread_local pointer to an object.
 */

namespace lf {

/**
 * @brief Store a thread_local static pointer to a T object.
 *
 * This is useful for implementing the ``context()`` method of a class satisfying ``lf::thread_context``.
 */
template <typename T>
class thread_local_ptr {
public:
  /**
   * @brief A runtime-error thrown when get() is called before set().
   */
  struct not_set : std::runtime_error {
    not_set() : std::runtime_error("Thread's pointer is not set!") {}
  };

  /**
   * @brief Get the object pointed to by the thread_local pointer.
   */
  [[nodiscard]]
#ifdef __clang__
  __attribute((noinline)) ///< Workaround for LLVM coroutine TLS bug.
#endif
  static auto
  get() -> T & {
    if (m_ptr == nullptr) {
#if LF_COMPILER_EXCEPTIONS
      throw not_set{};
#else
      std::terminate();
#endif
    }
    return *m_ptr;
  }

  /**
   * @brief Set the thread_local pointer to the given object.
   */
  static auto set(T &ctx) noexcept -> void { m_ptr = std::addressof(ctx); }

private:
  static inline thread_local constinit T *m_ptr = nullptr; // NOLINT
};

} // namespace lf

#endif /* F4C3CE1A_F0F7_485D_8D54_473CCE8294DC */


/**
 * @file busy.hpp
 *
 * @brief A work-stealing threadpool where all the threads spin when idle.
 */

namespace lf {

namespace detail {

template <simple T, std::size_t N>
  requires(std::has_single_bit(N))
class buffered_queue {
public:
  [[nodiscard]] auto empty() const noexcept -> bool { return m_top == m_bottom; }

  // See what would be popped
  [[nodiscard]] auto peek() -> T & {
    LF_ASSERT(!empty());
    return load(m_bottom - 1);
  }

  auto push(T const &val) noexcept -> void {
    if (buff_full()) {
      m_queue.push(load(m_top++));
    }
    store(m_bottom++, val);
  }

  auto pop() noexcept -> std::optional<T> {

    if (empty()) {
      return std::nullopt;
    }

    bool was_full = buff_full();

    T val = load(--m_bottom);

    if (was_full) {
      if (auto opt = m_queue.pop()) {
        store(--m_top, *opt);
      }
    }

    return val;
  }

  auto steal() noexcept -> typename queue<T>::steal_t { return m_queue.steal(); }

private:
  [[nodiscard]] auto buff_full() const noexcept -> bool { return m_bottom - m_top == N; }

  auto store(std::size_t index, T const &val) noexcept -> void {
    m_buff[index & mask] = val; // NOLINT
  }

  [[nodiscard]] auto load(std::size_t index) noexcept -> T & {
    return m_buff[(index & mask)]; // NOLINT
  }

  static constexpr std::size_t mask = N - 1;

  std::size_t m_top = 0;
  std::size_t m_bottom = 0;
  std::array<T, N> m_buff;
  queue<T> m_queue;
};

// template <is_virtual_stack Stack, typename Steal>
//   requires requires(Steal steal) {
//     { std::invoke(steal) } -> std::convertible_to<std::optional<typename Stack::handle>>;
//   }
// class stack_controller {
// public:
//   using handle_t = typename Stack::handle;

//   explicit constexpr stack_controller(Steal const &steal) : m_steal{steal} {}

//   auto stack_top() -> handle_t {
//     return m_stacks.peek();
//   }

//   void stack_pop() {
//     LF_LOG("stack_pop()");

//     LF_ASSERT(!m_stacks.empty());

//     m_stacks.pop();

//     if (!m_stacks.empty()) {
//       return;
//     }

//     LF_LOG("No stack, stealing from other threads");

//     if (std::optional handle = std::invoke(m_steal)) {
//       LF_ASSERT(m_stacks.empty());
//       m_stacks.push(*handle);
//     }

//     LF_LOG("No stacks found, allocating new stacks");
//     alloc_stacks();
//   }

//   void stack_push(handle_t handle) {
//     LF_LOG("Pushing stack to private queue");
//     LF_ASSERT(stack_top()->empty());
//     m_stacks.push(handle);
//   }

// private:
//   using stack_block = typename Stack::unique_arr_ptr_t;

//   static constexpr std::size_t k_buff = 16;

//   Steal m_steal;
//   buffered_queue<typename Stack::handle, k_buff> m_stacks;
//   std::vector<stack_block> m_stack_storage;

//   void alloc_stacks() {

//     LF_ASSERT(m_stacks.empty());

//     stack_block stacks = Stack::make_unique(k_buff);

//     for (std::size_t i = 0; i < k_buff; ++i) {
//       m_stacks.push(handle_t{stacks.get() + i});
//     }

//     m_stack_storage.push_back(std::move(stacks));
//   }
// };

} // namespace detail

/**
 * @brief A scheduler based on a traditional work-stealing thread pool.
 *
 * Worker threads continuously try to steal tasks from other worker threads hence, they
 * waste CPU cycles if sufficient work is not available. This is a good choice if the number
 * of threads is equal to the number of hardware cores and the multiplexer has no other load.
 */
class busy_pool : detail::immovable {
public:
  /**
   * @brief The context type for the busy_pools threads.
   */
  class context_type : thread_local_ptr<context_type> {
  public:
    using stack_type = virtual_stack<detail::mebibyte>;

    context_type() { alloc_stacks(); }

    static auto context() -> context_type & { return get(); }

    auto max_threads() const noexcept -> std::size_t { return m_pool->m_workers.size(); }

    auto stack_top() -> stack_type::handle {
      LF_ASSERT(&context() == this);
      return m_stacks.peek();
    }

    void stack_pop() {
      LF_ASSERT(&context() == this);
      LF_ASSERT(!m_stacks.empty());

      LF_LOG("Pop stack");

      m_stacks.pop();

      if (!m_stacks.empty()) {
        return;
      }

      LF_LOG("No stack, stealing from other threads");

      auto n = max_threads();

      std::uniform_int_distribution<std::size_t> dist(0, n - 1);

      for (std::size_t attempts = 0; attempts < 2 * n;) {

        auto steal_at = dist(m_rng);

        if (steal_at == m_id) {
          continue;
        }

        if (auto handle = m_pool->m_contexts[steal_at].m_stacks.steal()) {
          LF_LOG("Stole stack from thread {}", steal_at);
          LF_ASSERT(m_stacks.empty());
          m_stacks.push(*handle);
          return;
        }

        ++attempts;
      }

      LF_LOG("No stacks found, allocating new stacks");
      alloc_stacks();
    }

    void stack_push(stack_type::handle handle) {
      LF_ASSERT(&context() == this);
      LF_ASSERT(stack_top()->empty());

      LF_LOG("Pushing stack to private queue");

      m_stacks.push(handle);
    }

    auto task_steal() -> typename queue<task_handle>::steal_t {
      return m_tasks.steal();
    }

    auto task_pop() -> std::optional<task_handle> {
      LF_ASSERT(&context() == this);
      return m_tasks.pop();
    }

    void task_push(task_handle task) {
      LF_ASSERT(&context() == this);
      m_tasks.push(task);
    }

  private:
    static constexpr std::size_t block_size = 16;

    using stack_block = typename stack_type::unique_arr_ptr_t;
    using stack_handle = typename stack_type::handle;

    friend class busy_pool;

    busy_pool *m_pool = nullptr; ///< To the pool this context belongs to.
    std::size_t m_id = 0;        ///< Index in the pool.
    xoshiro m_rng;               ///< Our personal PRNG.

    detail::buffered_queue<stack_handle, block_size> m_stacks; ///< Our (stealable) stack queue.
    queue<task_handle> m_tasks;                                ///< Our (stealable) task queue.

    std::vector<stack_block> m_stack_storage; ///< Controls ownership of the stacks.

    // Alloc k new stacks and insert them into our private queue.
    void alloc_stacks() {

      LF_ASSERT(m_stacks.empty());

      stack_block stacks = stack_type::make_unique(block_size);

      for (std::size_t i = 0; i < block_size; ++i) {
        m_stacks.push(stack_handle{stacks.get() + i});
      }

      m_stack_storage.push_back(std::move(stacks));
    }
  };

  static_assert(thread_context<context_type>);

  /**
   * @brief Construct a new busy_pool object.
   *
   * @param n The number of worker threads to create, defaults to the number of hardware threads.
   */
  explicit busy_pool(std::size_t n = std::thread::hardware_concurrency()) : m_contexts(n) {
    // Initialize the random number generator.
    xoshiro rng(std::random_device{});

    std::size_t count = 0;

    for (auto &ctx : m_contexts) {
      ctx.m_pool = this;
      ctx.m_rng = rng;
      rng.long_jump();
      ctx.m_id = count++;
    }

#if LF_COMPILER_EXCEPTIONS
    try {
#endif
      for (std::size_t i = 0; i < n; ++i) {
        m_workers.emplace_back([this, i]() {
          // Get a reference to the threads context.
          context_type &my_context = m_contexts[i];

          // Set the thread local context.
          context_type::set(my_context);

          std::uniform_int_distribution<std::size_t> dist(0, m_contexts.size() - 1);

          while (!m_stop_requested.test(std::memory_order_acquire)) {

            for (int attempt = 0; attempt < k_steal_attempts; ++attempt) {

              std::size_t steal_at = dist(my_context.m_rng);

              if (steal_at == i) {
                if (auto root = m_submit.steal()) {
                  LF_LOG("resuming root task");
                  root->resume();
                }
              } else if (auto work = m_contexts[steal_at].task_steal()) {
                attempt = 0;
                LF_LOG("Stole work from {}", steal_at);
                work->resume();
                LF_LOG("worker resumes thieving");
                LF_ASSUME(my_context.m_tasks.empty());
              }
            }
          };

          LF_LOG("Worker {} stopping", i);
        });
      }
#if LF_COMPILER_EXCEPTIONS
    } catch (...) {
      // Need to stop the threads
      clean_up();
      throw;
    }
#endif
  }

  /**
   * @brief Schedule a task for execution.
   */
  auto schedule(stdx::coroutine_handle<> root) noexcept {
    m_submit.push(root);
  }

  ~busy_pool() noexcept { clean_up(); }

private:
  // Request all threads to stop, wake them up and then call join.
  auto clean_up() noexcept -> void {

    LF_LOG("Request stop");

    LF_ASSERT(m_submit.empty());

    // Set conditions for workers to stop
    m_stop_requested.test_and_set(std::memory_order_release);

    // Join workers
    for (auto &worker : m_workers) {
      LF_ASSUME(worker.joinable());
      worker.join();
    }
  }

  static constexpr int k_steal_attempts = 1024;

  queue<stdx::coroutine_handle<>> m_submit;

  std::atomic_flag m_stop_requested = ATOMIC_FLAG_INIT;

  std::vector<context_type> m_contexts;
  std::vector<std::thread> m_workers; // After m_context so threads are destroyed before the queues.
};

static_assert(scheduler<busy_pool>);

} // namespace lf

#endif /* B5AE1829_6F8A_4118_AB15_FE73F851271F */

#ifndef C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC
#define C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// Self Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>




/**
 * @file inline.hpp
 *
 * @brief A scheduler that runs all tasks inline on current thread.
 */

namespace lf {

/**
 * @brief A scheduler that runs all tasks inline on current thread.
 */
class inline_scheduler {
public:
  /**
   * @brief The context type for the scheduler.
   */
  class context_type : thread_local_ptr<context_type> {

  public:
    /**
     * @brief The stack type for the scheduler.
     */
    using stack_type = virtual_stack<detail::mebibyte>;
    /**
     * @brief Construct a new context type object, set the thread_local context object to this object.
     */
    context_type() noexcept {
      inline_scheduler::context_type::set(*this);
    }
    /**
     * @brief Get the thread_local context object.
     */
    static auto context() -> context_type & { return context_type::get(); }
    /**
     * @brief Returns one as this runs all tasks inline.
     */
    static constexpr auto max_threads() noexcept -> std::size_t { return 1; }
    /**
     * @brief Get the top stack object.
     */
    auto stack_top() -> stack_type::handle { return stack_type::handle{m_stack.get()}; }
    /**
     * @brief Should never be called, aborts the program.
     */
    static void stack_pop() { LF_ASSERT(false); }
    /**
     * @brief Should never be called, aborts the program.
     */
    static void stack_push([[maybe_unused]] stack_type::handle handle) { LF_ASSERT(false); }
    /**
     * @brief Pops a task from the task queue.
     */
    auto task_pop() -> std::optional<task_handle> {
      if (m_tasks.empty()) {
        return std::nullopt;
      }
      task_handle task = m_tasks.back();
      m_tasks.pop_back();
      return task;
    }
    /**
     * @brief Pushes a task to the task queue.
     */
    void task_push(task_handle task) {
      m_tasks.push_back(task);
    }

  private:
    std::vector<task_handle> m_tasks;
    typename stack_type::unique_ptr_t m_stack = stack_type::make_unique();
  };

  /**
   * @brief Immediately resume the root task.
   */
  static void schedule(stdx::coroutine_handle<> root_task) {
    root_task.resume();
  }

private:
  context_type m_context;
};

static_assert(scheduler<inline_scheduler>);

} // namespace lf

#endif /* C8EE9A0A_3B9F_4FFE_8FF5_910645E0C7CC */


#endif /* EDCA974A_808F_4B62_95D5_4D84E31B8911 */
