---
icon: lucide/network
---

# Schedulers

Schedulers map libfork task handles to worker contexts and OS threads. User
task code is independent of the scheduler as long as it satisfies the
`scheduler` concept.

## Inline scheduler

### `concept derived_worker_context<Context>`

`Context` has a `context_type` alias and derives from that context type. This is
the shape required by `inline_scheduler`.

### `inline_scheduler<Context>`

Single-threaded scheduler that owns one context. `post` immediately calls
`execute` on the calling thread.

Constructor forms:

- `inline_scheduler()`
- `explicit inline_scheduler(Args&&...)`, forwarded to `Context`

Public API:

- `using context_type = Context::context_type`
- `post(sched_handle<context_type>) -> void`

Inline schedulers are useful for tests, debugging, and measuring stack/context
overhead without worker-thread scheduling.

### `mono_inline_scheduler<Stack, Deque>`

Alias for:

```cpp
lf::inline_scheduler<lf::mono_context<Stack, Deque>>
```

### `poly_inline_scheduler<Stack, Deque>`

Alias for:

```cpp
lf::inline_scheduler<lf::derived_poly_context<Stack, Deque>>
```

## Busy pool

### `enum class pool_kind`

Selects the context implementation used by `basic_busy_pool`.

- `pool_kind::mono`
- `pool_kind::poly`

### `basic_busy_pool<Kind, Stack, Deque = adapt_deque<>, Alloc = std::allocator<std::byte>>`

Busy-waiting work-stealing thread pool. It creates `n` `std::jthread` workers,
posts root tasks into a shared queue, and lets idle workers steal from other
worker contexts.

Constructor:

```cpp
explicit basic_busy_pool(
    std::size_t n = std::thread::hardware_concurrency(),
    Alloc const& alloc = Alloc());
```

Public API:

- `using context_type`
- `post(sched_handle<context_type>) -> void`

The pool is non-copyable and non-movable. Destruction requests stop on all
workers and joins them through `std::jthread`.

!!! note

    `basic_busy_pool` currently busy-waits for work. It is useful for benchmark
    and development scenarios where low latency matters more than idle power.

### `mono_busy_pool<Stack, Deque = adapt_deque<>, Alloc = std::allocator<std::byte>>`

Alias for:

```cpp
lf::basic_busy_pool<lf::pool_kind::mono, Stack, Deque, Alloc>
```

This is the usual pool shape for concrete contexts:

```cpp
lf::mono_busy_pool<lf::geometric_stack<>> pool{4};
```

### `poly_busy_pool<Stack, Deque = adapt_deque<>, Alloc = std::allocator<std::byte>>`

Alias for:

```cpp
lf::basic_busy_pool<lf::pool_kind::poly, Stack, Deque, Alloc>
```

Use the polymorphic variant when scheduler/context code needs the
`poly_context<Stack>` abstraction.
