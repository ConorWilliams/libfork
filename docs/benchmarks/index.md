---
icon: lucide/timer
---

# Benchmarks

`libfork` is engineered for performance and has a comprehensive [benchmark
suite](./benchmarks/index.md) (which you can [reproduce
locally](reproducing.md)). For a detailed review of `libfork` on 1-112 cores see
[our paper](https://ieeexplore.ieee.org/document/10891812)[^1], the headline
results are __linear time and memory scaling__, for
[v3.x](https://github.com/ConorWilliams/libfork/releases/tag/v3.8.0) this
translated to:

[^1]: A slightly older version is avaliable on [arXiv](https://arxiv.org/abs/2402.18480)

- Up to 7.5× faster and 19× less memory consumption than OneTBB.
- Up to 24× faster and 24× less memory consumption than OpenMP (libomp).
- Up to 100× faster and >100× less memory consumption than taskflow.

## Latest performance

`libfork` has [expanded](../../ChangeLog.md) since the paper, with new features
and optimizations. In addition the landscape has evolved. This section is a
quick summary of the state-of-the-art.

### Scheduler overhead

For a quick comparison with other libraries, the average time to spawn/run a
task during the recursive [Fibonacci benchmark](./benchmarks/fib.md) gives a
good approximation to the tasking overhead and peak throughput[^2]:

TODO: graph

[^2]: All measured on a MacBook xxx with Clang XXX

### Memory consumption

TODO:

### Details of compared implementations

The implementations above correspond to:

- `libfork`

## Parallel scaling

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
