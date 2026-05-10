---
icon: lucide/cpu
---

# Core

Core defines the task model, fork-join operations, scheduling entry points,
receivers, cancellation, context interfaces, handles, concepts, and exceptions.

## Tasks

### `concept returnable<T>`

`T` may be returned from a libfork task. It is either `void` or a movable plain
object type.

### `struct env<Context>`

Environment object passed as the first argument to async functions. Users name
it in call operators but do not construct it directly.

```cpp
template <lf::worker_context Context>
auto operator()(lf::env<Context>, int value) -> lf::task<int, Context>;
```

### `class task<T, Context>`

Coroutine return type for async functions. `T` is the result type and `Context`
is the worker context. The public aliases are:

- `value_type`
- `context_type`

User code should treat `task` as a return type only.

## Fork-join scopes

### `scope()`

`co_await lf::scope()` returns a scope object for ordinary child tasks.

The returned object provides:

- `fork(R* ret, Fn&& fn, Args&&...)`
- `fork(Fn&& fn, Args&&...)` for `void` tasks
- `fork_drop(Fn&& fn, Args&&...)`
- `call(R* ret, Fn&& fn, Args&&...)`
- `call(Fn&& fn, Args&&...)` for `void` tasks
- `call_drop(Fn&& fn, Args&&...)`
- `join()`

`fork` makes the parent continuation stealable and starts the child. `call`
starts the child inline. `join` waits for outstanding children and rethrows a
stored child exception if one exists.

### `child_scope()`

`co_await lf::child_scope()` returns a scope object that also derives from
`stop_source`. Children launched through the scope receive the scope stop token.

Use this when a subtree should be cancellable independently from its parent.

## Scheduling

### `schedule(Sch&&, recv_state<R, S>, Fn&&, Args&&...)`

Schedules an async function as a root task using caller-provided receiver state.
The scheduler must satisfy `scheduler`, and the function must return
`task<R, context_t<Sch>>`.

This overload is used for custom allocation, pre-initialized return storage, or
cancellable roots.

### `schedule(Sch&&, Fn&&, Args&&...)`

Convenience overload that creates a non-cancellable `recv_state` with the
default allocator. The result type must be `void` or default-initializable and
movable.

### `class recv_state<T, Stoppable = false>`

Move-only shared state for a root task and receiver. It embeds the root frame
buffer and stores the task result, exception, readiness flag, and optional stop
source.

Constructors mirror `std::make_shared` and `std::allocate_shared`:

```cpp
lf::recv_state<int> a;
lf::recv_state<int> b{42};
lf::recv_state<int> c{std::allocator_arg, alloc};
lf::recv_state<int> d{std::allocator_arg, alloc, 42};
lf::recv_state<int, true> stoppable;
```

### `class receiver<T, Stoppable = false>`

Move-only handle returned by `schedule`.

Methods:

- `valid() const noexcept -> bool`
- `ready() const -> bool`
- `wait() const -> void`
- `stop_source() -> lf::stop_source&`, only when `Stoppable == true`
- `get() && -> T`

`get()` waits, consumes the receiver, returns the result, rethrows a stored
exception, or throws `operation_cancelled_error` for cancelled stoppable roots.

## Cancellation

### `class stop_source`

Immovable stop source. A `stop_source` may be chained to a parent token.

Methods:

- `token() const noexcept -> stop_token`
- `stop_requested() const noexcept -> bool`
- `request_stop() noexcept -> void`
- `race_request_stop() noexcept -> bool`

### `class stop_source::stop_token`

Lightweight non-owning stop handle.

Methods:

- `stop_possible() const noexcept -> bool`
- `stop_requested() const noexcept -> bool`

Stop checks walk the token chain, so deeply nested `child_scope` trees pay
proportionally more per check.

## Handles and execution

### `unsafe_steal_handle`

Untyped handle to a task suspended at a fork point. Context policies use it when
they cannot name the concrete context type.

### `unsafe_sched_handle`

Untyped handle to a scheduled task. Used for type erasure across context types.

### `steal_handle<Context>`

