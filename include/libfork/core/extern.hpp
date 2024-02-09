#ifndef E6A77A20_6653_4B56_9931_BA5B0987911A
#define E6A77A20_6653_4B56_9931_BA5B0987911A

// Copyright © Conor Williams <conorwilliams@outlook.com>

// SPDX-License-Identifier: MPL-2.0

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <type_traits> // for true_type, type_identity, false_type
#include <utility>     // for forward

#include "libfork/core/eventually.hpp" // for eventually, try_eventually
#include "libfork/core/first_arg.hpp"  // for quasi_pointer, first_arg_t, async_function_object
#include "libfork/core/invocable.hpp"  // for discard_t
#include "libfork/core/macro.hpp"      // for LF_STATIC_CONST, LF_STATIC_CALL, LF_CORO_WRAPPER
#include "libfork/core/sync_wait.hpp"  // for future_shared_state_ptr
#include "libfork/core/tag.hpp"        // for tag
#include "libfork/core/task.hpp"       // for returnable, task

/**
 * @file extern.hpp
 *
 * @brief Machinery for forward declaring async functions.
 */

namespace lf::impl {

namespace detail {

template <quasi_pointer I, returnable R, tag Tag>
struct valid_extern_impl : std::false_type {};

// ---

template <returnable R, tag Tag>
struct valid_extern_impl<discard_t, R, Tag> : std::true_type {};

template <returnable R, tag Tag>
  requires return_address_for<R *, R>
struct valid_extern_impl<R *, R, Tag> : std::true_type {};

template <returnable R, tag Tag>
struct valid_extern_impl<eventually<R> *, R, Tag> : std::true_type {};

template <returnable R, tag Tag>
struct valid_extern_impl<try_eventually<R> *, R, Tag> : std::true_type {};

// ---

template <returnable R, tag Tag>
struct valid_extern_impl<future_shared_state_ptr<R>, R, Tag> : std::true_type {};

} // namespace detail

/**
 * @brief Verify that a return type is valid for an extern'ed function.
 */
template <typename I, typename R, tag Tag>
concept valid_extern = quasi_pointer<I> && returnable<R> && detail::valid_extern_impl<I, R, Tag>::value;

/**
 * @brief A small shim/coroutine-wrapper that strips the `CallArgs` from an async dispatch.
 */
template <returnable R, async_function_object F>
struct extern_shim {
  /**
   * @brief Strips `CallArgs` and forwards to `F`.
   */
  template <quasi_pointer I, tag Tag, typename... CallArgs, typename... Args>
    requires valid_extern<I, R, Tag>
  LF_CORO_WRAPPER LF_FORCEINLINE LF_STATIC_CALL auto
  operator()(first_arg_t<I, Tag, extern_shim, CallArgs...> /**/, Args &&...args) LF_STATIC_CONST->task<R> {
    return F{}(first_arg_t<I, Tag, F>{{}}, std::forward<Args>(args)...);
  }
};

namespace detail {

/**
 * @brief We need another type to represent the discard_t in the extern_ret_ptr_t so we don't violate ORD.
 */
struct other_discard_t : discard_t {
  using discard_t::operator*;
};

template <returnable R>
struct extern_ret_ptr_impl : std::type_identity<other_discard_t> {};

template <returnable R>
  requires return_address_for<R *, R>
struct extern_ret_ptr_impl<R> : std::type_identity<R *> {};

} // namespace detail

/**
 * @brief If `R *` is a valid return address for `R` then `R *` else `discard_t`.
 */
template <returnable R>
using extern_ret_ptr_t = detail::extern_ret_ptr_impl<R>::type;

} // namespace lf::impl

// ---------------------- Forward declare an external function ---------------------- //

/**
 * @brief Forward declare an extern'ed function.
 */
#define LF_EXTERN_FWD_DECL(R, f, ...)                                                                        \
  namespace f##_impl {                                                                                       \
    struct f##_fn {                                                                                          \
      LF_STATIC_CALL ::lf::task<R> operator()(auto __VA_OPT__(, ) __VA_ARGS__) LF_STATIC_CONST;              \
    };                                                                                                       \
  }                                                                                                          \
  inline constexpr auto f = ::lf::impl::extern_shim<R, f##_impl::f##_fn> {}
/**
 * @brief Instantiate the extern'ed function for a specific call type and return type.
 */
#define LF_INSTANTIATE_SELF(R, f, head, ...)                                                                 \
  template ::lf::task<R> f##_impl::f##_fn::operator()<head>(head __VA_OPT__(, ) __VA_ARGS__) LF_STATIC_CONST
