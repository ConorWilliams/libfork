module;
#include <version>

#include "libfork/__impl/assume.hpp"
#include "libfork/__impl/exception.hpp"
#include "libfork/__impl/utils.hpp"
export module libfork.core:promise;

import std;

import libfork.utils;

import :concepts_awaitable;
import :concepts_context;
import :concepts_invocable;
import :frame;
import :stop;
import :task;
import :thread_locals;
import :ops;
import :handles;
import :final_suspend;
import :awaitables;

// TODO: vet constexpr usage in the library

namespace lf {

// =============== Final awaitable =============== //

struct final_awaitable : std::suspend_always {
  template <returnable T, worker_context Context>
  constexpr static auto await_suspend(coro<promise_type<T, Context>> handle) noexcept -> coro<> {
    return final_suspend_leading<Context>(&handle.promise().frame);
  }
};

// =============== Frame mixin =============== //

template <worker_context Context>
struct mixin_frame {

  // === For internal use === //

  using enum category;

  template <typename Self>
    requires (!std::is_const_v<Self>)
  [[nodiscard]]
  constexpr auto handle(this Self &self)
      LF_HOF(coro<Self>::from_promise(self))

  // === Called by the compiler === //

  // --- Allocation

  static auto operator new(std::size_t sz) noexcept(noexcept(get_tls_stack<Context>().push(sz))) -> void * {
    void *ptr = get_tls_stack<Context>().push(sz);
    LF_ASSUME(is_sufficiently_aligned<k_new_align>(ptr));
    return std::assume_aligned<k_new_align>(ptr);
  }

  static auto operator delete(void *p, std::size_t sz) noexcept -> void {
    get_tls_stack<Context>().pop(p, sz);
  }

  // --- Await transformations

  // Fork/call
  template <category Cat, bool StopToken, typename R, typename Fn, typename... Args>
  constexpr auto await_transform(this auto &self, pkg<Cat, StopToken, Context, R, Fn, Args...> &&pkg) noexcept
      -> async_awaitable<Cat, Context> {
    LF_TRY {
      return self.await_transform_pkg(std::move(pkg));
    } LF_CATCH_ALL {
      stash_current_exception(&self.frame);
    }
    return {.child = nullptr};
  }

  // Custom awaitable
  template <awaitable<Context> T>
  static constexpr auto await_transform(T &&x)
      LF_HOF(switch_awaitable_for<Context>(acquire_awaitable(LF_FWD(x))))

  // Join
  constexpr auto await_transform(this auto &self, join_type) noexcept -> join_awaitable<Context> {
    return {.frame = &self.frame};
  }

  // Scope getter (propagate stop token)
  static constexpr auto await_transform(scope_type) noexcept -> scope_awaitable<Context> { return {}; }

  // Scope getter (new attached stop token)
  constexpr auto
  await_transform(this auto const &self, child_scope_type) noexcept -> child_scope_awaitable<Context> {
    return {.parent_stop_token = self.frame.stop_token};
  }

  // --- Other

  constexpr static auto initial_suspend() noexcept -> std::suspend_always { return {}; }

  constexpr static auto final_suspend() noexcept -> final_awaitable { return {}; }

  constexpr void unhandled_exception(this auto &self) noexcept {
    // Stash the exception in the parent which will rethrow at the join.
    stash_current_exception(self.frame.parent);
  }

