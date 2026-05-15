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

It is worth noting that, of the two tasks spawned, only the first should be made
available for stealing as, after the second task is spawned, the parent can't
continue until after both children have completed and there is no work for it
to do in this period.

## Complexity

The time complexity of this algorithm is equal to the number of tasks run. Let
\(S(n)\) be equal to the total number of tasks in the recursive computation of
`fib(n)`, including the root task. Then for \(n \le 1\):

\[
S(0) = S(1) = 1
\]

Because each non-base call runs one parent task plus the two recursive subtrees,
for \(n > 1\):

\[
S(n) = 1 + S(n - 1) + S(n - 2)
\]

Which is related to the Fibonacci sequence itself:

\[
S(n) = 2 F(n + 1) - 1
\]

The proof follows by induction of \(F(0) = 0\), \(F(1) = 1\) and \(F(n) = F(n - 1) +
F(n - 2)\) and is left to the reader. Hence the running time is:

\[
\texttt{fib}(n) = \mathcal{O} \left( 2 \cdot \texttt{fib}(n + 1) - 1 \right)
\]

The time complexity is therefore exponential in \(n\). The space complexity and
span is linear in \(n\) as the longest path from the root to a leaf is \(n\)
(i.e. the critical path length of the DAG).

## Scaling

The work grows exponentially with `n`, while the span is linear in `n`, so the
algorithm exposes huge theoretical parallelism. In practice, tasks are so fine
grained that scaling may limited by scheduler throughput, worker wake-up/steal
costs, cache traffic on queues or stacks, and join coordination.

## Benchmark sizes

The following problem sizes are available:

| Name | `fib(n)` |
| ---- | -------- |
| test | `8`      |
| base | `37`     |

## Results

TODO: results
