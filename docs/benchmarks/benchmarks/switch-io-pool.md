---
icon: lucide/arrow-left-right
---

# I/O Pool Switch

The I/O pool switch benchmark fans out many request-like coroutines. Each
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

The fan-out has shallow span: the parent forks all requests, waits for them,
and then reduces their results:

\[
T_\infty = \mathcal{O}(m)
\]

for the serial fork and final sum in the benchmark driver. The per-request
span is constant.

## Scaling

This is a libfork-specific scheduler benchmark. It measures the cost of posting
continuations between pools in a structured fork-join computation, rather than
modeling real blocking I/O.

Good results indicate that explicit scheduler switches remain cheap relative to
coroutine creation, joining, and the small synthetic payload. Poor results can
point to cross-pool posting contention or cache migration.

## Benchmark sizes

The following problem sizes are available:

| Name | Requests | Compute units | I/O units |
|------|----------|---------------|-----------|
| test | `64` | `256` | `32` |
| base | `65'534` | `256` | `32` |

The I/O pool uses `max(2, hardware_concurrency / 8)` workers. The compute pool
uses the benchmark worker-count argument.

## Results

TODO: results
