---
icon: lucide/filter
---

# Projections

Libfork algorithms support projections that may be synchronous or asynchronous.
The core module exposes the concepts and aliases used to model those projected
iterator values.

## `projectable`

```cpp
template <typename Fn, typename Context, typename I>
concept projectable = /* ... */;
```

`projectable<Fn, Context, I>` checks that `Fn` can be used as a projection for
the indirectly readable type `I`.

Synchronous projections follow the usual standard-library shape. Async
projections are libfork tasks and may receive `env<Context>`:

```cpp
struct square_async {
  template <lf::worker_context Context>
  auto operator()(lf::env<Context>, int x) const -> lf::task<int, Context> {
    co_return x * x;
  }
};
```

Async projections must also produce default-initializable result types. Libfork
algorithms need scratch storage for async projected values before child tasks
write their results.

## `projected`

```cpp
template <worker_context Context, std::indirectly_readable I, projectable<Context, I> Fn>
using projected = /* exposition-only projected iterator type */;
```

`projected` is libfork's async-aware analogue of `std::projected`. It provides
the associated value and reference types that indirect concepts use when a range
is viewed through a projection.

For synchronous callables, `projected` behaves like the standard projection
machinery. For async callables, its value and reference types come from
[`async_result_t`](concepts.md#async-invocables).

```cpp
using P = lf::projected<Context, std::vector<int>::iterator, square_async>;
```

Most users do not name `projected` directly. It is primarily useful when
writing algorithms that should accept the same sync and async projection forms
as libfork's built-in algorithms.
