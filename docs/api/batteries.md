---
icon: lucide/battery-charging
---

# Batteries

Batteries provide ready-to-use stacks, deques, context policies, and worker
contexts.

## Stacks

All stack classes are allocator-aware templates over `std::byte` allocators and
satisfy `worker_stack` when the allocator's void pointer type is `void*`.

### `geometric_stack<Allocator = std::allocator<std::byte>>`

Segmented user-space stack with geometric growth and one cached segment to
avoid hot splitting. This is the general-purpose stack choice.

Main operations:

- `empty() const noexcept -> bool`
- `checkpoint() noexcept`
- `push(std::size_t) -> void*`
- `pop(void*, std::size_t) noexcept -> void`
- `prepare_release() noexcept`
- `release(release_t) noexcept`
- `acquire(checkpoint_t) noexcept`

### `slab_stack<Allocator = std::allocator<std::byte>>`

Fixed-size slab-backed stack. It allocates one slab at construction and throws
`std::bad_alloc` if a push exceeds the slab.

Constructors:

- `slab_stack()`
- `explicit slab_stack(diff_type num_nodes, Allocator const& = Allocator())`

Main operations match `geometric_stack`.

### `adaptor_stack<Allocator = std::allocator<std::byte>>`

Thin wrapper over an allocator. Every push allocates and every pop deallocates.
It is simple and useful for testing or comparison.

Main operations match `geometric_stack`, except release/acquire only propagate
allocator state when needed.

## Work-stealing deque

### `concept dequeable<T>`

`T` can be stored in `lf::deque`. It must be lock-free and default-initializable.

### `deque_full_error`

Thrown when `deque::push` cannot add another element because the deque is full.

### `enum class err`

Result code for stealing:

- `none`: steal succeeded.
- `lost`: another thief won the race.
- `empty`: the deque was empty.

### `steal_t<T>`

Return type of `thief_handle::steal`. It supports `operator bool`, `operator*`,
and `operator->`.

Fields:

- `err code`
- `T val`

`val` is valid only when `code == err::none`.

### `deque<T, Allocator = std::allocator<std::atomic<T>>>`

Bounded Chase-Lev single-producer multiple-consumer work-stealing deque.

The owner thread uses:

- `empty() const noexcept -> bool`
- `size() const noexcept -> size_type`
- `ssize() const noexcept -> diff_type`
- `capacity() const noexcept -> diff_type`
- `push(T) -> diff_type`
- `pop(Fn when_empty = {}) -> invoke_result_t<Fn>`
- `thief() noexcept -> thief_handle`

Non-owner threads steal through `deque::thief_handle`, which provides:

- `empty() noexcept -> bool`
- `size() noexcept -> size_type`
- `ssize() noexcept -> diff_type`
- `capacity() noexcept -> diff_type`
- `steal() noexcept -> steal_t<T>`

All threads must stop using a deque before it is destroyed.

## Deque adaptors

### `adapt_vector<Allocator = std::allocator<unsafe_steal_handle>>`

`std::vector`-backed LIFO context policy. It supports:

- `push(unsafe_steal_handle)`
- `pop() noexcept -> unsafe_steal_handle`

Use it for inline/single-threaded contexts where stealing is unnecessary.

### `adapt_deque<Allocator = std::allocator<std::atomic<unsafe_steal_handle>>>`

Lock-free deque-backed context policy. It supports:

- `push(unsafe_steal_handle)`
- `pop() noexcept -> unsafe_steal_handle`
- `steal() noexcept -> unsafe_steal_handle`

The default capacity is `32 * 1024` handles. The explicit constructor accepts a
capacity and allocator.

## Context policies

### `concept deque_policy<T>`

A LIFO stack over `unsafe_steal_handle`. Used by contexts to store work without
naming the full context type.

### `concept stealable_deque_policy<T>`

Extends `deque_policy` with:

```cpp
auto steal() -> lf::unsafe_steal_handle;
```

Use a stealable policy for multi-worker schedulers.

## Contexts

### `derived_poly_context<Stack, Deque>`

Polymorphic worker context composed of a `Stack` and `Deque`. It derives from
`poly_context<Stack>`, implements virtual `push`/`pop`, and exposes `steal()`
when `Deque` is stealable.

Aliases:

- `context_type = poly_context<Stack>`

### `mono_context<Stack, Deque>`

Monomorphic worker context composed of a `Stack` and `Deque`. It implements
`push`/`pop` directly and exposes `steal()` when `Deque` is stealable.

Aliases:

- `context_type = mono_context`

Both context classes support piecewise construction of their stack and deque:

```cpp
lf::mono_context<lf::geometric_stack<>, lf::adapt_deque<>> ctx{
    std::piecewise_construct,
    std::tuple{},
    std::tuple{1024}};
```
