---
icon: lucide/list
---

# Benchmarks

Each page documents one benchmark family: what it measures, which inputs are
registered, what scaling behavior to expect, and where the source lives.

For local runs, start with [reproducing results](../reproducing.md). For shared
benchmark helpers and registration macros, see [benchmark internals](../internals.md).

## Families

- [Fibonacci](fib.md): recursive task overhead and frame allocation.
- [Fold](fold.md): reductions over memory-backed and lazy ranges.
- [Unbalanced Tree Search](uts.md): irregular search-tree traversal.
- [Random Scheduler Switch](switch-random.md): cross-pool coroutine migration
  during recursive Fibonacci.
- [I/O Pool Switch](switch-io-pool.md): request fan-out with explicit
  compute-pool and I/O-pool hops.
- [Heat](heat.md): Jacobi heat-diffusion stencil.
- [Integrate](integrate.md): adaptive recursive quadrature.
- [Knapsack](knapsack.md): exact branch-and-bound search.
- [Mandelbrot](mandelbrot.md): per-pixel escape-time computation.
- [Matrix Multiply](matmul.md): recursive cubic matrix multiply.
- [Strassen](strassen.md): recursive seven-product matrix multiply.
- [N-Queens](nqueens.md): recursive backtracking search.
- [Primes](primes.md): trial-division prime counting.
- [Quicksort](quicksort.md): in-place divide-and-conquer sorting.
- [Mergesort](mergesort.md): stable divide-and-conquer sorting.
- [Scan](scan.md): repeated inclusive prefix scan.
- [Skynet](skynet.md): regular recursive fan-out reduction.
