---
icon: lucide/triangle-alert
---

# Exceptions

Core exceptions derive from `libfork_exception`.

## `libfork_exception`

```cpp
struct libfork_exception : std::exception {};
```

The base class for exceptions thrown by `libfork.core`.

## Derived exceptions

| Exception | Thrown by |
| --- | --- |
| `schedule_error` | `schedule` called from a worker thread already bound to the same context type |
| `execute_error` | `execute` called recursively on a thread already bound to the same context type |
| `steal_overflow_error` | a single task exceeds libfork's internal steal counter |
| `root_alloc_error` | a root coroutine frame exceeds the receiver state's embedded buffer |
| `broken_receiver_error` | receiver operations on an invalid receiver |
| `operation_cancelled_error` | `receiver<T, true>::get()` after cancellation was requested |
| `post_error` | default `poly_context::post` implementation |

See the pages for [scheduling](scheduling.md), [receivers](receiver.md), and
[contexts](context.md) for the exact operation-level behavior.
