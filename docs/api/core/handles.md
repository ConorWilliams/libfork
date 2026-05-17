---
icon: lucide/key-round
---

# Handles

Handles are lightweight wrappers around libfork coroutine frames. They are used
by context and scheduler implementations, not by ordinary task code.

All handle types are nullable and testable with explicit `operator bool`.

## `unsafe_steal_handle`

```cpp
struct unsafe_steal_handle {
  constexpr unsafe_steal_handle() = default;
  explicit constexpr operator bool() const noexcept;
  auto operator==(unsafe_steal_handle const&) const noexcept -> bool = default;
};
```

An untyped handle to a stealable continuation. It exists for erased storage
policies such as generic deques.

Prefer [`steal_handle<T>`](#steal_handle) whenever the context type is known.

## `unsafe_sched_handle`

```cpp
struct unsafe_sched_handle {
  constexpr unsafe_sched_handle() = default;
  explicit constexpr operator bool() const noexcept;
  auto operator==(unsafe_sched_handle const&) const noexcept -> bool = default;
};
```

An untyped handle to scheduled work. It exists for erased storage policies.

Prefer [`sched_handle<T>`](#sched_handle) whenever the context type is known.

## `steal_handle`

```cpp
template <typename T>
struct steal_handle : unsafe_steal_handle {
  using unsafe_steal_handle::unsafe_steal_handle;
};
```

A typed handle to a continuation that may be resumed with:

```cpp
lf::execute(context, handle);
```

The coroutine behind a `steal_handle<T>` is suspended at a fork point. Contexts
store these handles in LIFO order and thieves may take them through a stealing
policy.

## `sched_handle`

```cpp
template <typename T>
struct sched_handle : unsafe_sched_handle {
  using unsafe_sched_handle::unsafe_sched_handle;
};
```

A typed handle to scheduled work. The coroutine behind a `sched_handle<T>` is
either a not-yet-started root task or a task suspended at a custom
context-switching awaitable.

Schedulers receive `sched_handle<context_type>` in `post`, store it as needed,
and eventually resume it with `execute`.

## Safety

Handles do not own coroutine frames. They are only valid under the protocol that
created them:

- `sched_handle<T>` must eventually be resumed by a scheduler compatible with
  `T`;
- `steal_handle<T>` must be treated as consumed once a worker starts executing
  it;
- untyped handles should only be used inside storage adapters that restore the
  correct typed handle before execution.
