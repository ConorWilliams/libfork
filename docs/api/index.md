---
icon: lucide/book-open
---

# API reference

All public symbols documented here live in namespace `lf` and are reachable via:

```cpp
import libfork;
```

The meta-module re-exports:

- `libfork.core`
- `libfork.batteries`
- `libfork.schedulers`
- `libfork.algorithm`

## Sections

- [Core](core.md): tasks, scopes, receivers, scheduling, cancellation, handles,
  contexts, concepts, and exceptions.
- [Batteries](batteries.md): worker stacks, deque, context policies, and context
  implementations.
- [Schedulers](schedulers.md): inline and busy-pool schedulers.
- [Algorithm](algorithm.md): fork-join algorithms over random-access ranges.

`libfork.utils` is a support module and is not documented as user-facing API.