 private:
  template <category Cat, bool StopToken, typename R, typename Fn, typename... Args>
  constexpr auto
  await_transform_pkg(this auto const &self, pkg<Cat, StopToken, Context, R, Fn, Args...> &&pkg) noexcept(
      async_nothrow_invocable<Fn, Context, Args...>) -> async_awaitable<Cat, Context> {
    using U = async_result_t<Fn, Context, Args...>;

    // clang-format off

    promise_type<U, Context> *child_promise = get(key(), std::move(pkg.args).apply(
      [&](auto &&...args) LF_HOF(ctx_invoke_t<Context>{}(fwd_fn<Fn>(pkg.fn), LF_FWD(args)...))
    ));

    // clang-format on

    LF_ASSUME(child_promise);

    // void can signal drop return.
    static_assert(std::same_as<R, U> || std::is_void_v<R>);

    if constexpr (!std::is_void_v<R>) {
      child_promise->return_address = not_null(pkg.return_addr);
    } else if constexpr (!std::is_void_v<U>) {
      // Set child's return address to null to inhibit the return
      child_promise->return_address = nullptr;
    }

    if constexpr (StopToken) {
      // TODO: need some kind of API to launch an unstoppable task?
      LF_ASSUME(pkg.stop_token.stop_possible());
      child_promise->frame.stop_token = pkg.stop_token;
    } else {
      child_promise->frame.stop_token = self.frame.stop_token;
    }

    return {.child = &child_promise->frame};
  }
};

// =============== Promise (void) =============== //

template <worker_context Context>
struct promise_type<void, Context> : mixin_frame<Context> {

  // Putting init here allows:
  //  1. Frame not to need to know about the checkpoint type
  //  2. Compiler merge double read of thread local here and in allocator
  frame_t<Context> frame{get_tls_stack<Context>().checkpoint()};

  constexpr auto get_return_object() noexcept -> task<void, Context> { return {key(), this}; }

  constexpr static void return_void() noexcept {}
};

// =============== Promise (non-void) =============== //

template <returnable T, worker_context Context>
struct promise_type : mixin_frame<Context> {

  // Putting init here allows:
  //  1. Frame not to need to know about the checkpoint type
  //  2. Compiler merge double read of thread local here and in allocator
  frame_t<Context> frame{get_tls_stack<Context>().checkpoint()};
  T *return_address;

  constexpr auto get_return_object() noexcept -> task<T, Context> { return {key(), this}; }

  template <typename U = T>
    requires std::assignable_from<T &, U &&>
  constexpr void return_value(U &&value) noexcept(std::is_nothrow_assignable_v<T &, U &&>) {
    if (return_address) {
      *return_address = LF_FWD(value);
    }
  }
};

} // namespace lf

// =============== std specialization =============== //

template <typename R, lf::worker_context Context, typename... Args>
struct std::coroutine_traits<lf::task<R, Context>, Args...> {
  using promise_type = ::lf::promise_type<R, Context>;
};

template <typename R, typename Self, lf::worker_context Context, typename... Args>
struct std::coroutine_traits<lf::task<R, Context>, Self, Args...> {
  using promise_type = ::lf::promise_type<R, Context>;
};

// =============== Layout invariants =============== //

// clang-format off

namespace {

struct unit_checkpoint {
  auto operator==(unit_checkpoint const &) const -> bool = default;
};

struct unit_stack {
  static auto push(std::size_t) -> void *;
  static auto pop(void *, std::size_t) noexcept -> void;
  static auto checkpoint() noexcept -> unit_checkpoint;
  static auto prepare_release() noexcept -> int;
  static auto release(int) noexcept -> void;
  static auto acquire(unit_checkpoint) noexcept -> void;
};

struct unit_context {
  void push(lf::steal_handle<unit_context>);
  auto pop() noexcept -> lf::steal_handle<unit_context>;
  auto stack() noexcept -> unit_stack &;
};

static_assert(lf::worker_context<unit_context>);

using frame_t = lf::frame_type<unit_checkpoint>;

static_assert(std::is_standard_layout_v<frame_t>);
static_assert(alignof(lf::promise_type<void, unit_context>) == alignof(frame_t));
static_assert(alignof(lf::promise_type<int, unit_context>) == alignof(frame_t));
static_assert(std::is_standard_layout_v<lf::promise_type<void, unit_context>>);
static_assert(std::is_standard_layout_v<lf::promise_type<int, unit_context>>);

#ifdef __cpp_lib_is_pointer_interconvertible
static_assert(std::is_pointer_interconvertible_with_class(&lf::promise_type<void, unit_context>::frame));
static_assert(std::is_pointer_interconvertible_with_class(&lf::promise_type<int, unit_context>::frame));
#endif

} // namespace
