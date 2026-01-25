module;
#include <version>

#include "libfork/__impl/compiler.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:promise;

import std;

import :concepts;
import :utility;
import :frame;

namespace lf {

// =============== Forward-decl =============== //

template <typename T>
struct promise_type;

// =============== Task =============== //

/**
 * @brief `std::unique_ptr` compatible deleter for coroutine promises.
 */
struct promise_deleter {
  template <typename T>
  constexpr static void operator()(T *ptr) noexcept {
    std::coroutine_handle<T>::from_promise(*ptr).destroy();
  }
};

template <typename T>
using unique_promise = std::unique_ptr<T, promise_deleter>;

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
struct task final : unique_promise<promise_type<T>> {};

// =============== Frame-mixin =============== //

[[nodiscard]]
constexpr auto final_suspend(frame_type *frame) -> std::coroutine_handle<> {

  LF_ASSUME(frame != nullptr);

  frame_type *parent_frame = frame->parent;

  {
    // Destroy the child frame
    unique_promise<frame_type> _{frame};
  }

  if (parent_frame != nullptr) {
    return std::coroutine_handle<frame_type>::from_promise(*parent_frame);
  }

  return std::noop_coroutine();
}

struct final_awaitable : std::suspend_always {
  template <typename T>
  constexpr auto
  await_suspend(std::coroutine_handle<promise_type<T>> handle) noexcept -> std::coroutine_handle<> {
    return final_suspend(&handle.promise().frame);
  }
};

template <typename T>
struct just_awaitable : std::suspend_always {

  task<T> child;

  template <typename U>
  auto await_suspend(std::coroutine_handle<promise_type<U>> parent) noexcept -> std::coroutine_handle<> {

    LF_ASSUME(child != nullptr);
    LF_ASSUME(child->frame.parent == nullptr);

    child->frame.parent = &parent.promise().frame;

    return child.release()->handle();
  }
};

struct mixin_frame {

  // === For internal use === //

  template <typename Self>
    requires (!std::is_const_v<Self>)
  [[nodiscard]]
  constexpr auto handle(this Self &self) LF_HOF(std::coroutine_handle<Self>::from_promise(self))

  [[nodiscard]]
  constexpr auto get_frame(this auto &&self)
      LF_HOF(LF_FWD(self).frame)

  // === Called by the compiler === //

  static constexpr auto await_transform(task<void> child) -> just_awaitable<void> {
    return {.child = std::move(child)};
  }

  constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  constexpr static auto final_suspend() noexcept -> final_awaitable { return {}; }

  constexpr static void unhandled_exception() noexcept { std::terminate(); }
};

static_assert(std::is_empty_v<mixin_frame>);

// =============== Promises =============== //

template <>
struct promise_type<void> : mixin_frame {

  frame_type frame;

  constexpr auto get_return_object() -> task<void> { return {{this, {}}}; }

  constexpr static void return_void() {}
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

template <typename R, typename... Args>
struct std::coroutine_traits<lf::task<R>, Args...> {
  using promise_type = ::lf::promise_type<R>;
};
