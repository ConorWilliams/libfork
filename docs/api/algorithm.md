---
icon: lucide/workflow
---

# Algorithm

The algorithm module provides fork-join algorithms over sized random-access
ranges. Algorithms are async functions and are normally launched with
`lf::schedule` or called from another task through a scope.

## Concepts

### `sized_random_access_range<T>`

`T` satisfies both `std::ranges::random_access_range` and
`std::ranges::sized_range`.

## `for_each`

### `inline constexpr for_each_impl for_each`

Applies a function to every element in a sized random-access range or
iterator/sentinel pair.

Overload shapes:

```cpp
lf::for_each(env, first, last, chunk_size, fn);
lf::for_each(env, first, last, fn);
lf::for_each(env, range, chunk_size, fn);
lf::for_each(env, range, fn);
```

When `fn` is an async invocable, `for_each` calls it through libfork scopes.
Otherwise it invokes `fn` synchronously.

```cpp
std::vector<int> values{1, 2, 3};
auto recv = lf::schedule(pool, lf::for_each, std::span(values), [](int& x) {
  x *= 2;
});
std::move(recv).get();
```

The chunk-size overload assumes `chunk_size > 0`. Use the overload without a
chunk size for the single-element base case.

## `fold`

### `fold_chunk_error`

Thrown when a public `fold` overload receives a non-positive chunk size.

### `inline constexpr fold_fn fold`

Reduces a sized random-access range or iterator/sentinel pair with a semigroup
operation. The result is `std::optional<R>`: empty input returns `std::nullopt`,
and non-empty input returns the reduced value.

Overload shapes:

```cpp
lf::fold(env, first, last, chunk_size, binary_op, projection = {});
lf::fold(env, first, last, binary_op, projection = {});
lf::fold(env, range, chunk_size, binary_op, projection = {});
lf::fold(env, range, binary_op, projection = {});
```

The binary operation may be synchronous or async. The projection may also be
synchronous or async. The projected value and binary operation must satisfy
libfork's indirect semigroup constraints.

```cpp
std::vector<int> values{1, 2, 3, 4};

auto recv = lf::schedule(pool, lf::fold, std::span(values), std::plus<>{});
std::optional<int> sum = std::move(recv).get();
```

With projection:

```cpp
struct record {
  int value;
};

std::vector<record> records{{1}, {2}, {3}};
auto recv = lf::schedule(
    pool,
    lf::fold,
    std::span(records),
    std::plus<>{},
    &record::value);
```

For overloads with an explicit chunk size, `chunk_size <= 0` throws
`fold_chunk_error` before checking whether the range is empty.

## Async operations

Both algorithms understand libfork async callables. For example, an async
projection can be used with `fold`:

```cpp
struct square {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int value) -> lf::task<int, Context> {
    co_return value * value;
  }
};

auto recv = lf::schedule(
    pool,
    lf::fold,
    std::span(values),
    std::plus<>{},
    square{});
```

If a callable is both synchronously and asynchronously invocable, the algorithm
implementation may prefer the async path where its constraints select it.
