module;
#include <version>

#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:promise;

import std;

import :concepts;
import :constants;
import :frame;
import :utility;
import :tuple;

import :context;
import :ops;

namespace lf {

template <typename T = void>
using coro = std::coroutine_handle<T>;

// =============== Forward-decl =============== //

template <returnable T, alloc_mixin Stack, context Context = polymorphic_context>
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
struct task : immovable, std::type_identity<T> {
  promise_type<T, Stack> *promise;
};

// =============== Final =============== //

[[nodiscard]]
constexpr auto final_suspend(frame_type *frame) noexcept -> coro<> {

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
  constexpr static auto await_suspend(coro<promise_type<Ts...>> handle) noexcept -> coro<> {
    return final_suspend(&handle.promise().frame);
  }
};

// =============== Call =============== //

struct call_awaitable : std::suspend_always {

  frame_type *child;

  template <typename... Us>
  auto await_suspend(coro<promise_type<Us...>> parent) noexcept -> coro<> {

    // TODO: destroy on child if cannot launch i.e. scheduling failure

    LF_ASSUME(child != nullptr);
    LF_ASSUME(child->parent == nullptr);

    child->parent = &parent.promise().frame;

    return child->handle();
  }
};

// =============== Fork =============== //

struct fork_awaitable : std::suspend_always {

  frame_type *child;

  template <typename T, alloc_mixin S, typename Context>
  auto await_suspend(coro<promise_type<T, S, Context>> parent) noexcept -> coro<> {

    // TODO: destroy on child if cannot launch i.e. scheduling failure

    LF_ASSUME(child != nullptr);
    LF_ASSUME(child->parent == nullptr);

    child->parent = &parent.promise().frame;

    // Make a copy on the stack, this object may be destroyed
    // after the push to the scheduler's queue
    frame_type *local_child = child;

    LF_ASSUME(thread_context<Context> != nullptr);

    LF_TRY {
      thread_context<Context>->push(work_handle{.frame = local_child});
    } LF_CATCH_ALL {
      local_child->handle().destroy();
      // TODO: stash in parent frame (should not throw)
      LF_RETHROW;
    }

    return local_child->handle();
  }
};

// =============== Frame mixin =============== //

struct mixin_frame {

  // === For internal use === //

  template <typename Self>
    requires (!std::is_const_v<Self>)
  [[nodiscard]]
  constexpr auto handle(this Self &self) LF_HOF(coro<Self>::from_promise(self))

  [[nodiscard]]
  constexpr auto get_frame(this auto &&self)
      LF_HOF(LF_FWD(self).frame)

  // === Called by the compiler === //

  template <typename R, typename Fn, typename... Args>
  constexpr static auto await_transform(call_pkg<R, Fn, Args...> &&pkg) noexcept -> call_awaitable {

    task child = std::move(pkg.args).apply(std::move(pkg.fn));

    child.promise->frame.kind = category::call;

    if constexpr (!std::is_void_v<R>) {
      child.promise->return_address = pkg.return_address;
    }

    return {.child = &child.promise->frame};
  }

  template <typename R, typename Fn, typename... Args>
  constexpr static auto await_transform(fork_pkg<R, Fn, Args...> &&pkg) noexcept -> fork_awaitable {

    task child = std::move(pkg.args).apply(std::move(pkg.fn));

    child.promise->frame.kind = category::fork;

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

template <alloc_mixin Stack>
struct promise_type<void, Stack> : Stack, mixin_frame {

  frame_type frame;

  constexpr auto get_return_object() noexcept -> task<void, Stack> { return {.promise = this}; }

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

template <returnable T, alloc_mixin Stack, context Context>
struct promise_type : Stack, mixin_frame {

  frame_type frame;
  T *return_address;

  constexpr auto get_return_object() noexcept -> task<T, Stack> { return {.promise = this}; }

  template <typename U = T>
    requires std::assignable_from<T &, U &&>
  constexpr void return_value(U &&value) noexcept(std::is_nothrow_assignable_v<T &, U &&>) {
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