Typed handle to a stealable task suspended at a fork point.

### `sched_handle<Context>`

Typed handle to a not-yet-started task or a task suspended at a context switch.

### `execute(Context&, sched_handle<Context>)`

Binds the current thread to `Context`, acquires the task stack when needed, and
resumes a scheduled task. Scheduler implementations call this.

### `execute(Context&, steal_handle<Context>)`

Binds the current thread to `Context`, marks the task as stolen, and resumes it.
Scheduler implementations call this after stealing work.

## Context base classes

### `base_context<Stack>`

Base class storing a worker stack and exposing:

```cpp
auto stack() noexcept -> Stack&;
```

### `poly_context<Stack>`

Polymorphic context base. It inherits `base_context<Stack>` and requires
derived classes to implement:

```cpp
virtual void push(steal_handle<poly_context>) = 0;
virtual auto pop() noexcept -> steal_handle<poly_context> = 0;
```

It also provides virtual `post(sched_handle<poly_context>)`, which throws
`post_error` unless overridden.

## Concepts and aliases

### Stack and context concepts

- `worker_stack<T>`: stack API used for coroutine frame allocation.
- `lifo_stack<T, U>`: plain object with `push(U)` and `noexcept pop() -> U`.
- `worker_context<T>`: LIFO stack of `steal_handle<T>` with a worker stack.
- `stack_t<T>`: stack type extracted from a worker context.

### Scheduler concepts

- `has_context_typedef<T>`: type has `context_type`.
- `context_t<T>`: extracts `std::remove_cvref_t<T>::context_type`.
- `scheduler<Sch>`: scheduler with `post(sched_handle<context_t<Sch>>) -> void`.

### Async invocation concepts

- `async_invocable<Fn, Context, Args...>`
- `async_regular_invocable<Fn, Context, Args...>`
- `async_nothrow_invocable<Fn, Context, Args...>`
- `async_result_t<Fn, Context, Args...>`
- `async_invocable_to<Fn, R, Context, Args...>`
- `async_nothrow_invocable_to<Fn, R, Context, Args...>`

An async invocable may accept `lf::env<Context>` as its first argument or omit
it if it is otherwise invocable with the provided arguments.

### Awaitable concepts

- `acquire_awaitable(T&&)`: applies member/free `operator co_await` when present.
- `awaitable<T, Context>`: context-switching awaitable with
  `await_suspend(sched_handle<Context>, Context&)`.

### Indirect and projected concepts

The `sync` and `async` namespaces each export:

- `indirectly_unary_invocable`
- `indirectly_regular_unary_invocable`

The top-level concepts accept either synchronous or asynchronous invocables:

- `indirectly_unary_invocable<Fn, Context, I>`
- `indirectly_regular_unary_invocable<Fn, Context, I>`

`projectable<Fn, Context, I>` tests whether an iterator can be projected through
`Fn`. `projected<Context, I, Fn>` is a libfork-aware variant of
`std::projected`.

### Semigroup concepts

The `sync` and `async` namespaces each export `indirect_semigroup`.

Top-level exports:

- `indirect_semigroup<Fn, Context, I>`
- `indirect_commutative_semigroup<Fn, Context, I>`
- `indirect_semigroup_t<Fn, Context, I>`

These support algorithms such as `fold`, including binary operations that are
ordinary functions or async functions.

## Exceptions

All libfork exceptions derive from `libfork_exception`.

| Type | Typical source | Meaning |
| --- | --- | --- |
| `libfork_exception` | base | Base type for libfork exceptions. |
| `schedule_error` | `schedule` | Called from a worker thread. |
| `execute_error` | `execute` | Called from a worker thread. |
| `steal_overflow_error` | `execute(steal_handle)` | A task was stolen too many times. |
| `root_alloc_error` | `schedule` | Root coroutine frame exceeds the embedded receiver buffer. |
| `broken_receiver_error` | `receiver` | Receiver has no valid state. |
| `operation_cancelled_error` | `receiver::get` | Stoppable root completed with stop requested. |
| `post_error` | `poly_context::post` | Derived context did not override `post`. |
