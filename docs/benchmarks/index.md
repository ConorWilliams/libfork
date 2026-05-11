---
icon: lucide/activity
---

# Benchmarks

The benchmark suite measures fork-join task overhead, scheduler behavior, and
classic recursive or data-parallel kernels. It is built on Google Benchmark and
is organized around benchmark families. Each family page explains the workload,
expected scaling, bottlenecks, and available implementations.

## Running

Build benchmarks in release mode:

```sh
cmake --preset ci-release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake
cmake --build --preset ci-release
```

On Linux, use `cmake/gcc-brew-toolchain.cmake`.

Benchmark names use this shape:

```text
<mode>/<category>/<name>[/template-or-argument-tags]
```

`mode` is normally `test`, `base`, or `large`. Test inputs are for correctness
and smoke runs. Base inputs are the default comparison sizes. Large inputs are
intended for machines where the working set and runtime are acceptable.

## Implementations

The source tree separates shared benchmark data from implementation variants:

- [`benchmark/lib/`](https://github.com/conorwilliams/libfork/tree/main/benchmark/lib)
  contains shared kernels, input sizes, and correctness helpers.
- [`benchmark/src/libfork/`](https://github.com/conorwilliams/libfork/tree/main/benchmark/src/libfork)
  contains libfork coroutine and scheduler benchmarks.
- [`benchmark/src/serial/`](https://github.com/conorwilliams/libfork/tree/main/benchmark/src/serial)
  contains single-threaded baselines.
- [`benchmark/src/openmp/`](https://github.com/conorwilliams/libfork/tree/main/benchmark/src/openmp)
  contains OpenMP tasking comparisons where present.
- [`benchmark/src/baremetal/`](https://github.com/conorwilliams/libfork/tree/main/benchmark/src/baremetal)
  contains low-level coroutine or data-structure baselines.

## Families

- [Fibonacci](benchmarks/fib.md): recursive task overhead and frame allocation.
- [Fold](benchmarks/fold.md): reductions over memory-backed and lazy ranges.
- [Unbalanced Tree Search](benchmarks/uts.md): irregular search-tree traversal.
- [Random Scheduler Switch](benchmarks/switch-random.md): cross-pool coroutine
  migration during recursive Fibonacci.
- [I/O Pool Switch](benchmarks/switch-io-pool.md): request fan-out with explicit
  compute-pool and I/O-pool hops.
- [Heat](benchmarks/heat.md): Jacobi heat-diffusion stencil.
- [Integrate](benchmarks/integrate.md): adaptive recursive quadrature.
- [Knapsack](benchmarks/knapsack.md): exact branch-and-bound search.
- [Mandelbrot](benchmarks/mandelbrot.md): per-pixel escape-time computation.
- [Matrix Multiply](benchmarks/matmul.md): recursive cubic matrix multiply.
- [Strassen](benchmarks/strassen.md): recursive seven-product matrix multiply.
- [N-Queens](benchmarks/nqueens.md): recursive backtracking search.
- [Primes](benchmarks/primes.md): trial-division prime counting.
- [Quicksort](benchmarks/quicksort.md): in-place divide-and-conquer sorting.
- [Scan](benchmarks/scan.md): repeated inclusive prefix scan.
- [Skynet](benchmarks/skynet.md): regular recursive fan-out reduction.

## Interpreting Results

For scheduler benchmarks, near-linear speedup is possible only while there is
enough ready work to keep all workers busy and each task has enough work to
amortize fork, scheduling, and join costs. Recursive microbenchmarks such as
Fibonacci intentionally create tiny tasks, so they are most useful for comparing
runtime overhead rather than end-user algorithms.

For data-parallel kernels, memory bandwidth, cache locality, allocation rate,
and reduction or synchronization costs usually dominate before raw worker count
does. Compare variants at the same input size, compiler, CPU frequency policy,
and thread count.

Useful background:

- [libfork paper](https://arxiv.org/abs/2402.18480)
- [Google Benchmark user guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [OpenMP 5.2 task construct](https://www.openmp.org/spec-html/5.2/openmpse73.html)
- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
