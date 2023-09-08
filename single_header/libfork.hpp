
//---------------------------------------------------------------//
//        This is a machine generated file DO NOT EDIT IT        //
//---------------------------------------------------------------//

#ifndef EDCA974A_808F_4B62_95D5_4D84E31B8911
#define EDCA974A_808F_4B62_95D5_4D84E31B8911
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

#ifndef C5DCA647_8269_46C2_B76F_5FA68738AEDA
#define C5DCA647_8269_46C2_B76F_5FA68738AEDA

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#include <version>

/**
 * @file macro.hpp
 *
 * @brief A collection of internal and public macros.
 */

// NOLINTBEGIN Sometime macros are the only way to do things...

/**
 * @brief The major version of libfork.
 *
 * Changes with incompatible API/ABI changes.
 */
#define LF_VERSION_MAJOR 3
/**
 * @brief The minor version of libfork.
 *
 * Changes when functionality is added in an API/ABI backward compatible manner.
 */
#define LF_VERSION_MINOR 1
/**
 * @brief The patch version of libfork.
 *
 * Changes when bug fixes are made in an API/ABI backward compatible manner.
 */
#define LF_VERSION_PATCH 0

namespace detail {

#define LF_CONCAT_IMPL(x, y) x##y
#define LF_CONCAT(x, y) LF_CONCAT_IMPL(x, y)

} // namespace detail

/**
 * @brief Use with ``inline namespace`` to mangle the major version number into the symbol names.
 *
 */
#define LF_API LF_CONCAT(v, LF_VERSION_MAJOR)

/**
 * @brief Use with ``inline namespace`` to alter the symbols of classes with different ABI in debug/release
 * mode.
 *
 */
#ifndef NDEBUG
  #define LF_DEPENDANT_ABI release
#else
  #define LF_DEPENDANT_ABI debug
#endif

/**
 * @brief Use to decorate lambdas and ``operator()`` (alongside ``LF_STATIC_CONST``) and with ``static`` if
 * supported.
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
#define LF_LIFT(overload_set)                                                                                \
  [](auto &&...args) LF_STATIC_CALL LF_HOF_RETURNS(overload_set(std::forward<decltype(args)>(args)...))

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

#if LF_COMPILER_EXCEPTIONS || defined(LF_DOXYGEN_SHOULD_SKIP_THIS)
  /**
   * @brief Expands to ``try`` if exceptions are enabled, otherwise expands to ``if constexpr (true)``.
   */
  #define LF_TRY try
  /**
   * @brief Expands to ``catch (...)`` if exceptions are enabled, otherwise expands to ``if constexpr
   * (false)``.
   */
  #define LF_CATCH_ALL catch (...)
  /**
   * @brief Expands to ``throw X`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_THROW(X) throw X
  /**
   * @brief Expands to ``throw`` if exceptions are enabled, otherwise terminates the program.
   */
  #define LF_RETHROW throw
#else
  #define LF_TRY if constexpr (true)
  #define LF_CATCH_ALL if constexpr (false)
  #ifndef NDEBUG
    #define LF_THROW(X) assert(false && "Tried to throw: " #X)
  #else
    #define LF_THROW(X) std::terminate()
  #endif
  #ifndef NDEBUG
    #define LF_RETHROW assert(false && "Tried to rethrow without compiler exceptions")
  #else
    #define LF_RETHROW std::terminate()
  #endif
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
  #define LF_ASSUME(expr)                                                                                    \
    if (bool(expr)) {                                                                                        \
    } else {                                                                                                 \
      __builtin_unreachable();                                                                               \
    }
#elif defined(_MSC_VER) || defined(__ICC)
  #define LF_ASSUME(expr) __assume(bool(expr))
#else
  #warning "No LF_ASSUME() implementation for this compiler."
  #define LF_ASSUME(expr)                                                                                    \
    do {                                                                                                     \
    } while (false)
#endif

/**
 * @brief If ``NDEBUG`` is defined then ``LF_ASSERT(expr)`` is  ``LF_ASSUME(expr)`` otherwise
 * ``assert(expr)``.
 */
#ifndef NDEBUG
  #define LF_ASSERT(expr) assert(expr)
#else
  #define LF_ASSERT(expr) LF_ASSUME(expr)
#endif

/**
 * @brief A customizable logging macro.
 *
 * By default this is a no-op. Defining ``LF_LOGGING`` will enable a default
 * logging implementation which prints to ``std::cout``. Overridable by defining your
 * own ``LF_LOG`` macro. API like ``std::format()``.
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

    #define LF_LOG(message, ...)                                                                             \
      do {                                                                                                   \
        if (!std::is_constant_evaluated()) {                                                                 \
          LF_SYNC_COUT << ": " << LF_FORMAT(message __VA_OPT__(, ) __VA_ARGS__) << '\n';                     \
        }                                                                                                    \
      } while (false)
  #else
    #define LF_LOG(head, ...)                                                                                \
      do {                                                                                                   \
      } while (false)
  #endif
#endif

// NOLINTEND

#endif /* C5DCA647_8269_46C2_B76F_5FA68738AEDA */
#ifndef DF63D333_F8C0_4BBA_97E1_32A78466B8B7
#define DF63D333_F8C0_4BBA_97E1_32A78466B8B7

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

