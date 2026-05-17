---
icon: lucide/book-open
---

# API reference

All public symbols documented here live in namespace `lf` and are reachable via:

```cpp
import libfork;
```

The meta-module re-exports the following modules:

- `libfork.core`
- `libfork.batteries`
- `libfork.schedulers`
- `libfork.algorithm`

Each of these modules is documented in its own section:

- [Core](core/index.md): Central component.
- [Batteries](batteries.md): worker stacks, deque, context policies, and context implementations.
- [Schedulers](schedulers.md): inline and busy-pool schedulers.
- [Algorithm](algorithm.md): fork-join algorithms over random-access ranges.

`libfork.utils` is a support module and is not documented as user-facing API.
