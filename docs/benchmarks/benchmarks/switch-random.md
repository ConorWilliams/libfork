---
icon: lucide/shuffle
---

# Random Scheduler Switch

The random scheduler-switch benchmark runs recursive Fibonacci while randomly
migrating continuations between two scheduler pools. It is a libfork-specific
stress test for cross-pool posting, continuation resumption, and type-erased
scheduler overhead.

Source:

- [benchmark implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/libfork/switch_random.cpp)
- [shared Fibonacci reference](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/fib.hpp)
- [libfork scheduling API](../../api/core/scheduling.md)

## What It Measures

The workload is still recursive Fibonacci, checked against the iterative
reference value. At each internal node, a SplitMix64-derived state gives an
approximately 10 percent chance of switching to the other pool before forking
children. The total worker count is split between the two pools.

Variants compare mono and type-erased busy pools. The benchmark records the
worker split as counters.

## Scaling

This benchmark should scale worse than the ordinary Fibonacci task benchmark
because some continuations must be posted to another pool. Good results indicate
that cross-pool scheduling remains cheap relative to coroutine creation and
join overhead. Poor results can point to posting contention, cache migration, or
type-erasure overhead.

## Bottlenecks And Granularity

Tasks are intentionally tiny. The useful compute per node is small, so random
pool switches expose scheduler mechanics rather than application throughput.
The benchmark requires at least two workers because a single-worker run cannot
split work between pools.

This is not an I/O model. It is a controlled migration probe with deterministic
randomness and strict fork-join structure.

## References

- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
- [libfork scheduler docs](../../api/schedulers.md)
- [SplitMix64 reference implementation](https://prng.di.unimi.it/splitmix64.c)
