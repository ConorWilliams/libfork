module;
#include <version>

#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:promise;

import std;

import :concepts;
import :utility;
import :frame;
import :tuple;

namespace lf {

// =============== Forward-decl =============== //

template <typename T, alloc_mixin StackPolicy>
struct promise_type;

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
 * \endrst
 */
export template <returnable T, alloc_mixin Stack>
struct task : std::type_identity<T>, immovable {
  promise_type<T, Stack> *promise;
};

// =============== Frame-mixin =============== //

[[nodiscard]]
constexpr auto final_suspend(frame_type *frame) -> std::coroutine_handle<> {

  // TODO: noexcept

  LF_ASSUME(frame != nullptr);

  frame_type *parent_frame = frame->parent;

  // Destroy the child frame
  frame->handle().destroy();

  if (parent_frame != nullptr) {
    return parent_frame->handle();
  }

  return std::noop_coroutine();
}

struct final_awaitable : std::suspend_always {
  template <typename... Ts>
  constexpr auto
  await_suspend(std::coroutine_handle<promise_type<Ts...>> handle) noexcept -> std::coroutine_handle<> {
    return final_suspend(&handle.promise().frame);
  }
};

struct call_awaitable : std::suspend_always {

  frame_type *child;

  template <typename... Us>
  auto await_suspend(std::coroutine_handle<promise_type<Us...>> parent) noexcept -> std::coroutine_handle<> {

    // TODO: destroy on child if cannot launch i.e. scheduling failure

    LF_ASSUME(child != nullptr);
    LF_ASSUME(child->parent == nullptr);

    child->parent = &parent.promise().frame;

    return child->handle();
  }
};

// clang-format off

template <typename R, typename Fn, typename... Args>
struct package {
  R *return_address;
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

template <typename Fn, typename... Args>
struct package<void, Fn, Args...> {
  [[no_unique_address]] Fn fn;
  [[no_unique_address]] tuple<Args...> args;
};

// clang-format on

template <typename R, typename Fn, typename... Args>
struct call_pkg : package<R, Fn, Args...> {};

export template <typename... Args, async_invocable_to<void, Args...> Fn>
constexpr auto call(Fn &&fn, Args &&...args) noexcept -> call_pkg<void, Fn, Args &&...> {
  return {LF_FWD(fn), {LF_FWD(args)...}};
}

export template <typename R, typename... Args, async_invocable_to<R, Args...> Fn>
constexpr auto call(R *ret, Fn &&fn, Args &&...args) noexcept -> call_pkg<R, Fn, Args &&...> {
  return {ret, LF_FWD(fn), {LF_FWD(args)...}};
}

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

  template <typename R, typename Fn, typename... Args>
  constexpr static auto await_transform(call_pkg<R, Fn, Args...> &&pkg) noexcept -> call_awaitable {

    task child = std::move(pkg.args).apply([&](auto &&...args) {
      return std::invoke(std::move(pkg.fn), key{}, LF_FWD(args)...);
    });

    if constexpr (!std::is_void_v<R>) {
      child.promise->return_address = pkg.return_address;
    }

    return {.child = &child.promise->frame};
  }

  constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  constexpr static auto final_suspend() noexcept -> final_awaitable { return {}; }

  constexpr static void unhandled_exception() noexcept { std::terminate(); }
};

static_assert(std::is_empty_v<mixin_frame>);

// =============== Promise (void) =============== //

template <alloc_mixin StackPolicy>
struct promise_type<void, StackPolicy> : StackPolicy, mixin_frame {

  frame_type frame;

  constexpr auto get_return_object() noexcept -> task<void, StackPolicy> { return {.promise = this}; }

  constexpr static void return_void() noexcept {}
};

struct dummy_alloc {
  static auto operator new(std::size_t) -> void *;
  static auto operator delete(void *, std::size_t) noexcept -> void;
};

static_assert(alignof(promise_type<void, dummy_alloc>) == alignof(frame_type));

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&promise_type<void, dummy_alloc>::frame));
#else
static_assert(std::is_standard_layout_v<promise_type<void, dummy_alloc>>);
#endif

// =============== Promise (non-void) =============== //

template <typename T, alloc_mixin StackPolicy>
struct promise_type : StackPolicy, mixin_frame {

  frame_type frame;
  T *return_address;

  constexpr auto get_return_object() noexcept -> task<T, StackPolicy> { return {.promise = this}; }

  template <typename U = T>
    requires std::assignable_from<T &, U &&>
  constexpr void return_value(U &&value) noexcept {
    *return_address = LF_FWD(value);
  }
};

static_assert(alignof(promise_type<int, dummy_alloc>) == alignof(frame_type));

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&promise_type<int, dummy_alloc>::frame));
#else
static_assert(std::is_standard_layout_v<promise_type<int, dummy_alloc>>);
#endif

} // namespace lf

// =============== std specialization =============== //

template <typename R, typename... Policy, typename... Args>
struct std::coroutine_traits<lf::task<R, Policy...>, Args...> {
  using promise_type = ::lf::promise_type<R, Policy...>;
};
