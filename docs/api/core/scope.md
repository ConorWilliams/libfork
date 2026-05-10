---
icon: lucide/git-branch
---

# Scopes

```cpp
[[nodiscard]]
constexpr auto scope() noexcept;

[[nodiscard]]
constexpr auto child_scope() noexcept;
```

Scopes are acquired inside a libfork task:

```cpp
auto sc = co_await lf::scope();
```

The returned scope object is the public way to create children and join them.
The exact scope type is intentionally unnamed, but its member functions are part
of the API.

## `scope`

`scope()` creates a normal child-launching scope. It inherits the current task's
stop token.

```cpp
auto sc = co_await lf::scope();
```

The scope object is immovable and should be used locally. It exposes `fork`,
`fork_drop`, `call`, `call_drop`, and `join`.

## `child_scope`

`child_scope()` creates a normal scope plus a new embedded
[`stop_source`](cancellation.md#stop_source). Children launched from the scope
receive that stop source's token, chained to the parent's token.

```cpp
auto sc = co_await lf::child_scope();
sc.request_stop();
co_await sc.join();
```

Because the returned object derives from `stop_source`, it also exposes:

```cpp
auto token() const noexcept -> stop_source::stop_token;
auto stop_requested() const noexcept -> bool;
auto request_stop() noexcept -> void;
auto race_request_stop() noexcept -> bool;
```

## `fork`

```cpp
template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
auto fork(R* ret, Fn&& fn, Args&&... args) noexcept;

template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
auto fork(Fn&& fn, Args&&... args) noexcept;
```

`fork` creates a child task and makes the parent continuation stealable. The
child starts running immediately on the current worker. The parent continues
later, either on this worker or on a worker that stole the continuation.

Use the pointer overload to receive a non-void result:

```cpp
template <lf::worker_context Context>
auto parent(lf::env<Context>) -> lf::task<int, Context> {
  int value = 0;
  auto sc = co_await lf::scope();

  co_await sc.fork(&value, child{});
  co_await sc.join();

  co_return value;
}
```

The pointed-to object must remain alive until `join` completes. That is
normally achieved by using a local variable in the parent task.

## `fork_drop`

```cpp
template <typename... Args, async_invocable<Context, Args...> Fn>
auto fork_drop(Fn&& fn, Args&&... args) noexcept;
```

`fork_drop` launches a child and discards its result. It accepts both `void` and
non-void child tasks.

```cpp
co_await sc.fork_drop(write_log{}, item);
```

## `call`

```cpp
template <typename R, typename... Args, async_invocable_to<R, Context, Args...> Fn>
auto call(R* ret, Fn&& fn, Args&&... args) noexcept;

template <typename... Args, async_invocable_to<void, Context, Args...> Fn>
auto call(Fn&& fn, Args&&... args) noexcept;
```

`call` invokes a child task as a direct child but does not make the parent
continuation stealable. It behaves like an async function call in the fork-join
tree.

Use it when the parent cannot profitably continue in parallel with the child:

```cpp
int value = 0;
co_await sc.call(&value, child{}, input);
```

## `call_drop`

```cpp
template <typename... Args, async_invocable<Context, Args...> Fn>
auto call_drop(Fn&& fn, Args&&... args) noexcept;
```

`call_drop` is the direct-call equivalent of `fork_drop`.

## `join`

```cpp
auto join() noexcept;
```

`join` waits for all forked children in the current scope to complete. A task
that forks children must join before returning.

```cpp
co_await sc.join();
```

If no child continuation was stolen, `join` is a fast local operation. If one or
more continuations were stolen, `join` participates in the join race and may
suspend the current coroutine until the last child completes.

## Exceptions

Exceptions thrown by children are stashed in the parent and rethrown from
`join`. If several children throw, libfork preserves one exception.

If a task is already cancelled when a child would start, the child frame is
destroyed without running. Exceptions already recorded in a cancelled subtree
may be dropped while cancellation unwinds that subtree.

## Choosing `fork` or `call`

Use `fork` when the parent has useful work that can run in parallel with the
child or when multiple independent children should race toward a later `join`.

Use `call` when the parent needs the result immediately and exposing the parent
continuation as stealable work would only add scheduler traffic.
