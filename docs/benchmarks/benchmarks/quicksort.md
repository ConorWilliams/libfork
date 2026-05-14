---
icon: lucide/list-ordered
---

# Quicksort

The quicksort benchmark sorts a deterministic random array of 32-bit unsigned
integers. It partitions in place around a middle-element pivot and then sorts
the two partitions recursively:

```cpp linenums="1"
auto pivot = partition(first, last);
quicksort(first, pivot);
quicksort(pivot + 1, last);
```

Small partitions are handled by insertion sort. The benchmark validates the
result against `std::sort`.

```mermaid
flowchart TD
  A["partition n elements"] --> L["sort left half"]
  A --> R["sort right half"]
  L --> LL["partition"]
  L --> LR["partition"]
  R --> RL["partition"]
  R --> RR["partition"]
```

## Complexity

For the benchmark's deterministic random input, the useful model is average
case with balanced partitions. Quicksort performs linear partitioning work at
each level:

\[
T_1 = \mathcal{O}(n \log n)
\]

The worst case is quadratic if the pivot repeatedly creates one empty or tiny
partition, but that is not the intended operating point of this benchmark:

\[
T_1 = \mathcal{O}(n^2)
\]

The partition itself is not parallel. Even with balanced partitions, the span is
therefore:

\[
T_\infty(n) = T_\infty(n / 2) + \mathcal{O}(n) = \mathcal{O}(n)
\]

because each level must complete a serial partition before the two recursive
sorts can proceed.

## Scaling

Quicksort exposes divide-and-conquer parallelism after each partition. The two
recursive calls are independent, but their sizes depend on the pivot, so the
task graph is less regular than mergesort.

Partitioning is branch-heavy and memory-bandwidth sensitive. The benchmark also
copies the deterministic source array into a work buffer inside each measured
iteration, so copy bandwidth is part of the result.

Quicksort is a useful contrast with [mergesort](mergesort.md): both are
divide-and-conquer sorts, but quicksort has cheaper in-place partitioning and a
less predictable task graph.

## Benchmark sizes

The following problem sizes are available:

| Name | Elements | Base case |
|------|----------|-----------|
| test | `10'000` | `32` |
| base | `10'000'000` | `32` |

## Results

TODO: results
