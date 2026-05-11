---
icon: lucide/rocket
---

# Quickstart

This quickstart explains how the current module-based libfork API fits together.

## Fork-join tasks

Libfork models work as a strict fork-join tree. A task may create children, but
it must join those children before returning. This restriction keeps the task
graph structured and lets the runtime move continuations between workers without
requiring users to manage task lifetimes manually.

An async function is any callable that returns `lf::task<T, Context>` when
invoked in a worker context:

```cpp
struct work {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int input) -> lf::task<int, Context> {
    co_return input * 2;
  }
};
```

The `lf::env<Context>` argument is supplied by libfork. It identifies the worker
context type and allows the same callable to be used with different schedulers.

## Fork, call, join

Inside a task, `co_await lf::scope()` returns a scope object with `fork`, `call`,
`fork_drop`, `call_drop`, and `join`.

```cpp
auto sc = co_await lf::scope();
co_await sc.fork(&left, child{}, 1);
co_await sc.call(&right, child{}, 2);
co_await sc.join();
```

`fork` exposes the parent continuation for stealing and immediately starts the
child on the current worker. `call` starts the child inline and is useful when
there is no useful continuation left to steal. `join` waits until all children
created through the scope have finished.

Use the `_drop` variants when the child result is intentionally ignored:

```cpp
co_await sc.fork_drop(side_effect{}, item);
co_await sc.call_drop(cleanup{}, item);
```

## Result storage

Non-void child results are written into caller-provided storage:

```cpp
int result = 0;
co_await sc.fork(&result, compute{}, input);
co_await sc.join();
```

The pointer must remain valid until after the join. A common pattern is to keep
child result variables in the parent coroutine frame and read them only after
`join`.

## Continuation stealing

On a fork, libfork pushes a handle to the parent continuation into the worker
context and runs the child immediately. Another worker may steal that
continuation and resume it. This differs from child-stealing runtimes, where the
new child is normally offered to other workers.

The important user-facing consequence is that execution may resume on a
different OS thread after any `co_await`. Code inside tasks should not assume
thread affinity unless it uses an explicit scheduling awaitable.

## Worker stacks

Coroutine frames created by fork/call are allocated from a worker stack. The
provided stacks trade simplicity, speed, and bounded memory:

- `geometric_stack` is the general-purpose segmented stack.
- `slab_stack` uses a fixed-capacity slab and throws `std::bad_alloc` on
  overflow.
- `adaptor_stack` delegates every push/pop to an allocator.

Schedulers combine a stack with a context policy, such as `adapt_vector` for a
single-threaded inline scheduler or `adapt_deque` for stealing between workers.

## Exceptions

If a child task exits with an exception, libfork stores the exception in the
parent and rethrows it at `join`.

```cpp
auto sc = co_await lf::scope();
co_await sc.fork_drop(may_throw{}, input);
co_await sc.join(); // rethrows if the child failed
```

Because libfork is strict fork-join, task code should structure potentially
throwing work so outstanding children are still joined before the task exits.
When in doubt, join in the same lexical region that created the children.

`receiver::get()` also rethrows exceptions from a scheduled root task.

## Cancellation

Use `lf::child_scope()` to create a scope with its own `stop_source`. Children
launched through that scope inherit its stop token.

```cpp
struct maybe_run {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>) -> lf::task<void, Context> {
    auto sc = co_await lf::child_scope();
    sc.request_stop();
    co_await sc.fork_drop(child_work{});
    co_await sc.join();
  }
};
```

For root tasks, construct `lf::recv_state<T, true>` and pass it to `schedule`.
The returned `lf::receiver<T, true>` exposes `stop_source()`:

```cpp
lf::recv_state<int, true> state;
auto recv = lf::schedule(pool, std::move(state), root_task{});
recv.stop_source().request_stop();
```

When a cancellable receiver is consumed after cancellation,
`receiver::get()` throws `lf::operation_cancelled_error`.

## Explicit scheduling

Libfork supports context-switching awaitables. A type is an `lf::awaitable<T,
Context>` when it can be acquired through `operator co_await` and its awaitable
has:

```cpp
auto await_ready() -> bool;
auto await_suspend(lf::sched_handle<Context>, Context&) -> void;
auto await_resume();
```

`await_suspend` receives a schedulable handle and the current context. It may
post that handle to another scheduler, allowing a task to hop between pools.

## Algorithms

The algorithm module provides fork-join operations over random-access ranges.

`lf::for_each` applies a synchronous or asynchronous function to every element:

```cpp
auto recv = lf::schedule(pool, lf::for_each, std::span(values), [](int& x) {
  x *= 2;
});
std::move(recv).get();
```

`lf::fold` reduces a non-empty range to `std::optional<T>`, returning
`std::nullopt` for empty input:

```cpp
auto recv = lf::schedule(pool, lf::fold, std::span(values), std::plus<>{});
auto sum = std::move(recv).get();
```

Both algorithms accept iterator/sentinel pairs or ranges. Overloads with an
explicit chunk size require that size to be positive.
