---
icon: lucide/inbox
---

# Receivers

Receivers are the completion handles returned by
[`schedule`](scheduling.md#schedule). They are intentionally separate from
tasks: a `task` belongs to the coroutine tree, while a `receiver` belongs to the
outside caller waiting for a root task.

## `recv_state`

```cpp
template <typename T, bool Stoppable = false>
class recv_state {
 public:
  recv_state();

  template <typename... Args>
    requires std::constructible_from</*state*/, Args...>
  explicit recv_state(Args&&... args);

  template <simple_allocator Alloc>
  recv_state(std::allocator_arg_t, Alloc const& alloc);

  template <simple_allocator Alloc, typename... Args>
    requires std::constructible_from</*state*/, Args...>
  recv_state(std::allocator_arg_t, Alloc const& alloc, Args&&... args);

  recv_state(recv_state&&) noexcept;
  auto operator=(recv_state&&) noexcept -> recv_state&;

  recv_state(recv_state const&) = delete;
  auto operator=(recv_state const&) -> recv_state& = delete;
};
```

`recv_state` owns the shared state used by a scheduled root task and its
receiver. It contains the result storage, exception storage, ready flag, optional
root stop source, and the embedded root-frame buffer.

Most users can rely on the convenience `schedule` overload, which creates a
non-stoppable state automatically:

```cpp
auto recv = lf::schedule(pool, root_task{});
```

Construct `recv_state` yourself when you need stoppability, allocator-aware
state allocation, or a non-default initial result value:

```cpp
lf::recv_state<int, true> state{0};
auto recv = lf::schedule(pool, std::move(state), root_task{});
```

`recv_state` is move-only and is consumed by `schedule`.

## `receiver`

```cpp
template <typename T, bool Stoppable = false>
class receiver {
 public:
  receiver(receiver&&) noexcept;
  auto operator=(receiver&&) noexcept -> receiver&;

  receiver(receiver const&) = delete;
  auto operator=(receiver const&) -> receiver& = delete;

  auto valid() const noexcept -> bool;
  auto ready() const -> bool;
  void wait() const;

  auto stop_source() -> stop_source& requires Stoppable;

  auto get() && -> T;
};
```

`receiver<T, Stoppable>` is a move-only handle to scheduled root-task
completion.

### `valid`

```cpp
auto valid() const noexcept -> bool;
```

Returns whether this receiver still refers to shared state. A receiver becomes
invalid after `std::move(receiver).get()`.

### `ready`

```cpp
auto ready() const -> bool;
```

Returns whether the root task has completed, either with a value, with an
exception, or through cancellation. Throws `broken_receiver_error` if the
receiver is invalid.

### `wait`

```cpp
void wait() const;
```

Blocks until the root task completes. `wait` may be called multiple times.
Throws `broken_receiver_error` if the receiver is invalid.

### `stop_source`

```cpp
auto stop_source() -> lf::stop_source& requires Stoppable;
```

Returns the root stop source for stoppable receivers. Requesting stop prevents
not-yet-started cancellable work from running and causes cancellation-aware join
paths to stop resuming cancelled subtrees.

```cpp
lf::recv_state<void, true> state;
auto recv = lf::schedule(pool, std::move(state), root_task{});
recv.stop_source().request_stop();
```

### `get`

```cpp
auto get() && -> T;
```

Waits for completion, consumes the receiver state, and returns the result. For
`T = void`, it returns nothing.

If the task completed with an exception, `get` rethrows it. If `Stoppable` is
`true` and the receiver's stop source has been requested, `get` throws
`operation_cancelled_error`.

!!! warning
    `get` is rvalue-qualified and may only be called once:

    ```cpp
    auto value = std::move(recv).get();
    ```

## `broken_receiver_error`

```cpp
struct broken_receiver_error final : libfork_exception {
  auto what() const noexcept -> const char* override;
};
```

Thrown by `ready`, `wait`, or `stop_source` when called on an invalid receiver.

## `operation_cancelled_error`

```cpp
struct operation_cancelled_error final : libfork_exception {
  auto what() const noexcept -> const char* override;
};
```

Thrown by `std::move(receiver).get()` for a stoppable receiver whose root stop
source has been requested.
