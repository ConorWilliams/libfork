---
icon: lucide/cpu
---

# Contexts

Contexts are the worker-local objects used by `execute`. They own a coroutine
frame stack and a LIFO collection of stealable continuations.

The core module exposes reusable context base classes. Concrete contexts are in
`libfork.batteries` and `libfork.schedulers`, but custom schedulers can build on
these types directly.

## `base_context`

```cpp
template <worker_stack Stack>
class base_context {
 public:
  auto stack() noexcept -> Stack&;

 protected:
  constexpr base_context();

  template <typename... Args>
    requires std::constructible_from<Stack, Args...>
  explicit constexpr base_context(Args&&... args);
};
```

`base_context` stores a worker stack and exposes it through `stack()`. It is a
convenience base for context implementations that want stack storage without
committing to a particular queue or scheduler policy.

```cpp
class my_context : public lf::base_context<my_stack> {
 public:
  using base_context::base_context;

  void push(lf::steal_handle<my_context>);
  auto pop() noexcept -> lf::steal_handle<my_context>;
};
```

`base_context<Stack>` does not itself model
[`worker_context`](concepts.md#worker-contexts); derived types must add
`push` and `pop`.

## `poly_context`

```cpp
template <worker_stack Stack>
class poly_context : public base_context<Stack> {
 public:
  using base_context<Stack>::base_context;

  virtual void push(steal_handle<poly_context>) = 0;
  virtual auto pop() noexcept -> steal_handle<poly_context> = 0;
  virtual void post(sched_handle<poly_context> handle);

  virtual ~poly_context() noexcept = default;
};
```

`poly_context` is the standard polymorphic context base. It is polymorphic over
`push`, `pop`, and optionally `post`, while the stack type remains part of the
static type.

The default `post` throws [`post_error`](#post_error). Scheduler-like derived
contexts override it when they can accept externally scheduled root work.

Use `poly_context` when a scheduler or adapter needs dynamic dispatch but still
wants libfork's typed handle discipline.

## `post_error`

```cpp
struct post_error final : libfork_exception {
  auto what() const noexcept -> const char* override;
};
```

Thrown by `poly_context::post` when a derived context does not override posting.
