---
icon: lucide/sigma
---

# Fold

The fold benchmark reduces a range with addition. The serial algorithm is a
direct left-to-right loop:

```cpp linenums="1"
auto fold(auto first, auto last) {

  auto sum = 0;

  for (; first != last; ++first) {
    sum += *first;
  }

  return sum;
}
```

The inputs are either memory-backed vectors or lazy ranges.

## Complexity

For \(n\) elements, the work is:

\[
T_1 = \mathcal{O}(n)
\]

A tree reduction (which is what is used for most parallel implementations) has
logarithmic span when enough work is available at each level:

\[
T_\infty = \mathcal{O}(\log n)
\]

The memory-backed variant stores all \(n\) input values. The lazy variant avoids
the input allocation but recomputes each value when it is visited.

## Scaling

Fold is a homogeneous divide and conquer workload. Scaling is mostly limited by
memory bandwidth for memory-backed input and by task overhead when the chunk
size is too small. The lazy variant shifts the cost toward value generation,
tasking and, iterator/projection overhead.

Fold is the reduction counterpart to [scan](scan.md): fold produces one value,
while scan produces every prefix value.

## Benchmark sizes

The following problem sizes are available:

| Name    | Elements     |
| ------- | ------------ |
| test    | `10`         |
| base_kb | `1'024`      |
| base_mb | `1'024 ** 2` |
| base_gb | `1'024 ** 3` |

## Results

TODO: results
