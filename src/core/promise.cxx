module;
#include <version>

#include "libfork/macros.hpp"
export module libfork.core:promise;

import std;

import :concepts;

namespace lf {

// =============== Task =============== //

/**
 * @brief The return type for libfork's async functions/coroutines.
 *
 * This predominantly exists to disambiguate `libfork`s coroutines from other
 * coroutines and specify `T` the async function's return type which is
 * required to be `void` or a `std::movable` type.
 *
 * \rst
 *
 * .. note::
 *
 *    No consumer of this library should ever touch an instance of this type,
 *    it is used for specifying the return type of an `async` function only.
 *
 * .. warning::
 *    The value type ``T`` of a coroutine should be independent of the
 *    coroutines first-argument.
 *
 * \endrst
 */
export template <returnable T = void>
class task {};

// =============== Frame =============== //

struct frame_type {
  frame_type *parent;
};

static_assert(std::is_standard_layout_v<frame_type>);

// =============== Frame-mixin =============== //

struct mixin_frame {
  auto self(this auto &&self)
      LF_HOF(LF_FWD(self).frame)

  // auto get_return_object() -> task_of { return {handle<promise_type>::from_promise(*this)}; }
  //
  // auto initial_suspend() -> std::suspend_always { return {}; }
  //
  // auto final_suspend() noexcept {
  //   struct final_awaitable : std::suspend_always {
  //     auto await_suspend(handle<promise_type> h) noexcept -> handle<> {
  //
  //       handle continue_ = h.promise().continue_;
  //
  //       h.destroy();
  //
  //       if (continue_) {
  //         return continue_;
  //       }
  //
  //       return std::noop_coroutine();
  //     }
  //   };
  //
  //   return final_awaitable{};
  // }
};

static_assert(std::is_empty_v<mixin_frame>);

// =============== Promises =============== //

template <typename T>
struct promise_type;

template <>
struct promise_type<void> : mixin_frame {
  frame_type frame;
};

static_assert(alignof(promise_type<void>) == alignof(frame_type));

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&promise_type<void>::frame));
#else
static_assert(std::is_standard_layout_v<promise_type<void>>);
#endif

template <typename T>
struct promise_type : mixin_frame {
  frame_type frame;
  T *return_address;
};

static_assert(alignof(promise_type<int>) == alignof(frame_type));

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&promise_type<int>::frame));
#else
static_assert(std::is_standard_layout_v<promise_type<int>>);
#endif

} // namespace lf
