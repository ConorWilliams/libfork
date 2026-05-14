---
icon: lucide/git-fork
---

# Fibonacci

The Fibonacci benchmark computes `fib(n)` with the deliberately inefficient
binary recursion:

```cpp linenums="1"
auto fib(int n) -> int {
  if (n <= 1) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}
```

It is a tasking microbenchmark: each internal node creates two sub-problems
(line 5 above) and does almost no work/arithmetic. Hence, it primarily measures
the overhead of task creation and scheduling.

It is worth noting that, of the two task spawned, only the first should be made
available for stealing as, after the second task is spawned, the parent can't
continue until after both children have completed and there is no work for it
to do in this period.

## Complexity

The time complexity of this algorithm is equal to the number of task spawned,
let \[S(n)\] be equal to the total number of task spawned by `fib(n)`. Then for
\(n = 1\):

\[
S(0) = S(1) = 1
\]

Because they need to spawn the root task, and for \(n > 1\):

\[
S(n) = 2 + S(n - 1) + S(n - 2)
\]

Which is related to the Fibonacci sequence itself:

\[
S(n) = 2 F(n + 1) - 1
\]

The proof follows by induction of \(F(0) = f(1) = 1\) and \(F(n) = F(n - 1) +
F(n - 2)\). The time complexity is therefore exponential in \(n\). The space
complexity is linear in \(n\) as the longest path from the root to a leaf is
\(n\) (i.e. the critical path length of the DAG). Similarly the span is linear
in \(n\) as the longest path from the root to a leaf is \(n\).

## Benchmark sizes

The following problem sizes are available:

- test: `n = 8`

Source:

- [shared input and reference](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/fib.hpp)
- [serial variants](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/fib.cpp)
- [libfork variants](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/libfork/fib.cpp)
- [OpenMP variant](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/openmp/fib.cpp)
- [bare-metal variants](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/baremetal/fib.cpp)

## What It Measures

`test` uses `n = 8`; `base` uses `n = 37`. The useful arithmetic per recursive
node is tiny, so elapsed time is mostly frame creation, coroutine/task
scheduling, deque or stack traffic, and join overhead.

The benchmark includes:

- serial recursion that writes into an out-parameter;
- serial recursion that returns the value directly;
- libfork inline schedulers over vector or deque adaptors and multiple stack
  types;
- libfork busy pools using mono and type-erased schedulers;
- OpenMP `task`/`taskwait`;
- bare-metal coroutine and deque-overhead probes.

## Scaling

The work grows exponentially with `n`, while the span is linear in `n`, so the
algorithm exposes huge theoretical parallelism at the base size. In practice,
tasks are so fine grained that scaling is limited by scheduler throughput,
worker wake-up/steal costs, cache traffic on queues or stacks, and join
coordination.

Good scaling means the runtime can process many small strict fork-join tasks per
second. Poor scaling does not say much about Fibonacci itself; it usually means
the per-task overhead is larger than the useful work.

## Bottlenecks And Granularity

The benchmark is compute-light and allocation/scheduling-heavy. Memory pressure
comes from coroutine frames, recursion frames, and per-worker stack/deque
storage, not from input data. Granularity is intentionally too small for a real
parallel Fibonacci implementation. A production algorithm would stop spawning
below a cutoff and compute the remaining subtree serially.

## References

- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
- [Cilk publications and tasking references](https://cilk.mit.edu/publications/)
- [OpenMP 5.2 task construct](https://www.openmp.org/spec-html/5.2/openmpse73.html)