/**
 * @brief Build the first argument for the extern'ed function.
 */
#define LF_FIRST_ARG(R, I, Tag, f) ::lf::impl::first_arg_t<I, Tag, f##_impl::f##_fn>
/**
 * @brief Instantiate the extern'ed function generating the appropriate first argument.
 */
#define LF_INSTANTIATE(R, f, I, Tag, ...)                                                                    \
  LF_INSTANTIATE_SELF(R, f, LF_FIRST_ARG(R, I, Tag, f) __VA_OPT__(, ) __VA_ARGS__)
/**
 * @brief Instantiate for all valid return types.
 */
#define LF_INSTANTIATE_RETURNS(declspec, R, f, Tag, ...)                                                     \
  declspec LF_INSTANTIATE(R, f, ::lf::impl::extern_ret_ptr_t<R>, Tag __VA_OPT__(, ) __VA_ARGS__);            \
  declspec LF_INSTANTIATE(R, f, ::lf::impl::discard_t, Tag __VA_OPT__(, ) __VA_ARGS__);                      \
  declspec LF_INSTANTIATE(R, f, ::lf::eventually<R> *, Tag __VA_OPT__(, ) __VA_ARGS__);                      \
  declspec LF_INSTANTIATE(R, f, ::lf::try_eventually<R> *, Tag __VA_OPT__(, ) __VA_ARGS__)
/**
 * @brief Forward declare an async function.
 *
 * This is useful for speeding up compile times by allowing you to put the definition
 * of an async function in a source file and only forward declare it in a header file.
 *
 * \rst
 *
 * Usage: in a header file (e.g. ``fib.hpp``), forward declare an async function:
 *
 * .. code::
 *
 *    LF_FWD_DECL(int, fib, int n);
 *
 * And in a corresponding source file (e.g. ``fib.cpp``), implement the function:
 *
 * .. code::
 *
 *    LF_IMPLEMENT(int, fib, int n) {
 *
 *      if (n < 2) {
 *        co_return n;
 *      }
 *
 *      int a, b;
 *
 *      co_await lf::fork(&a, fib)(n - 1);
 *      co_await lf::call(&b, fib)(n - 2);
 *
 *      co_await lf::join;
 *    }
 *
 * \endrst
 *
 * Now in some other file you can include ``fib.hpp`` and use ``fib`` as normal with the restriction
 * that you must always bind the result of the extern'ed function to an ``lf::core::eventually<int> *``,
 * ``lf::core::try_eventually<int> *`` or ``int *``.
 */
#define LF_FWD_DECL(R, f, ...)                                                                               \
  LF_EXTERN_FWD_DECL(R, f __VA_OPT__(, ) __VA_ARGS__);                                                       \
  LF_INSTANTIATE_RETURNS(extern, R, f, ::lf::tag::call __VA_OPT__(, ) __VA_ARGS__);                          \
  LF_INSTANTIATE_RETURNS(extern, R, f, ::lf::tag::fork __VA_OPT__(, ) __VA_ARGS__);                          \
  extern LF_INSTANTIATE(                                                                                     \
      R, f, ::lf::impl::future_shared_state_ptr<R>, ::lf::tag::root __VA_OPT__(, ) __VA_ARGS__)

// ---------------------- Implement a forward declared function ---------------------- //

/**
 * @brief An alternative to ``LF_IMPLEMENT`` that allows you to name the ``self`` parameter.
 */
#define LF_IMPLEMENT_NAMED(R, f, self, ...)                                                                  \
  LF_INSTANTIATE_RETURNS(/* nodecl */, R, f, ::lf::tag::call __VA_OPT__(, ) __VA_ARGS__);                    \
  LF_INSTANTIATE_RETURNS(/* nodecl */, R, f, ::lf::tag::fork __VA_OPT__(, ) __VA_ARGS__);                    \
  LF_INSTANTIATE(R, f, ::lf::impl::future_shared_state_ptr<R>, ::lf::tag::root __VA_OPT__(, ) __VA_ARGS__);  \
  ::lf::task<R> f##_impl::f##_fn::operator()(auto self __VA_OPT__(, ) __VA_ARGS__) LF_STATIC_CONST

/**
 * @brief See ``LF_FWD_DECL`` for usage.
 */
#define LF_IMPLEMENT(R, f, ...) LF_IMPLEMENT_NAMED(R, f, f __VA_OPT__(, ) __VA_ARGS__)

#endif /* E6A77A20_6653_4B56_9931_BA5B0987911A */
