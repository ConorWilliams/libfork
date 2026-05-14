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

## Complexity

With balanced partitions, quicksort performs linear partitioning work at each
level:

\[
T_1 = \mathcal{O}(n \log n)
\]

The worst case is quadratic if the pivot repeatedly creates one empty or tiny
partition:

\[
T_1 = \mathcal{O}(n^2)
\]

The span depends on partition balance. With balanced partitions it follows the
height of the recursion tree; with bad partitions it becomes linear.

## Scaling

Quicksort exposes divide-and-conquer parallelism after each partition. The two
recursive calls are independent, but their sizes depend on the pivot, so the
task graph is less regular than mergesort.

Partitioning is branch-heavy and memory-bandwidth sensitive. The benchmark also
copies the deterministic source array into a work buffer inside each measured
iteration, so copy bandwidth is part of the result.

## Benchmark sizes

The following problem sizes are available:

| Name | Elements | Base case |
|------|----------|-----------|
| test | `10'000` | `32` |
| base | `10'000'000` | `32` |

## Results

TODO: results
