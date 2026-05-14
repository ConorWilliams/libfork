---
icon: lucide/sigma
---

# Fold

The fold benchmark reduces a range with addition. The serial projection uses
`std::reduce`:

```cpp linenums="1"
auto fold(auto first, auto last) {
  return std::reduce(first, last, 0, std::plus<>{});
}
```

The inputs are either memory-backed vectors or lazy ranges. Values repeat the
pattern `0, 1, 2, 3`, so the expected result is known exactly for integer data
and within a tight tolerance for floating-point data.

## Complexity

For \(n\) elements, the work is:

\[
T_1 = \mathcal{O}(n)
\]

A tree reduction has logarithmic span when enough work is available at each
level:

\[
T_\infty = \mathcal{O}(\log n)
\]

The memory-backed variant stores all \(n\) input values. The lazy variant avoids
the input allocation but recomputes each value when it is visited.

## Scaling

Fold is a regular bulk-parallel workload. Each chunk performs the same
operation over a contiguous range, then the partial sums are combined in a
small reduction tree.

Scaling is mostly limited by memory bandwidth for memory-backed input and by
task overhead when the chunk size is too small. The lazy variant shifts the cost
toward value generation and iterator/projection overhead.

## Benchmark sizes

The following problem sizes are available:

| Name | Elements |
|------|----------|
| test | `10` |
| base | `1'024` |
| base | `1'048'576` |
| base | `1'073'741'824` |

The suite includes `int32` and `float32` data, memory-backed and lazy ranges,
and libfork variants with fixed, deduced, and unit chunk sizes.

## Results

TODO: results
