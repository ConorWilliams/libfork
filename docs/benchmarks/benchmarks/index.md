---
icon: lucide/list
---

# Benchmark catalogue

Each page documents one benchmark family: what it measures, which inputs are
registered, what scaling behavior to expect. For a quick primer on parallel
scaling read the [parallel scaling section](../index.md#parallel-scaling). For
local runs, start with the [reproducing](../reproducing.md) page.

## Benchmarks

### Divide and conquer

These benchmarks follow a recursive divide-and-conquer pattern, they are the
poster child of fork-join frameworks where these kinds of algorithms are
natural to parallelize.

#### Homogeneous

Task graphs that are regular produce approximately equal work per child task in
any give parent task. This makes scheduling easier as work follows a
predictable pattern. Work can be quickly evenly distributed by sharing some
tasks close to the root of the graph.

- [Fibonacci](fib.md): recursive computation of the Fibonacci numbers.
- [Unbalanced tree search (geometric)](uts.md): regular search-tree traversal.
- [Matrix multiply](matmul.md): recursive cubic matrix multiply.
- [Fold](fold.md): reductions over memory-backed and lazy ranges.
- [Scan](scan.md): prefix sums over memory-backed and lazy ranges.

!!! info

    On modern heterogeneous hardware even homogeneous workloads can have irregular
    task costs (as a slower core may take longer to complete the same work as a
    fast core) hence, these are classifications more about regularity of the task
    graph.

#### Heterogeneous

TODO: link T1/T3 families
TODO: use names binary/geometric

- [Unbalanced tree search T2](uts.md): irregular search-tree traversal.
- [Integrate](integrate.md): adaptive recursive quadrature.

### Bulk parallelism

#### Homogeneous

- [Heat](heat.md): Jacobi heat-diffusion stencil.

#### Heterogeneous

- [Mandelbrot](mandelbrot.md): per-pixel escape-time computation.
- [Primes](primes.md): trial-division prime counting.

### Other/special

- [Random Scheduler Switch](switch-random.md): cross-pool coroutine migration during recursive Fibonacci.
- [I/O Pool Switch](switch-io-pool.md): request fan-out with explicit compute-pool and I/O-pool hops.

- [Knapsack](knapsack.md): exact branch-and-bound search.
- [Strassen](strassen.md): recursive seven-product matrix multiply.
- [N-Queens](nqueens.md): recursive backtracking search.
- [Quicksort](quicksort.md): in-place divide-and-conquer sorting.
- [Mergesort](mergesort.md): stable divide-and-conquer sorting.
- [Skynet](skynet.md): regular recursive fan-out reduction.
