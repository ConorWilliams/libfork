---
icon: lucide/network
---

# Skynet

The Skynet benchmark traverses a regular recursive tree with branching factor
10. Leaves are numbered consecutively, and the recursion returns the sum of all
leaf numbers. The expected value is the arithmetic-series sum over the leaves.

Source:

- [shared Skynet helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/skynet.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/skynet.cpp)

## What It Measures

`test` uses depth 4, or 10000 leaves. `base` uses depth 6, or 1000000 leaves.
Unlike UTS, the tree is perfectly regular, so this is a fan-out/fan-in overhead
benchmark with predictable load.

## Scaling

A parallel version should expose abundant balanced work at the top levels. The
span is proportional to depth, while work is proportional to `10^depth`. Because
each node does very little work, scaling depends on grouping subtrees into tasks
large enough to amortize scheduling.

## Bottlenecks And Granularity

The workload has almost no memory pressure and little arithmetic per node. It is
therefore a scheduler and recursion-overhead benchmark. Forking every child at
every depth would create many tiny tasks; a practical version should stop
forking below a depth cutoff and sum the remaining subtree serially.

## References

- [Skynet benchmark in Crystal examples](https://github.com/kostya/benchmarks#skynet)
- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
- [Strict fork-join background in libfork paper](https://arxiv.org/abs/2402.18480)
