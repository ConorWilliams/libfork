---
icon: lucide/blocks
---

# Core

All public symbols documented here live in namespace `lf` and are reachable via:

```cpp
import libfork.core;
```

`libfork.core` is the minimal public module for writing libfork tasks and for
building schedulers, contexts, stacks, and higher-level algorithms. It does not
include concrete schedulers or stack implementations; those are provided by
`libfork.batteries` and `libfork.schedulers`.

Use this module directly when you want the core coroutine protocol without the
standard batteries. Most applications can instead import the meta-module:

```cpp
import libfork;
```

## What core provides

- [Tasks](task.md): `task<T, Context>` and `env<Context>`.
- [Scopes](scope.md): `scope()`, `child_scope()`, `fork`, `call`, and `join`.
- [Scheduling](scheduling.md): `schedule`, `execute`, and root-task errors.
- [Receivers](receiver.md): `recv_state`, `receiver`, waiting, result retrieval,
  and root cancellation.
- [Cancellation](cancellation.md): `stop_source` and `stop_token`.
- [Contexts](context.md): `base_context`, `poly_context`, and `post_error`.
- [Handles](handles.md): typed and untyped task handles.
- [Projections](projected.md): async-aware `projectable` and `projected`.
- [Concepts](concepts.md): the constraints used by the public API.
- [Exceptions](exceptions.md): the common exception hierarchy.

## Execution model

Core tasks are coroutines arranged into a strict fork-join tree. A task may
create child tasks with `fork` or `call`, but those children are always joined
before the parent can return. Libfork uses continuation stealing: a forked child
runs immediately, while the parent's continuation becomes stealable work.

The core module deliberately separates three roles:

- a **task** is a coroutine with a `task<T, Context>` return type;
- a **context** owns the worker-local stack and a LIFO queue of stealable
  continuations;
- a **scheduler** accepts root work and eventually resumes it with `execute`.

That split is what lets `libfork.core` stay independent from any specific
thread pool or work-stealing queue.
