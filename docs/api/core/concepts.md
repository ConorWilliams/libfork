---
icon: lucide/puzzle
---

# Concepts

`libfork.core` exposes the concepts used by tasks, schedulers, algorithms, and
custom integrations. They are public API because user-defined schedulers,
contexts, stacks, awaitables, projections, and algorithms are expected to use
the same constraints as libfork itself.

## Returnable

```cpp
template <typename T>
concept returnable = std::is_void_v<T> || (/*plain-object*/<T> && std::movable<T>);
```

Types suitable for use as the `T` in [`task<T, Context>`](task.md). `void` is
allowed. Non-void return types must be movable plain object types.

### Plain-object

An exposition-only concept used throughout libfork:

```cpp
template <typename T>
concept /*plain-object*/ =
    std::is_object_v<T> &&
    std::same_as<T, std::remove_reference_t<T>>;
```

## Worker stacks

```cpp
template <typename T>
concept worker_stack = /* ... */;
```

`worker_stack` defines the stack API used to allocate coroutine frames. A stack
implementation must provide:

```cpp
auto push(std::size_t bytes) -> void*;
auto pop(void* ptr, std::size_t bytes) noexcept -> void;
auto checkpoint() noexcept -> std::regular auto;
auto prepare_release() noexcept -> std::movable auto;
auto release(decltype(prepare_release())) noexcept -> void;
auto acquire(decltype(checkpoint()) const&) noexcept -> void;
```

The fast path is `push`, `pop`, and `checkpoint`. `prepare_release`, `release`,
and `acquire` are used when continuation stealing transfers stack ownership
between workers.

Stack checkpoints must be cheap to copy. A default-constructed checkpoint is a
null state that only compares equal to itself. Non-null checkpoints compare
equal when they refer to the same underlying stack allocation source.

## Worker contexts

```cpp
template <typename T, typename U>
concept lifo_stack = /* ... */;

template <typename T>
concept worker_context = /* ... */;

template <worker_context T>
using stack_t = std::remove_reference_t<decltype(std::declval<T&>().stack())>;
```

`lifo_stack<T, U>` requires:

```cpp
context.push(value); // -> void
context.pop();       // noexcept -> U
```

`worker_context<T>` refines that shape for `steal_handle<T>` and also requires
access to a worker stack:

```cpp
auto stack() noexcept -> Stack&; // Stack models worker_stack
```

Contexts are the per-worker execution state. They store stealable
continuations, expose the coroutine frame stack, and are temporarily bound to a
thread while `execute` resumes a task.

## Schedulers

```cpp
template <typename T>
concept has_context_typedef = requires {
  typename std::remove_cvref_t<T>::context_type;
};

template <has_context_typedef T>
using context_t = typename std::remove_cvref_t<T>::context_type;

template <typename Sch>
concept scheduler = /* ... */;
```

A scheduler provides a `context_type` and can post a scheduled root task:

```cpp
void post(lf::sched_handle<context_type>);
```

