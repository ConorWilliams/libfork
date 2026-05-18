---
icon: lucide/list
---

# Benchmark catalogue

## Divide and conquer

These benchmarks follow a recursive divide-and-conquer pattern, they are the
poster child of fork-join frameworks, where these kinds of algorithms are
natural to parallelize. The key feature is tasks can recursively spawn more
tasks in a parent-child relationship.

### Homogeneous

Task graphs that are regular produce approximately equal work per child task in
any give parent task. This makes scheduling easier as work follows a
predictable pattern. Work can be evenly-distributed quickly, by sharing some
tasks close to the root of the graph.

- [Fibonacci](fib.md): recursive computation of the Fibonacci numbers.
- [Skynet](skynet.md): regular recursive fan-out reduction.
- [Matrix multiply](matmul.md): recursive cubic matrix multiply.
- [Rectangular matrix multiply](rectmul.md): recursive blocked rectangular matrix multiply.
- [Matrix transpose](transpose.md): recursive matrix transpose.
- [Strassen](strassen.md): recursive seven-product matrix multiply.
- [Winograd](winograd.md): optimized Strassen-family matrix multiply.
- [Cholesky](cholesky.md): sparse quadtree Cholesky factorization.
- [LU](lu.md): recursive blocked LU decomposition.
- [FFT](fft.md): recursive mixed-radix fast Fourier transform.
- [Fold](fold.md): reductions over memory-backed and lazy ranges.
- [Scan](scan.md): prefix sums over memory-backed and lazy ranges.
- [Mergesort](mergesort.md): stable divide-and-conquer sorting with parallel merge.

!!! info

    On modern heterogeneous hardware even homogeneous workloads can have irregular
    task costs (i.e. a slower core may take longer to complete the same work as a
    fast core) hence, these classifications are more about regularity of the task
    graph.

### Heterogeneous

These benchmarks have irregular task graphs where the work per child task can
vary. The degree of irregularity can vary from benchmark to benchmark.
Scheduling irregular workloads is substantially more difficult and often
results in a higher scheduling overhead (e.g. for things like work stealing).
Highly irregular workloads can also mandate a finer task granularity to achieve
good load balancing, which puts further demands on minimizing per-task
scheduling overhead.

These benchmarks are prime examples of programs that are very hard to
parallelize efficiency with less flexible parallel programming models.

- [Unbalanced tree search](uts.md): irregular search-tree traversal.
- [Simpson integrate](integrate.md): adaptive recursive Simpson integral.
- [Quadrature integrate](quadrature-integrate.md): adaptive trapezoidal quadrature integral.
- [Knapsack](knapsack.md): exact branch-and-bound search.
- [N-Queens](nqueens.md): recursive backtracking search.
- [Quicksort](quicksort.md): divide-and-conquer quick sort (serial partition).

## Bulk parallelism

These programs are categorized by a shallow very wide task graphs.

### Homogeneous

Homogeneous bulk-parallel benchmarks are the simplest to schedule, this is the
speciality of things like OpenMP parallel for loops. These kinds of algorithms
are often easy to map to SIMD instructions and GPU kernels.

- [Heat](heat.md): Jacobi heat-diffusion stencil.

### Heterogeneous

When the time per task can vary, scheduling becomes more difficult. Sometimes a
chunk size can be chosen that mitigates this issue, potentially at the cost of
reduced parallelism.

- [Mandelbrot](mandelbrot.md): per-pixel escape-time computation.
- [Primes](primes.md): trial-division prime counting.

## Other/special

These benchmarks are for testing specific features of `libfork` and it's scheduler(s).

- [Random Scheduler Switch](switch-random.md): cross-pool coroutine migration during recursive Fibonacci.
- [I/O Pool Switch](switch-io-pool.md): request fan-out with explicit compute-pool and I/O-pool hops.
