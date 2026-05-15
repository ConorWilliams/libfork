---
icon: lucide/network
---

# Skynet

The [skynet benchmark](https://github.com/atemerev/skynet) builds a perfectly
regular recursive fan-out tree. Each internal node has branching factor 10, and
each leaf returns its global leaf index. Internal nodes sum their children.

```cpp linenums="1"
for (int i = 0; i < 10; ++i) {
  sum += skynet(child_start + i * child_width, depth - 1);
}
```

The expected result is the sum of integers from `0` to `leaves - 1`.

## Complexity

For branching factor \(b = 10\) and depth \(d\), the number of leaves is
\(b^d\). The total number of nodes is geometric:

\[
T_1 = \mathcal{O}(b^d)
\]

The longest path from the root to a leaf is the depth:

\[
T_\infty = \mathcal{O}(d)
\]

## Scaling

Skynet is a regular tasking microbenchmark. The shape is predictable and
balanced, so scheduling overhead is easier to isolate than in irregular search
benchmarks.

Each task does little arithmetic besides spawning children and summing results.
As a result, scaling mainly reflects task creation, join overhead, and worker
coordination rather than application compute.

Skynet is similar to [Fibonacci](fib.md): both create many tiny tasks and
reduce child results. The difference is that Skynet's fan-out tree is regular
and balanced, while Fibonacci is a binary tree whose subtrees shrink at
different rates.

## Benchmark sizes

The following problem sizes are available:

| Name | Depth | Branching | Leaves |
|------|-------|-----------|--------|
| test | `4` | `10` | `10 ** 4` |
| base | `8` | `10` | `10 ** 8` |

## Results

TODO: results
