---
icon: lucide/calendar-clock
---

# Scheduling

Scheduling bridges outside code and libfork tasks. `schedule` creates a root
task and submits it to a scheduler; `execute` resumes a task handle on a worker
context.

## `schedule`

```cpp
template <scheduler Sch, typename R, bool Stoppable, typename Fn, typename... Args>
  requires async_invocable_to<std::decay_t<Fn>, R, context_t<Sch>, std::decay_t<Args>...>
[[nodiscard]]
constexpr auto schedule(
    Sch&& sch,
    recv_state<R, Stoppable> state,
    Fn&& fn,
    Args&&... args) -> receiver<R, Stoppable>;
```

Schedules a root task using caller-provided receiver state. `Fn` and `Args...`
are decayed into the root coroutine frame. The returned
[`receiver`](receiver.md#receiver) observes completion, exceptions, and the
result.

Use this overload when you need a custom allocator for receiver state, an
initial return object value, or a stoppable root task:

```cpp
lf::recv_state<int, true> state;
auto recv = lf::schedule(pool, std::move(state), root_task{}, 42);

recv.stop_source().request_stop();
int result = std::move(recv).get();
```

!!! warning
    `schedule` must not be called from inside a worker thread that is already
    executing the same context type. Doing so throws `schedule_error`.

### Convenience overload

```cpp
template <scheduler Sch, typename Fn, typename... Args>
  requires /* async invocable with default-schedulable result */
[[nodiscard]]
constexpr auto schedule(Sch&& sch, Fn&& fn, Args&&... args)
    -> receiver<async_result_t<std::decay_t<Fn>, context_t<Sch>, std::decay_t<Args>...>>;
```

This overload creates a non-stoppable `recv_state` with the default allocator.
The task result must be `void` or default-initializable and movable.

```cpp
auto recv = lf::schedule(pool, root_task{}, 42);
auto value = std::move(recv).get();
```

## `execute`

```cpp
template <worker_context Context>
constexpr void execute(Context& context, sched_handle<Context> handle);

template <worker_context Context>
constexpr void execute(Context& context, steal_handle<Context> handle);
```

Binds the current thread to `context`, resumes the task represented by
`handle`, and unbinds the thread before returning.

Scheduler implementations call `execute` after taking a handle from their work
source:

```cpp
if (auto h = queue.pop()) {
  lf::execute(context, h);
}
```

The `sched_handle` overload resumes root tasks and tasks suspended by custom
awaitables. The `steal_handle` overload resumes a stolen continuation and marks
the frame as stolen before execution.

!!! warning
    `execute` must not be called recursively on a thread already bound to a
    context of the same type. Doing so throws `execute_error`.

## `schedule_error`

```cpp
struct schedule_error final : libfork_exception {
  auto what() const noexcept -> const char* override;
};
```

Thrown when `schedule` is called from inside a worker thread for the same
context type.

## `execute_error`

```cpp
struct execute_error final : libfork_exception {
  auto what() const noexcept -> const char* override;
};
```

Thrown when `execute` is called while the current thread is already executing a
task for the same context type.

## `steal_overflow_error`

```cpp
struct steal_overflow_error final : libfork_exception {
  auto what() const noexcept -> const char* override;
};
```

Thrown if a single task is stolen enough times to overflow libfork's internal
steal counter.

## `root_alloc_error`

```cpp
struct root_alloc_error final : libfork_exception {
  auto what() const noexcept -> const char* override;
};
```

Thrown when the root coroutine frame does not fit into the buffer embedded in
the receiver state. This usually means the scheduled callable or its arguments
are too large to store directly in the root frame.
