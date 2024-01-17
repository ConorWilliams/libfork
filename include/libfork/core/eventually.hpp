#ifndef B7972761_4CBF_4B86_B195_F754295372BF
#define B7972761_4CBF_4B86_B195_F754295372BF

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <concepts>    // for constructible_from
#include <exception>   // for exception_ptr, current_exce...
#include <memory>      // for destroy_at, construct_at
#include <type_traits> // for add_lvalue_reference_t, add...
#include <utility>     // for addressof, forward

#include "libfork/core/impl/utility.hpp" // for new_empty, else_empty_t
#include "libfork/core/macro.hpp"        // for LF_ASSERT, unreachable
#include "libfork/core/task.hpp"         // for returnable

/**
 * @file eventually.hpp
 *
 * @brief Classes for delaying construction of an object.
 */

namespace lf {

namespace impl {

namespace detail {

/**
 * @brief Base case -> T
 */
template <returnable T>
struct eventually_value : std::type_identity<T> {};

/**
 * @brief void specialization -> empty
 */
template <>
struct eventually_value<void> : std::type_identity<new_empty<>> {};

/**
 * @brief Reference specialization -> remove_reference<T> *
 */
template <returnable T>
  requires std::is_reference_v<T>
struct eventually_value<T> : std::add_pointer<T> {};

} // namespace detail

/**
 * @brief Return the appropriate type to store in an eventually.
 *
 * If `T` is `void` then we store an empty object.
 * If `T` is a reference then we store a pointer to the referenced type.
 * Otherwise we store the type directly.
 */
template <returnable T>
using eventually_value_t = typename detail::eventually_value<T>::type;

} // namespace impl

inline namespace core {

// ------------------------------------------------------------------------ //

/**
 * @brief A wrapper to delay construction of an object.
 *
 * An eventually is either empty, contains an object of type `T` or, (if `Exception` is true) contains an
 * exception. Assignment to an empty eventually will construct an object of type `T` inside the eventually.
 */
template <returnable T, bool Exception>
  requires impl::non_void<T> || Exception
class basic_eventually : impl::immovable<basic_eventually<T, Exception>> {

  /**
   *         | void                 | ref                        | val
   * eptr    | empty or exception * | ref or empty or exception  | val or empty or exception
   * no eptr | invalid              | ref or empty *             | val or empty
   *
   *
   * If two-states (*) then we can omit the state member.
   */

  static constexpr bool is_void = std::is_void_v<T>;
  static constexpr bool is_ref_value = !is_void && std::is_reference_v<T>;
  static constexpr bool is_val_value = !is_void && !std::is_reference_v<T>;

  /**
   * @brief If implicit_state is true then we store state bit in exception_ptr or reference ptr.
   */
  static constexpr bool implicit_state = (is_ref_value && !Exception) || (is_void && Exception);

  enum class state : char {
    empty,     ///< No object has been constructed.
    value,     ///< An object has been constructed.
    exception, ///< An exception has been thrown during and is stored.
  };

  [[no_unique_address]] union {
    [[no_unique_address]] impl::new_empty<> m_empty;
    [[no_unique_address]] impl::eventually_value_t<T> m_value;
    [[no_unique_address]] impl::else_empty_t<Exception, std::exception_ptr> m_exception;
  };

  [[no_unique_address]] impl::else_empty_t<!implicit_state, state> m_flag;

  // ----------------------- Hidden friends ----------------------- //

  /**
   * @brief Store the current exception, ``dest.empty()`` must be true.
   *
   * After this function is called, ``has_exception()`` will be true.
   */
  friend auto stash_exception(basic_eventually &dest) noexcept -> void
    requires Exception
  {
    LF_ASSERT(dest.empty());

    std::construct_at(std::addressof(dest.m_exception), std::current_exception());

    if constexpr (!implicit_state) {
      dest.m_flag = state::exception;
    }
  }

 public:
  // ------------------------- Helper ------------------------- //

  /**
   * @brief The type of the object stored in the eventually.
   */
  using value_type = T;

  // ------------------------ Construct ------------------------ //

  // clang-format off

  /**
   * @brief Construct an empty eventually.
   */
  basic_eventually() noexcept requires (implicit_state && Exception) : m_exception{nullptr} {}

  /**
   * @brief Construct an empty eventually.
   */
  basic_eventually() noexcept requires (implicit_state && !Exception) : m_value{nullptr} {}


  /**
   * @brief Construct an empty eventually.
   */
  basic_eventually() noexcept requires (!implicit_state) : m_empty{}, m_flag{state::empty} {}

  // clang-format on

  // ------------------------ Destruct ------------------------ //

  /**
   * @brief Destroy the eventually object and the contained object.
   */
  ~basic_eventually() noexcept {
    if constexpr (implicit_state) {
      if constexpr (Exception) {
        std::destroy_at(std::addressof(m_exception));
      } else {
        // T* is trivially destructible.
      }
    } else {
      switch (m_flag) {
        case state::empty:
          return;
        case state::value:
          if constexpr (!is_void) {
            std::destroy_at(std::addressof(m_value));
            return;
          } else {
            lf::impl::unreachable();
          }
        case state::exception:
          if constexpr (Exception) {
            std::destroy_at(std::addressof(m_exception));
            return;
          } else {
            lf::impl::unreachable();
          }
        default:
          lf::impl::unreachable();
      }
    }
  }
  // ----------------------- Check state ----------------------- //

  /**
   * @brief Check if the eventually is empty.
   */
  [[nodiscard]] auto empty() const noexcept -> bool {
    if constexpr (implicit_state) {
      if constexpr (Exception) {
        return m_exception == nullptr;
      } else {
        return m_value == nullptr;
      }
    } else {
      return m_flag == state::empty;
    }
  }

  /**
   * @brief Check if there is a value stored in the eventually.
   */
  [[nodiscard]] auto has_value() const noexcept -> bool
    requires (is_val_value || is_ref_value)
  {
    if constexpr (implicit_state) {
      return m_value != nullptr;
    } else {
      return m_flag == state::value;
    }
  }

  /**
   * @brief Test is there is an exception stored in the eventually.
   */
  [[nodiscard]] auto has_exception() const noexcept -> bool
    requires Exception
  {
    if constexpr (implicit_state) {
      return m_exception != nullptr;
    } else {
      return m_flag == state::exception;
    }
  }

  // ------------------------ Assignment ------------------------ //

  /**
   * @brief Store a value in the eventually, requires that ``empty()`` is true.
   *
   * After this function is called, ``has_value()`` will be true.
   */
  template <typename U>
    requires (is_val_value && std::constructible_from<T, U>)
  auto operator=(U &&expr) noexcept(std::is_nothrow_constructible_v<T, U>) -> basic_eventually & {
    LF_ASSERT(empty());
    std::construct_at(std::addressof(m_value), std::forward<U>(expr));
    m_flag = state::value;
    return *this;
  }

  // -----------

  /**
   * @brief Store a value in the eventually, requires that ``empty()`` is true.
   *
   * After this function is called, ``has_value()`` will be true.
   */
  template <impl::safe_ref_bind_to<T> U>
    requires (is_ref_value)
  auto operator=(U &&expr) noexcept -> basic_eventually & {

    LF_ASSERT(empty());
    m_value = std::addressof(expr);

    if constexpr (!implicit_state) {
      m_flag = state::value;
    }

    return *this;
  }

  // -------------------- Exception handling -------------------- //

  /**
   * @brief Access the stored exception, ``has_exception()`` must be true.
   */
  [[nodiscard]] auto exception() & noexcept -> std::exception_ptr &
    requires Exception
  {
    LF_ASSERT(has_exception());
    return m_exception;
  }

  /**
   * @brief Access the stored exception, ``has_exception()`` must be true.
   */
  [[nodiscard]] auto exception() const & noexcept -> std::exception_ptr const &
    requires Exception
  {
    LF_ASSERT(has_exception());
    return m_exception;
  }

  /**
   * @brief Access the stored exception, ``has_exception()`` must be true.
   */
  [[nodiscard]] auto exception() && noexcept -> std::exception_ptr &&
    requires Exception
  {
    LF_ASSERT(has_exception());
    return std::move(m_exception);
  }

  /**
   * @brief Access the stored exception, ``has_exception()`` must be true.
   */
  [[nodiscard]] auto exception() const && noexcept -> std::exception_ptr const &&
    requires Exception
  {
    LF_ASSERT(has_exception());
    return std::move(m_exception);
  }

  // ------------------------ Operator -> ------------------------ //

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   */
  [[nodiscard]] auto operator->() noexcept -> std::add_pointer_t<T>
    requires is_val_value
  {
    LF_ASSERT(has_value());
    return std::addressof(m_value);
  }

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   */
  [[nodiscard]] auto operator->() const noexcept -> std::add_pointer_t<T const>
    requires is_val_value
  {
    LF_ASSERT(has_value());
    return std::addressof(m_value);
  }

  // -----------

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   */
  [[nodiscard]] auto operator->() const noexcept -> std::add_pointer_t<T>
    requires is_ref_value
  {
    LF_ASSERT(has_value());
    return m_value;
  }

  // ------------------------ Operator * ------------------------ //

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   */
  [[nodiscard]] auto operator*() & noexcept -> std::add_lvalue_reference_t<T>
    requires is_val_value
  {
    LF_ASSERT(has_value());
    return m_value;
  }

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   */
  [[nodiscard]] auto operator*() const & noexcept -> std::add_lvalue_reference_t<T const>
    requires is_val_value
  {
    LF_ASSERT(has_value());
    return m_value;
  }

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   */
  [[nodiscard]] auto operator*() && noexcept -> std::add_rvalue_reference_t<T>
    requires is_val_value
  {
    LF_ASSERT(has_value());
    return std::move(m_value);
  }

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   */
  [[nodiscard]] auto operator*() const && noexcept -> std::add_rvalue_reference_t<T const>
    requires is_val_value
  {
    LF_ASSERT(has_value());
    return std::move(m_value);
  }

  // -----------

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   *
   * This will decay `T&&` to `T&` just like using a `T &&` reference would.
   */
  [[nodiscard]] auto operator*() const & noexcept -> std::add_lvalue_reference_t<std::remove_reference_t<T>>
    requires is_ref_value
  {
    LF_ASSERT(has_value());
    return *m_value;
  }

  /**
   * @brief Access the stored value, ``has_value()`` must be true.
   *
   * This will not decay T&& to T&, nor will it promote T& to T&&.
   */
  [[nodiscard]] auto operator*() const && noexcept -> T
    requires is_ref_value
  {

    LF_ASSERT(has_value());

    if constexpr (std::is_rvalue_reference_v<T>) {
      return std::move(*m_value);
    } else {
      return *m_value;
    }
  }
};

/**
 * @brief An alias for `lf::core::basic_eventually<T, false>`.
 */
template <returnable T>
using eventually = basic_eventually<T, false>;

/**
 * @brief An alias for `lf::core::basic_eventually<T, true>`.
 */
template <returnable T>
using try_eventually = basic_eventually<T, true>;

} // namespace core

} // namespace lf

#endif /* B7972761_4CBF_4B86_B195_F754295372BF */