/**
 * @file utility.hpp
 *
 * @brief A collection of internal utilities.
 */

namespace lf::detail {

/**
 * @brief The cache line size (bytes) of the current architecture.
 */
#ifdef __cpp_lib_hardware_interference_size
inline constexpr std::size_t k_cache_line = std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t k_cache_line = 64;
#endif

/**
 * @brief The default alignment of `operator new`, a power of two.
 */
inline constexpr std::size_t k_new_align = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

static_assert(std::has_single_bit(k_new_align));

/**
 * @brief Shorthand for `std::numeric_limits<std::unt16_t>::max()`.
 */
static constexpr std::uint16_t k_u16_max = std::numeric_limits<std::uint16_t>::max();

/**
 * @brief Shorthand for `std::numeric_limits<std::int16_t>::max()`.
 */
static constexpr std::int16_t k_i16_max = std::numeric_limits<std::int16_t>::max();

/**
 * @brief Shorthand for `std::numeric_limits<std::unt16_t>::min()`.
 */
static constexpr std::int16_t k_i16_min = std::numeric_limits<std::int16_t>::min();

// /**
//  * @brief Shorthand for `std::numeric_limits<std::uint32_t>::max()`.
//  */
// static constexpr std::uint32_t k_u32_max = std::numeric_limits<std::uint32_t>::max();

inline constexpr std::size_t k_kibibyte = 1024 * 1;          // NOLINT
inline constexpr std::size_t k_mebibyte = 1024 * k_kibibyte; //

/**
 * @brief An empty type that is not copyable or movable.
 *
 * The template parameter prevents multiple empty bases.
 */
template <typename CRTP>
struct immovable {
  immovable() = default;
  immovable(const immovable &) = delete;
  immovable(immovable &&) = delete;
  auto operator=(const immovable &) -> immovable & = delete;
  auto operator=(immovable &&) -> immovable & = delete;
  ~immovable() = default;
};

static_assert(std::is_empty_v<immovable<void>>);

/**
 * @brief An empty type that is only movable.
 */
template <typename CRTP>
struct move_only {
  move_only() = default;
  move_only(const move_only &) = delete;
  move_only(move_only &&) noexcept = default;
  auto operator=(const move_only &) -> move_only & = delete;
  auto operator=(move_only &&) noexcept -> move_only & = default;
  ~move_only() = default;
};

static_assert(std::is_empty_v<immovable<void>>);

/**
 * @brief Invoke a callable with the given arguments, unconditionally noexcept.
 */
template <typename... Args, std::invocable<Args...> Fn>
constexpr auto noexcept_invoke(Fn &&fun, Args &&...args) noexcept -> std::invoke_result_t<Fn, Args...> {
  return std::invoke(std::forward<Fn>(fun), std::forward<Args>(args)...);
}

/**
 * @brief Ensure a type is not const/volatile or ref qualified.
 */
template <typename T>
concept unqualified = std::same_as<std::remove_cvref_t<T>, T>;

namespace detail {

template <typename From, typename To>
struct forward_cv;

template <unqualified From, typename To>
struct forward_cv<From, To> : std::type_identity<To> {};

template <unqualified From, typename To>
struct forward_cv<From const, To> : std::type_identity<To const> {};

template <unqualified From, typename To>
struct forward_cv<From volatile, To> : std::type_identity<To volatile> {};

template <unqualified From, typename To>
struct forward_cv<From const volatile, To> : std::type_identity<To const volatile> {};

} // namespace detail

template <typename From, typename To>
using forward_cv_t = typename detail::forward_cv<From, To>::type;

/**
 * @brief Cast a pointer to a byte pointer.
 */
template <typename T>
auto byte_cast(T *ptr) -> forward_cv_t<T, std::byte> * {
  return std::bit_cast<forward_cv_t<T, std::byte> *>(ptr);
}

/**
 * @brief An empty type.
 */
struct empty {};

} // namespace lf::detail

/**
 * @brief An (unchecked, transparent) annotation to indicate that a pointer is non-null.
 */
template <typename T>
using non_null = T;

/**
 * @brief An (unchecked, transparent) annotation to indicate that a pointer is owning.
 */
template <typename T>
using owner = T;

#endif /* DF63D333_F8C0_4BBA_97E1_32A78466B8B7 */


/**
 * @file stack.hpp
 *
 * @brief Provides an async cactus-stack, control blocks and memory management.
 */

