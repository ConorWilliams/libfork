---
icon: lucide/arrow-left-right
---

# I/O Pool Switch

The I/O pool switch benchmark models request fan-out where each request does
CPU work, hops to an I/O pool, does a smaller amount of work, then hops back to
the compute pool. A baseline variant performs the same total work without pool
switches.

Source:

- [benchmark implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/libfork/switch_io_pool.cpp)
- [libfork scheduling API](../../api/core/scheduling.md)
- [libfork scheduler docs](../../api/schedulers.md)

## What It Measures

Each request performs deterministic busy-loop work. The `request_io` variants
use a custom awaitable to post the continuation to another scheduler, while
`request_baseline` stays on the compute pool. A parent task forks many requests,
joins them, and sums their return values.

The benchmark compares mono and type-erased busy pools. It records request
count, compute workers, and I/O workers. The I/O pool worker count is
`max(2, hardware_concurrency / 8)`.

## Scaling

The baseline should scale like a regular fan-out/fan-in computation until
request tasks become too small or the join reduction dominates. The I/O variant
adds two cross-pool posts per request, so speedup depends on whether useful
work per request amortizes those hops.

If compute workers scale but I/O workers are saturated, the I/O pool can become
the bottleneck. If the I/O pool is underused, the extra switches mostly measure
posting and cache-migration overhead.

## Bottlenecks And Granularity

The benchmark is compute-bound by construction, but it is intended to expose
scheduler overhead rather than memory bandwidth. Granularity is controlled by
request count and fixed busy-loop units. Small request counts underfill the
workers; very large counts amplify queue traffic and result-vector writes.

Because the I/O work is simulated, this benchmark should not be interpreted as
network or disk throughput. It isolates continuation migration costs.

## References

- [libfork scheduler docs](../../api/schedulers.md)
- [OpenMP task scheduling background](https://www.openmp.org/spec-html/5.2/openmpch12.html)
- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
