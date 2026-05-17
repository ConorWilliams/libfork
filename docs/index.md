---
icon: lucide/utensils
---

# libfork

`libfork` is a C++ coroutine-tasking library for strict fork-join parallelism.
It gives programs a small async-function vocabulary, a scheduler-independent
execution model, and worker stacks designed for fine-grained parallel tasks.

At the top level, users import the library as a C++ module:

```cpp
import libfork;
```

The core idea is simple: a task may fork child tasks, continue with local work,
and then join before reading child results or returning to its own parent. The
runtime uses continuation stealing: the worker that performs a fork continues
with the child, while another worker may steal the parent continuation.

```cpp
import std;
import libfork;

struct fib {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, std::int64_t n)
      -> lf::task<std::int64_t, Context> {
    if (n < 2) {
      co_return n;
    }

    std::int64_t lhs = 0;
    std::int64_t rhs = 0;

    auto sc = co_await lf::scope();
    co_await sc.fork(&rhs, fib{}, n - 2);
    co_await sc.call(&lhs, fib{}, n - 1);
    co_await sc.join();

    co_return lhs + rhs;
  }
};

auto main() -> int {
  lf::mono_busy_pool<lf::geometric_stack<>> pool{4};
  auto result = lf::schedule(pool, fib{}, 20).get();
  return result == 6765 ? 0 : 1;
}
```

## Start here

- [Installation](installation.md) covers prerequisites, configuration,
  building, and a first program.
- [Quickstart](quickstart.md) explains the fork-join model, scheduling, cancellation,
  exceptions, algorithms, and the stack model.
- [API reference](api/index.md) documents the exported `libfork` modules.
- [Benchmarks](benchmarks/index.md) describes the benchmark suite.
- [Contributing](contributing.md) lists the local development workflow.

## Design in one page

`libfork` tasks are C++ coroutines returning `lf::task<T, Context>`. The first
argument is normally `lf::env<Context>`, which lets libfork pass context through
the task graph without constructing user-visible runtime objects.

Tasks run inside a strict fork-join tree. A fork starts a child that may run in
parallel with the parent continuation. A call starts a child inline and is useful
when there is no profitable continuation left to steal. A join waits for all
outstanding children in the current scope.

Schedulers are separate from task code. The same task can run on the synchronous
`inline_scheduler`, a monomorphic busy-waiting pool, or a polymorphic pool. The
default practical choice for parallel work today is:

```cpp
using pool_type = lf::mono_busy_pool<lf::geometric_stack<>>;
```

The module surface is intentionally split:

- `libfork.core` defines tasks, scopes, scheduling, receivers, cancellation,
  contexts, handles, projections, and concepts.
- `libfork.batteries` provides worker stacks, deques, context policies, and
  context implementations.
- `libfork.schedulers` provides scheduler implementations.
- `libfork.algorithm` provides higher-level fork-join algorithms such as
  `for_each` and `fold`.
