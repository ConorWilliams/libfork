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
predictable pattern. Work can be quickly evenly distributed by sharing some
tasks close to the root of the graph.

- [Fibonacci](fib.md): recursive computation of the Fibonacci numbers.
- [Unbalanced tree search (geometric)](uts.md): regular search-tree traversal.
- [Matrix multiply](matmul.md): recursive cubic matrix multiply.
- [Fold](fold.md): reductions over memory-backed and lazy ranges.
- [Scan](scan.md): prefix sums over memory-backed and lazy ranges.

!!! info

    On modern heterogeneous hardware even homogeneous workloads can have irregular
    task costs (i.e. a slower core may take longer to complete the same work as a
    fast core) hence, these are classifications more about regularity of the task
    graph.

### Heterogeneous

These benchmarks have irregular task graphs where the work per child task can
vary. The degree of irregularity can vary from benchmark to benchmark.
Scheduling irregular workloads is substantially more difficult and often
results in a higher scheduling overhead (e.g. for things like work stealing).
Highly irregular workloads can also mandate a finer task granularity to achieve
good load balancing which puts further demands on minimizing per-task
scheduling overhead.

These benchmarks are prime examples of programs that are very hard to
parallelize efficiency with less flexible parallel programming models.

TODO: link T1/T3 families

- [Unbalanced tree search (binary)](uts.md): irregular search-tree traversal.
- [Integrate](integrate.md): adaptive recursive quadrature integral.

## Bulk parallelism

### Homogeneous

- [Heat](heat.md): Jacobi heat-diffusion stencil.

### Heterogeneous

- [Mandelbrot](mandelbrot.md): per-pixel escape-time computation.
- [Primes](primes.md): trial-division prime counting.

## Other/special

- [Random Scheduler Switch](switch-random.md): cross-pool coroutine migration during recursive Fibonacci.
- [I/O Pool Switch](switch-io-pool.md): request fan-out with explicit compute-pool and I/O-pool hops.

- [Knapsack](knapsack.md): exact branch-and-bound search.
- [Strassen](strassen.md): recursive seven-product matrix multiply.
- [N-Queens](nqueens.md): recursive backtracking search.
- [Quicksort](quicksort.md): in-place divide-and-conquer sorting.
- [Mergesort](mergesort.md): stable divide-and-conquer sorting.
- [Skynet](skynet.md): regular recursive fan-out reduction.