`post` must provide the strong exception guarantee. If it returns normally, the
task associated with the handle must eventually be resumed by a worker using
[`execute`](scheduling.md#execute).

## Async invocables

```cpp
template <typename Fn, typename Context, typename... Args>
concept async_invocable = /* ... */;

template <typename Fn, typename Context, typename... Args>
concept async_regular_invocable = async_invocable<Fn, Context, Args...>;

template <typename Fn, typename Context, typename... Args>
concept async_nothrow_invocable = /* ... */;

template <typename Fn, typename Context, typename... Args>
using async_result_t = /* task value_type */;

template <typename Fn, typename R, typename Context, typename... Args>
concept async_invocable_to = /* ... */;

template <typename Fn, typename R, typename Context, typename... Args>
concept async_nothrow_invocable_to = /* ... */;
```

`async_invocable` checks whether `Fn` can be called as a libfork task for the
given `Context` and arguments. The result must be `task<R, Context>` for some
`R`.

When overload resolution is checked, libfork first tries to call:

```cpp
fn(lf::env<Context>{}, args...);
```

If that is not viable, it checks:

```cpp
fn(args...);
```

This is what makes [`env<Context>`](env.md) useful for context-generic tasks.
`async_result_t` is the `value_type` of the returned task.

## Awaitables

```cpp
template <awaitable_acquirable T>
constexpr auto acquire_awaitable(T&& t);

template <typename T, typename Context>
concept awaitable = /* ... */;
```

`acquire_awaitable` implements the same acquisition rule used by `co_await`:

- if `t.operator co_await()` is available, use it;
- otherwise, if `operator co_await(t)` is available, use it;
- otherwise, treat `t` itself as the awaitable.

`awaitable<T, Context>` is libfork's context-switching awaitable concept. The
acquired awaitable must be storable and provide:

```cpp
auto await_ready() -> bool-convertible;
auto await_suspend(lf::sched_handle<Context>, Context&) -> void;
auto await_resume();
```

!!! warning
    `await_suspend` must not complete the same coroutine inline. A custom
    awaitable hands the suspended task to another scheduling path, which must
    later resume it with `execute`.

## Indirect invocables

```cpp
namespace sync {
  template <typename Fn, typename I>
  concept indirectly_unary_invocable = /* ... */;

  template <typename Fn, typename I>
  concept indirectly_regular_unary_invocable = /* ... */;
}

namespace async {
  template <typename Fn, typename Context, typename I>
  concept indirectly_unary_invocable = /* ... */;

  template <typename Fn, typename Context, typename I>
  concept indirectly_regular_unary_invocable = /* ... */;
}

template <typename Fn, typename Context, typename I>
concept indirectly_unary_invocable =
    sync::indirectly_unary_invocable<Fn, I> ||
    async::indirectly_unary_invocable<Fn, Context, I>;

template <typename Fn, typename Context, typename I>
concept indirectly_regular_unary_invocable =
    sync::indirectly_regular_unary_invocable<Fn, I> ||
    async::indirectly_regular_unary_invocable<Fn, Context, I>;
```

These concepts mirror the standard indirect callable concepts, but also support
libfork's async projections and the `indirect_value_t` customization used by
[`projected`](projected.md).

The combined concepts accept either a synchronous callable or an asynchronous
callable. If both are viable, libfork algorithms generally prefer the async
form.

## Semigroups

```cpp
namespace sync {
  template <typename Fn, typename I>
  concept indirect_semigroup = /* ... */;
}

namespace async {
  template <typename Fn, typename Context, typename I>
  concept indirect_semigroup = /* ... */;
}

template <typename Fn, typename Context, typename I>
concept indirect_semigroup =
    sync::indirect_semigroup<Fn, I> ||
    async::indirect_semigroup<Fn, Context, I>;

template <typename Fn, typename Context, typename I>
concept indirect_commutative_semigroup = indirect_semigroup<Fn, Context, I>;

template <typename Fn, typename Context, typename I>
using indirect_semigroup_t = /* operation result type */;
```

An indirect semigroup is an indirectly readable input plus an associative binary
operation that is closed over all combinations of:

- the projected indirect value type;
- the iterator reference type;
- the operation result type.

The async variant requires those combinations to be valid libfork task
invocations. `indirect_commutative_semigroup` is a semantic refinement: the
type system cannot prove commutativity, so users are responsible for supplying
an operation where `a op b == b op a`.

`indirect_semigroup_t` returns the result type of applying the operation to two
elements.

## Projections

```cpp
template <typename Fn, typename Context, typename I>
concept projectable = /* ... */;

template <worker_context Context, std::indirectly_readable I, projectable<Context, I> Fn>
using projected = /* ... */;
```

See [Projections](projected.md) for the public projection alias and the extra
default-initialization rule for async projections.
