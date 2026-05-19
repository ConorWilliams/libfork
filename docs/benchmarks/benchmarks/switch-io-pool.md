---
icon: lucide/arrow-left-right
---

# I/O pool scheduler-switch

The I/O pool scheduler-switch benchmark fans out many request-like coroutines. Each
request performs a small amount of compute work, switches to an I/O pool,
performs a smaller busy-loop payload, then switches back to the compute pool
before returning.

The benchmark also includes a baseline with the same work and no scheduler
switches.

## Complexity

For \(m\) requests, the useful work is linear:

\[
T_1 = \mathcal{O}(m)
\]

The fan-out has shallow span: the parent forks all requests and then reduces
their results (via a [fold](fold.md)).

\[
T_\infty = \mathcal{O}(log m)
\]

## Scaling

This is a libfork-specific scheduler benchmark. It measures the cost of posting
continuations between pools in a structured fork-join computation, rather than
modeling real blocking I/O.

Good results indicate that explicit scheduler switches remain cheap relative to
coroutine creation, joining, and the small synthetic payload. Poor results can
point to cross-pool posting contention or cache migration.

This benchmark is related to [Random Scheduler Switch](switch-random.md), but
uses deterministic request phases instead of random migration inside Fibonacci.

## Benchmark sizes

The following problem sizes are available:

| Name | Requests | Compute units | I/O units |
| ---- | -------- | ------------- | --------- |
| test | `64`     | `256`         | `32`      |
| base | `65'534` | `256`         | `32`      |

## Results

TODO: results
