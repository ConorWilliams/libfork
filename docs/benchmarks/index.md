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

[^1]: A pre-print is available on [arXiv](https://arxiv.org/abs/2402.18480)

- Up to 7.5× faster and 19× less memory consumption than OneTBB.
- Up to 24× faster and 24× less memory consumption than OpenMP (libomp).
- Up to 100× faster and >100× less memory consumption than taskflow.

## Latest performance

`libfork` has [evolved](../../ChangeLog.md) since the paper, with new features
and optimizations. In addition the landscape has evolved. This section is a
brief summary of the state-of-the-art, for detailed results see [the suite](./benchmarks/index.md).

### Scheduler overhead

For a quick comparison with other libraries, the average time to spawn/run a
task during the recursive [Fibonacci benchmark](./benchmarks/fib.md) gives a
good approximation to the tasking overhead and peak throughput[^2]:

TODO: graph

[^2]: All measured on a MacBook xxx with Clang XXX

### Memory consumption

`libfork` is competitive with other libraries in terms of memory consumption,
and the only one to have mathematically optimal (linear) memory scaling (with
number of workers). Below is the peak (physical) memory allocation during the
[T3L unbalanced tree search](./benchmarks/uts.md) benchmark[^2]:

TODO:

### Frameworks

The implementations above correspond to:

- `libfork`: TODO:

## Parallel scaling

A work stealing scheduler with \(P\) workers can execute a fork-join program in expected time[^3]:

[^3]: See [Blumofe](https://doi.org/10.1145/324133.324234)

\[
T_p \le \frac{T_1}{P} + \mathcal{O} \left( T_\infty \right)
\]

where \(T_1\) is the time taken to execute the program with a single worker and
\(T_\infty\) is the ideal runtime on a machine with an infinite number of
workers. This is optimal within a constant factor[^3]. Here \(\frac{T_1}{P}\)
corresponds to the parrlelizable work, and \(T_\infty\) corresponds to the
critical path length of the DAG i.e. the non-parallelizable work. For more
details see [Amdahl's law](https://en.wikipedia.org/wiki/Amdahl%27s_law).

The parallel speedup of a program is defined as:

\[
\text{Speedup} = \frac{T_s}{T_p}
\]

Where \(T_s\) is the time taken to execute the serial projection of the program
(i.e. has no tasking overhead and hence \(T_s \le T_1\)). Following the first
equation, for a program with sufficient parallelism (i.e. \(T_1 \gg
T_\infty\)), the speedup is expected to be linear in \(P\). Additionally, the
parallel efficiency is defined as:

\[
\text{Efficiency} = \frac{\text{Speedup}}{P}
\]

A linear scaling framework should have a constant efficiency, ideally close to
1 for a program with sufficient parallelism. At \(P = 1\), the efficiency is
equal to \(\frac{T_s}{T_1}\), which directly measures the overhead of a
framework in the absence of communication interference, e.g. stealing,
cache-thrashing, lock-contention, etc.
