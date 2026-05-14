---
icon: lucide/merge
---

# Merge Sort

The mergesort benchmark stably sorts a deterministic random array of 32-bit
unsigned integers. It uses top-down divide and conquer:

```cpp linenums="1"
sort(first, mid);
sort(mid, last);
merge(first, mid, last);
```

Small partitions are handled by insertion sort. A scratch buffer is allocated
once per benchmark iteration and threaded through the recursion.

## Complexity

Mergesort splits the input in half and performs linear merge work at each level:

\[
T_1(n) = 2T_1(n / 2) + \mathcal{O}(n)
\]

so the work is:

\[
T_1 = \mathcal{O}(n \log n)
\]

The benchmark uses an auxiliary buffer, so the extra space is
\(\mathcal{O}(n)\). With parallel recursive sorts and a serial merge, the span
is \(\mathcal{O}(n)\); a parallel merge can reduce this further.

## Scaling

Mergesort exposes regular divide-and-conquer parallelism in the two recursive
sorts. Unlike quicksort, the split sizes are predictable, so the task graph is
balanced.

The merge step is memory-bandwidth heavy and can become the scaling limit. The
base-case cutoff also matters: small tasks increase scheduling overhead, while
large tasks reduce available parallelism near the leaves.

## Benchmark sizes

The following problem sizes are available:

| Name | Elements | Base case |
|------|----------|-----------|
| test | `10'000` | `32` |
| base | `10'000'000` | `32` |

## Results

TODO: results
