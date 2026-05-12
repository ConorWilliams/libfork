---
icon: lucide/timer
---

# Benchmarks

`libfork` is designed for strict fork-join parallelism where many small
coroutines cooperate through continuation stealing. The benchmark suite measures
the costs that matter for that model: task creation, joining, scheduling,
worker-to-worker stealing, scheduler switching, and the point where real
workloads stop being scheduler-bound and become limited by memory, cache
locality, or arithmetic throughput.

## Performance results

Graphs will be added here once the plotting pipeline is checked in.

For detailed workload notes, input sizes, and links to each benchmark family,
see the [catalogue](benchmarks/index.md). To build and run the suite locally,
see [reproducing results](reproducing.md).

### Compared libraries

The benchmark source tree separates shared inputs and reference checks from
implementation variants:

- [`benchmark/src/libfork/`](../../benchmark/src/libfork/) measures `libfork`
  coroutine tasks and scheduler implementations.
- [`benchmark/src/serial/`](../../benchmark/src/serial/) provides
  single-threaded baselines for the same workload families.
- [`benchmark/src/openmp/`](../../benchmark/src/openmp/) provides OpenMP tasking
  comparisons where the workload has an OpenMP implementation.
- [`benchmark/src/baremetal/`](../../benchmark/src/baremetal/) contains
  low-level coroutine or data-structure baselines used to isolate runtime costs.

## Performance scaling

### Parallel speedup

### Amdhal's law

[Amdahl's law](https://en.wikipedia.org/wiki/Amdahl%27s_law) gives the first-order limit on speedup:

```text
speedup(p) <= 1 / (s + (1 - s) / p)
```

where `p` is the worker count and `s` is the fraction of work that remains
serial. Fork-join runtimes also care about the work/span model. `T1` is total
work, `Tinf` is the critical path length, and available parallelism is roughly
`T1 / Tinf`. A benchmark can only scale while it has enough ready work to occupy
the workers and each task has enough useful work to amortize fork, scheduling,
and join overhead.

This is why the suite includes both tiny recursive microbenchmarks and larger
algorithmic kernels. Fibonacci and Skynet expose scheduler overhead directly.
Heat, matrix multiply, sorting, scans, reductions, and search kernels show where
memory bandwidth, cache behavior, load balance, allocation, and synchronization
replace scheduler overhead as the dominant cost.

Useful background:

- [libfork paper](https://arxiv.org/abs/2402.18480)
- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
- [OpenMP 5.2 task construct](https://www.openmp.org/spec-html/5.2/openmpse73.html)