namespace lf {

// ----------------------------------------------- //

namespace detail {

/**
 * @brief A small control structure that a root tasks use to communicate with the main thread.
 */
template <typename T>
struct root_block;

template <>
struct root_block<void> : immovable<root_block<void>> {
  std::binary_semaphore semaphore{0};
};

template <typename T>
struct root_block : root_block<void> {

  std::optional<T> result{};

  template <typename U>
    requires std::constructible_from<std::optional<T>, U>
  constexpr auto operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) -> root_block & {

    LF_LOG("Root task assigns");

    LF_ASSERT(!result.has_value());

    result.emplace(std::forward<U>(expr));

    return *this;
  }
};

// ----------------------------------------------- //

inline namespace LF_DEPENDANT_ABI {

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

struct frame_block; ///< Forward declaration, impl below.

} // namespace LF_DEPENDANT_ABI

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

// ----------------------------------------------- //

inline namespace LF_DEPENDANT_ABI {

/**
 * @brief A small bookkeeping struct allocated immediately before/after each coroutine frame.
 */
struct frame_block : immovable<frame_block>, debug_block {
  /**
   * @brief A lightweight nullable handle to a `frame_block` that contains the public API for thieves.
   */
  struct thief_handle {
    /**
     * @brief Resume a stolen task.
     */
    void resume() const noexcept {
      LF_LOG("Call to resume on stolen task");

      // Link the sentinel to the parent.
      LF_ASSERT(asp);
      asp->write_sentinel_parent(m_frame_block);

      LF_ASSERT(m_frame_block);
      LF_ASSERT(m_frame_block->is_regular()); // Only regular tasks should be stolen.
      m_frame_block->m_steal += 1;
      m_frame_block->get_coro().resume();
    }

    // TODO: make private
    // private:
    frame_block *m_frame_block = nullptr;
  };

  /**
   * @brief For root blocks.
   */
  explicit constexpr frame_block(stdx::coroutine_handle<> coro) noexcept
      : m_prev{0},
        m_coro(offset(coro.address())) {}

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
  explicit constexpr frame_block([[maybe_unused]] empty tag) noexcept : m_prev{0}, m_coro{0} {};

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
    bool was_last;       ///< `true` if the destroyed coroutine was the last on the stack.
  };

  /**
   * @brief Destroy the coroutine on the top of this threads async stack, sets asp.
   */
  static auto pop_asp() -> parent_t {

    frame_block *top = asp;

    // Destroy the coroutine (this does not effect top)
    LF_ASSERT(top->is_regular());
    top->get_coro().destroy();

    asp = std::bit_cast<frame_block *>(top->from_offset(top->m_prev));

    LF_ASSERT(is_aligned(asp, alignof(frame_block)));

    if (asp->is_sentinel()) [[unlikely]] {
      return {asp->read_sentinel_parent(), true};
    }
    return {asp, false};
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

  std::atomic_uint16_t m_join = k_u16_max; ///< Number of children joined (with offset).
  std::uint16_t m_steal = 0;               ///< Number of steals.
  std::int16_t m_prev;                     ///< Distance to previous frame.
  std::int16_t m_coro;                     ///< Offset from `this` to paired coroutine's void handle.

  constexpr auto is_initialised() const noexcept -> bool {
    static_assert(sizeof(frame_block) > uninitialized, "Required for uninitialized to be invalid offset");
    return m_prev != uninitialized && m_coro != uninitialized;
  }

  auto is_sentinel() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_coro == 0;
  }

  auto is_regular() const noexcept -> bool {
    LF_ASSUME(is_initialised());
    return m_prev != 0;
  }

  static constexpr auto is_aligned(void *address, std::size_t align) noexcept -> bool {
    return std::bit_cast<std::uintptr_t>(address) % align == 0;
  }

  /**
   * @brief Compute the offset from `this` to `external`.
   *
   * Satisfies: `external == from_offset(offset(external))`
   */
  [[nodiscard]] constexpr auto offset(void *external) const noexcept -> std::int16_t {
    LF_ASSERT(external);

    std::ptrdiff_t offset = byte_cast(external) - byte_cast(this);

    LF_ASSERT(k_i16_min <= offset && offset <= k_i16_max);  // Check fits in i16.
    LF_ASSERT(offset < 0 || sizeof(frame_block) <= offset); // Check is not internal.

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
  void write_sentinel_parent(frame_block *parent) noexcept {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    new (address) frame_block *{parent};
  }

  /**
   * @brief Read the parent below a sentinel frame.
   */
  auto read_sentinel_parent() noexcept -> frame_block * {
    LF_ASSERT(is_sentinel());
    void *address = from_offset(sizeof(frame_block));
    LF_ASSERT(is_aligned(address, alignof(frame_block *)));
    // Strict aliasing ok as from_offset(...) guarantees external.
    return *std::bit_cast<frame_block **>(address);
  }
};

static_assert(alignof(frame_block) <= k_new_align, "Will be allocated above a coroutine-frame");
static_assert(std::is_trivially_destructible_v<frame_block>);

} // namespace LF_DEPENDANT_ABI

// ----------------------------------------------- //

/**
 * @brief A fraction of a thread's cactus stack.
 */
class alignas(k_new_align) async_stack : immovable<async_stack>, std::array<std::byte, k_mebibyte> {

public:
  /**
   * @brief The size of an `async_stack` (number of bytes it can store).
   */
  static constexpr auto size() noexcept -> std::size_t { return k_mebibyte; }

private:
  static_assert(alignof(frame_block) <= alignof(frame_block *), "As we will allocate sentinel above pointer");

