---
icon: lucide/sigma
---

# Fold

The fold benchmark reduces a range with addition and verifies the result against
a closed-form sum. It exercises libfork's `fold` algorithm across input
representation, chunk size, projection mode, type, and scheduler variants.

Source:

- [shared fold helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/fold.hpp)
- [serial `std::reduce` variants](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/fold.cpp)
- [libfork variants](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/libfork/fold.cpp)
- [fold API docs](../api/algorithm.md#fold)

## What It Measures

Inputs are either memory-backed vectors or lazy iota/transform ranges. Values
are `int32` or `float32`, accumulated into a wider integer or double type where
appropriate. Libfork variants compare:

- fixed chunks of 4096 elements;
- a single-element explicit chunk;
- the algorithm's deduced chunking path;
- synchronous and asynchronous projections;
- mono and type-erased busy-pool schedulers.

## Scaling

For memory-backed ranges, large reductions should scale until memory bandwidth,
cache hierarchy, or reduction-tree overhead dominates. Lazy ranges remove input
loads and emphasize iterator/projection arithmetic plus scheduling overhead.

Single-element chunks are expected to scale poorly except as an overhead stress
test. Fixed chunks provide enough arithmetic per task to amortize scheduling at
large sizes. Async projections add coroutine overhead and are useful for
checking whether the algorithm handles async work correctly, not for raw
throughput.

## Bottlenecks And Granularity

Memory-backed `1024^3` inputs can require several GiB of storage and should be
treated as machine-size-sensitive. Lazy inputs avoid that allocation but still
perform one projected addition per element. Floating-point reductions may differ
in order from serial code, so the benchmark checks tolerance rather than exact
bit identity.

Granularity should be large enough that each task streams many cache lines. Too
small a chunk turns the benchmark into a scheduler test; too large a chunk
leaves workers idle near the end of the reduction tree.

## References

- [C++ `std::reduce`](https://en.cppreference.com/w/cpp/algorithm/reduce)
- [libfork `fold` API](../api/algorithm.md#fold)
- [Google Benchmark counters and complexity](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
