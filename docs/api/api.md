# libfork Public API

All symbols live in the `lf` namespace. Access them via `import libfork;`.

---

## Core concepts

### `concept returnable<T>` — `:task`

`T` is `void` or a `std::movable` plain object type. Used as the return-type constraint on async functions.

### `concept worker_stack<T>` — `:concepts_stack`

A type that provides a contiguous stack with `push`, `pop`, `checkpoint`, `prepare_release`, `release`, and `acquire`.

### `concept lifo_stack<T, U>` — `:concepts_context`

`T` is a plain object type supporting `push(U)` and a `noexcept pop() -> U`. Used to define `worker_context`.

### `concept worker_context<T>` — `:concepts_context`

A type that satisfies `lifo_stack<T, steal_handle<T>>` and exposes a `worker_stack` via a `noexcept stack()`.

### `using stack_t<Context>` — `:concepts_context`

Extracts the stack type from a `worker_context`.

### `concept has_context_typedef<T>` — `:concepts_scheduler`

`T` has a `context_type` member typedef. Used to define `scheduler` and constrain `context_t`.

### `concept scheduler<Sch>` — `:concepts_scheduler`

A type satisfying `has_context_typedef` with a `post(sched_handle<context_type>)` method.

### `using context_t<T>` — `:concepts_scheduler`

Extracts `T::context_type`. Requires `has_context_typedef<T>`.

### `concept async_invocable<Fn, Context, Args...>` — `:concepts_invocable`

`Fn` is callable with an `env<Context>` (or without one) and `Args...`, returning an `lf::task`.

### `concept async_nothrow_invocable<Fn, Context, Args...>` — `:concepts_invocable`

Subsumes `async_invocable` and requires the call to be `noexcept`.

### `using async_result_t<Fn, Context, Args...>` — `:concepts_invocable`

The `value_type` of the `task` returned by invoking `Fn`.

### `concept async_invocable_to<Fn, R, Context, Args...>` — `:concepts_invocable`

Subsumes `async_invocable` and constrains the result type to `R`.

### `concept async_nothrow_invocable_to<Fn, R, Context, Args...>` — `:concepts_invocable`

Subsumes both `async_nothrow_invocable` and `async_invocable_to`.

---

## Coroutine types

### `struct env<Context>` — `:task`

The Y-combinator environment. Passed as the first argument to every async function, allowing recursive self-calls. Users declare it but never construct it directly.

### `class task<T, Context>` — `:task`

The return type of all async functions. `T` must satisfy `returnable`. Users never store or manipulate instances — the type exists solely to identify libfork coroutines.

---

## Handles

### `struct unsafe_steal_handle` — `:handles`

Untyped steal handle. Used by `deque_policy` implementations to store handles without knowing the full context type.

### `struct unsafe_sched_handle` — `:handles`

Untyped schedule handle. Used when type-erasing across context types.

### `struct steal_handle<Context>` — `:handles`

Typed handle to a task suspended at a fork point; passed to `context.push()` and returned by `context.pop()` / `context.steal()`.

### `struct sched_handle<Context>` — `:handles`

Typed handle to a task ready to be started or resumed; passed to `scheduler::post()` and `execute()`.

---

## Scope and task operations

### `constexpr auto scope() -> scope_type` — `:ops`

Primary entry point for fork/call/join. `co_await` this to obtain a `scope_ops<Context>` which provides:

- `.fork(ret, fn, args...)` / `.fork(fn, args...)` / `.fork_drop(fn, args...)` — spawn concurrent child
- `.call(ret, fn, args...)` / `.call(fn, args...)` / `.call_drop(fn, args...)` — inline child call
- `.join()` — wait for all outstanding children

### `constexpr auto child_scope() -> child_scope_type` — `:ops`

Entry point for a cancellable scope. `co_await` this to obtain a `child_scope_ops<Context>`, which extends `scope_ops` and also inherits from `stop_source`. Tasks forked/called through this scope receive the scope's stop token.

---

## Cancellation

### `class stop_source` — `:stop`

A non-copyable, non-movable stop source. Methods: `token()`, `stop_possible()`, `stop_requested()`, `request_stop()`, `race_request_stop()`.

### `class stop_source::stop_token` — `:stop`

Lightweight copyable token. Methods: `stop_possible()`, `stop_requested()`.

---

## Scheduling

### `class recv_state<T, Stoppable = false>` — `:receiver`

Pre-allocated shared state for a root task. Constructors mirror `make_shared` / `allocate_shared`:

```cpp
recv_state<int> s;                                   // default-init
recv_state<int> s{42};                               // in-place init
recv_state<int> s{std::allocator_arg, alloc};        // custom allocator
recv_state<int> s{std::allocator_arg, alloc, 42};    // custom allocator + in-place init
recv_state<int, true> s;                             // cancellable variant
```

Move-only. Pass to `schedule()` to get back a `receiver`.

### `class receiver<T, Stoppable = false>` — `:receiver`

Handle to the result of a scheduled root task. Methods:

- `.valid()` — whether the receiver is connected to state
- `.ready()` — whether the task has completed
- `.wait()` — block until complete (may be called multiple times)
- `.stop_source()` — access the stop source (only when `Stoppable = true`)
- `.get()` — consume the result, rethrowing any stored exception; throws `operation_cancelled_error` if cancelled

### `auto schedule(Sch&&, recv_state<R,S>, Fn&&, Args&&...) -> receiver<R,S>` — `:schedule`

Schedule an async function as a root task using a pre-allocated `recv_state`.

