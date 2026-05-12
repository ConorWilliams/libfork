---
icon: lucide/list
---

# Catalogue

Each page documents one benchmark family: what it measures, which inputs are
registered, what scaling behavior to expect, and where the source lives.

For local runs, start with [reproducing results](../reproducing.md). For shared
benchmark helpers and registration macros, see [benchmark internals](../internals.md).

## Benchmarks

### Divide and conquer

#### Homogeneous

Note: on heterogenous hardware even homogenous workloads can have irregular
task costs, so these are more about regularity of the task graph than
regularity of the task costs.

- [Fibonacci](fib.md): recursive task overhead and frame allocation.
- [Unbalanced tree search T1](uts.md): regular search-tree traversal.
- [Fold](fold.md): reductions over memory-backed and lazy ranges.
- [Matrix multiply](matmul.md): recursive cubic matrix multiply.
- [Scan](scan.md): repeated inclusive prefix scan.

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

### Other

- [Random Scheduler Switch](switch-random.md): cross-pool coroutine migration during recursive Fibonacci.
- [I/O Pool Switch](switch-io-pool.md): request fan-out with explicit compute-pool and I/O-pool hops.

- [Knapsack](knapsack.md): exact branch-and-bound search.
- [Strassen](strassen.md): recursive seven-product matrix multiply.
- [N-Queens](nqueens.md): recursive backtracking search.
- [Quicksort](quicksort.md): in-place divide-and-conquer sorting.
- [Mergesort](mergesort.md): stable divide-and-conquer sorting.
- [Skynet](skynet.md): regular recursive fan-out reduction.
