---
icon: lucide/octagon-x
---

# Cancellation

Libfork cancellation is cooperative. `stop_source` records a request, and tasks
observe it through stop tokens propagated through scopes. Cancellation never
asynchronously interrupts running user code.

## `stop_source`

```cpp
class stop_source {
 public:
  class stop_token;

  constexpr stop_source() noexcept;
  constexpr explicit stop_source(stop_token parent) noexcept;

  stop_source(stop_source const&) = delete;
  stop_source(stop_source&&) = delete;
  auto operator=(stop_source const&) -> stop_source& = delete;
  auto operator=(stop_source&&) -> stop_source& = delete;

  auto token() const noexcept -> stop_token;
  auto stop_requested() const noexcept -> bool;
  auto request_stop() noexcept -> void;
  auto race_request_stop() noexcept -> bool;
};
```

`stop_source` is a small linked cancellation source. A source may be constructed
with a parent token; `stop_requested()` then checks this source and every parent
source in the chain.

Root stoppability is created with [`recv_state<T, true>`](receiver.md#recv_state).
Nested stoppability is created with [`child_scope()`](scope.md#child_scope).

```cpp
template <lf::worker_context Context>
auto root(lf::env<Context>) -> lf::task<void, Context> {
  auto sc = co_await lf::child_scope();

  co_await sc.fork_drop(worker{});
  sc.request_stop();
  co_await sc.join();
}
```

### `token`

```cpp
auto token() const noexcept -> stop_token;
```

Returns a non-owning token referring to this source. Child scopes store tokens
to propagate cancellation.

### `stop_requested`

```cpp
auto stop_requested() const noexcept -> bool;
```

Returns true if this source or any ancestor source has been stopped. The check
is linear in the parent-chain depth.

### `request_stop`

```cpp
auto request_stop() noexcept -> void;
```

Requests cancellation for this source and its descendants. Calling it more than
once is allowed.

### `race_request_stop`

```cpp
auto race_request_stop() noexcept -> bool;
```

Requests cancellation and returns true only for the first caller that changed
the source from not-stopped to stopped. Use `request_stop()` when the return
value is not needed.

## `stop_source::stop_token`

```cpp
class stop_source::stop_token {
 public:
  constexpr stop_token() noexcept;

  auto stop_possible() const noexcept -> bool;
  auto stop_requested() const noexcept -> bool;
};
```

`stop_token` is a pointer-sized non-owning handle to a stop-source chain. A
default-constructed token is unstoppable.

### `stop_possible`

```cpp
auto stop_possible() const noexcept -> bool;
```

Returns true when the token refers to a stop source.

### `stop_requested`

```cpp
auto stop_requested() const noexcept -> bool;
```

Returns true if any source in the chain has been stopped. A null token always
returns false.

## Propagation rules

Normal [`scope()`](scope.md#scope) children inherit the parent's current stop
token. [`child_scope()`](scope.md#child_scope) inserts a new stop source between
the parent token and any children launched by that scope.

If a child task has not started and its stop token is already stopped, libfork
destroys the child frame without resuming it. At `join`, a cancelled task may
stop resuming the cancelled subtree after required cleanup.
