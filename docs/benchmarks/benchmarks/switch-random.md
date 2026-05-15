---
icon: lucide/shuffle
---

# Random scheduler-switch

The random scheduler-switch benchmark runs recursive [Fibonacci](fib.md)
while occasionally migrating the current continuation between two scheduler
pools. At each internal node there is an approximately 10 percent chance of
switching pools before spawning the two children.

TODO: link to explicit scheduling documentation

!!! warning

    Because this benchmark makes use of [explicit
    scheduling](../../api/core/scheduling.md) it is not covered by `libfork`'s
    theortical guarantees (i.e. linear time/memory scaling).

## Complexity

The task graph has the same exponential size as recursive [Fibonacci](fib.md):

\[
S(n) = 2F(n + 1) - 1
\]

The span is linear in \(n\), but some nodes add an explicit cross-pool post
before continuing.

## Scaling

This is a libfork-specific stress test for scheduler mobility. It should scale
worse than ordinary Fibonacci because a fraction of continuations move between
worker pools.

The benchmark isolates cross-pool posting, continuation resumption, and
type-erased scheduler overhead. It requires at least two workers so the worker
set can be split between the two pools.

## Benchmark sizes

The following problem sizes are available:

| Name | `fib(n)` | Switch probability |
| ---- | -------- | ------------------ |
| test | `8`      | about `10%`        |
| base | `37`     | about `10%`        |

The total worker count is split between pool A and pool B.

## Results

TODO: results