  static constexpr std::size_t sentinel_offset = k_mebibyte - sizeof(frame_block) - sizeof(frame_block *);

public:
  /**
   * @brief Construct a new `async_stack`.
   */
  async_stack() noexcept {
    // Initialize the sentinel with space for a pointer behind it.
    new (static_cast<void *>(data() + sentinel_offset)) frame_block{lf::detail::empty{}};
  }

  /**
   * @brief Get a pointer to the sentinel `frame_block` on the stack.
   */
  auto sentinel() noexcept -> frame_block * {
    return std::launder(std::bit_cast<frame_block *>(data() + sentinel_offset));
  }

  /**
   * @brief Convert a pointer to a stack's sentinel `frame_block` to a pointer to the stack.
   */
  static auto unsafe_from_sentinel(frame_block *sentinel) noexcept -> async_stack * {
    return std::bit_cast<async_stack *>(std::launder(byte_cast(sentinel) - sentinel_offset));
  }
};

static_assert(std::is_standard_layout_v<async_stack>);
static_assert(sizeof(async_stack) == async_stack::size(), "Spurious padding in async_stack!");
static_assert(async_stack::size() >= k_kibibyte, "Stack is too small!");

// ----------------------------------------------- //

inline namespace LF_DEPENDANT_ABI {

class promise_alloc_heap : frame_block {
protected:
  explicit promise_alloc_heap(std::coroutine_handle<> self) noexcept : frame_block{self} { asp = this; }
};

} // namespace LF_DEPENDANT_ABI

/**
 * @brief A base class for promises that allocates on an `async_stack`.
 *
 * When a promise deriving from this class is constructed 'asp' will be set and when it is destroyed 'asp'
 * will be returned to the previous frame.
 */
class promise_alloc_stack {

  // Convert an alignment to a std::uintptr_t, ensure its is a power of two and >= k_new_align.
  constexpr static auto unwrap(std::align_val_t al) noexcept -> std::uintptr_t {
    auto align = static_cast<std::underlying_type_t<std::align_val_t>>(al);
    LF_ASSERT(std::has_single_bit(align));
    LF_ASSERT(align > 0);
    return std::max(align, k_new_align);
  }

protected:
  explicit promise_alloc_stack(std::coroutine_handle<> self) noexcept {
    LF_ASSERT(asp);
    asp->set_coro(self);
  }

public:
  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size) noexcept -> void * {
    return promise_alloc_stack::operator new(size, std::align_val_t{k_new_align});
  }

  /**
   * @brief Allocate a new `frame_block` on the current `async_stack` and enough space for the coroutine
   * frame.
   *
   * This will update `asp` to point to the top of the new async stack.
   */
  [[nodiscard]] static auto operator new(std::size_t size, std::align_val_t al) noexcept -> void * {

    std::uintptr_t align = unwrap(al);

    frame_block *prev_asp = asp;

    LF_ASSERT(prev_asp);

    auto prev_stack_addr = std::bit_cast<std::uintptr_t>(prev_asp);

    std::uintptr_t coro_frame_addr = (prev_stack_addr - size) & ~(align - 1);

    LF_ASSERT(coro_frame_addr % align == 0);

    std::uintptr_t frame_addr = coro_frame_addr - sizeof(frame_block);

    LF_ASSERT(frame_addr % alignof(frame_block) == 0);

    // Starts the lifetime of the new frame block.
    asp = new (std::bit_cast<void *>(frame_addr)) frame_block{prev_asp};

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

} // namespace detail

/**
 * @brief An aliases to the internal type that controls a task.
 *
 * Pointers given/received to this type are all non-owning, the only method
 * implementors of a context are permitted to use is `resume()`.
 */
using task_ptr = detail::frame_block::thief_handle;

} // namespace lf

#endif /* D66428B1_3B80_45ED_A7C2_6368A0903810 */


// #include "libfork/schedule/busy.hpp"
// #include "libfork/schedule/inline.hpp"

#endif /* EDCA974A_808F_4B62_95D5_4D84E31B8911 */