### `auto schedule(Sch&&, Fn&&, Args&&...) -> receiver<R>` — `:schedule`

Convenience overload: default-constructs a non-cancellable `recv_state<R>`.

### `void execute(Context&, sched_handle<Context>)` — `:execute`

Bind the calling thread to `context` and resume the scheduled task. Used by scheduler implementations.

### `void execute(Context&, steal_handle<Context>)` — `:execute`

Bind the calling thread to `context` and resume a stolen task. Used by scheduler implementations.

---

## Polymorphic context base classes

### `class base_context<Stack>` — `:poly_context`

CRTP base providing `stack()` -> `Stack&`. Inherit from this (or `poly_context`) when implementing a custom context.

### `class poly_context<Stack>` — `:poly_context`

Abstract base for polymorphic contexts. Provides pure-virtual `push(steal_handle<poly_context>)`, `pop()`, and a defaulting `post(sched_handle<poly_context>)` that throws `post_error`.

---

## Exception hierarchy

All exceptions derive from `lf::libfork_exception : std::exception`.

| Type                        | Thrown by              | Condition                                |
| --------------------------- | ---------------------- | ---------------------------------------- |
| `libfork_exception`         | —                      | Base type; catch-all for libfork errors  |
| `schedule_error`            | `schedule()`           | Called from a worker thread              |
| `execute_error`             | `execute()`            | Called from a worker thread              |
| `steal_overflow_error`      | `execute()`            | A single task stolen > 65,535 times      |
| `root_alloc_error`          | `schedule()`           | Root frame too large for inline buffer   |
| `broken_receiver_error`     | `receiver` methods     | Receiver is in an invalid state          |
| `operation_cancelled_error` | `receiver::get()`      | Task was cancelled via stop token        |
| `post_error`                | `poly_context::post()` | Derived context does not override `post` |
| `deque_full_error`          | `deque::push()`        | Deque has reached maximum capacity       |

---

## Batteries: stacks

All stacks satisfy `worker_stack`. Template parameter is an allocator for `std::byte`.

### `class geometric_stack<Allocator>` — `:geometric_stack`

Segmented stack with geometric growth and segment caching. Recommended default.

### `class adaptor_stack<Allocator>` — `:adaptor_stack`

Thin allocator-backed stack; allocates/deallocates on every push/pop.

### `class slab_stack<Allocator>` — `:slab_stack`

Fixed-capacity slab stack; throws on overflow.

---

## Batteries: deque and adaptors

### `class deque<T, Allocator>` — `:deque`

Lock-free Chase-Lev work-stealing deque. `T` must be `lock_free` and `default_initializable`. Methods: `push(T)`, `pop() -> std::optional<T>`, `get_thief() -> thief_handle`.

### `class deque::thief_handle` — `:deque`

Non-owning steal handle obtained via `deque::get_thief()`. Method: `steal(Fn on_empty) -> std::optional<T>`.

### `enum class err` — `:deque`

Return code from low-level steal operations: `none`, `lost`, `empty`.

### `struct steal_t<T>` — `:deque`

Steal result wrapper returned by `thief_handle::steal`. Has `err code` and `T val` fields; `operator bool` tests `code == err::none`.

### `class adapt_vector<Allocator>` — `:adaptors`

`std::vector`-backed LIFO deque policy. Satisfies `deque_policy`.

### `class adapt_deque<Allocator>` — `:adaptors`

Lock-free deque-backed policy. Satisfies both `deque_policy` and `stealable_deque_policy`.

---

## Batteries: context policies and contexts

### `concept deque_policy<T>` — `:contexts`

A type that is a LIFO stack over `unsafe_steal_handle` (has `push` and `pop`).

### `concept stealable_deque_policy<T>` — `:contexts`

Extends `deque_policy` with a `steal() -> unsafe_steal_handle` method for FIFO work stealing.

### `class mono_context<Stack, Deque>` — `:contexts`

Monomorphic worker context. Composes a `worker_stack` and a `deque_policy`. Satisfies `worker_context<mono_context>`. Exposes `steal()` when `Deque` satisfies `stealable_deque_policy`.

### `class derived_poly_context<Stack, Deque>` — `:contexts`

Polymorphic worker context. Derives from `poly_context<Stack>` and implements `push`/`pop` via `Deque`. Exposes `steal()` when `Deque` satisfies `stealable_deque_policy`. The `context_type` alias is `poly_context<Stack>`.

---

## Schedulers

### `concept derived_worker_context<Context>` — `:inline_scheduler`

`Context` has a `context_type` typedef and is derived from it (i.e., it is a concrete subclass of its own context type).

### `class inline_scheduler<Context>` — `:inline_scheduler`

Single-threaded synchronous scheduler. Stores one `Context` instance; `post()` calls `execute()` directly on the calling thread.

### `enum class pool_kind` — `:basic_busy_pool`

`mono` — uses `mono_context`; `poly` — uses `derived_poly_context`.

### `class basic_busy_pool<Kind, Stack, Deque, Alloc>` — `:basic_busy_pool`

Work-stealing thread pool using busy-wait. Spawns `N` worker threads (default: `std::thread::hardware_concurrency()`). Constructor: `basic_busy_pool(n_threads)`.

### `using mono_busy_pool<Stack, Deque, Alloc>` — `:basic_busy_pool`

Alias for `basic_busy_pool<pool_kind::mono, ...>`.

### `using poly_busy_pool<Stack, Deque, Alloc>` — `:basic_busy_pool`

Alias for `basic_busy_pool<pool_kind::poly, ...>`.
