---
icon: lucide/merge
---

# Merge Sort

The mergesort benchmark stably sorts a deterministic random array of 32-bit
unsigned integers. Mergesort is a divide-and-conquer sorting algorithm: split
the input, sort the pieces, then merge the sorted pieces back together. The
serial projection uses the same four-way recursion pattern as
[Cilksort](https://publications.csail.mit.edu/lcs/pubs/pdf/MIT-LCS-TR-785.pdf),
but keeps the merges serial:

```cpp linenums="1"
mergesort(q1, scratch1);
mergesort(q2, scratch2);
mergesort(q3, scratch3);
mergesort(q4, scratch4);
merge(q1, q2, scratch12);
merge(q3, q4, scratch34);
merge(scratch12, scratch34, first);
```

Small partitions are handled by [insertion
sort](https://en.wikipedia.org/wiki/Insertion_sort). A scratch buffer is
allocated once per benchmark iteration and threaded through the recursion, so
the measured work is the sort and copy traffic rather than repeated allocation.

The two intermediate merges write into the scratch buffer, and the final merge
writes back to the input array. This ping-pong between input and scratch avoids
allocating temporary storage at each recursive level.

The parallel merge is the key difference from a basic serial mergesort. It
chooses a split point in the larger sorted range, uses binary search to find
the matching split point in the other range, then recursively merges the two
independent output halves. For further details see S. G. Akl and N. Santoro's
1987 paper, [Optimal Parallel Merging and Sorting Without Memory
Conflicts](https://doi.org/10.1109/TC.1987.5009478).

## Complexity

The four-way form performs four quarter-sized recursive sorts and linear merge
work at each level:

\[
T_1(n) = 4T_1(n / 4) + \mathcal{O}(n)
\]

so the work is:

\[
T_1 = \mathcal{O}(n \log n)
\]

The benchmark uses an auxiliary buffer, so the extra space is
\(\mathcal{O}(n)\). The serial benchmark uses ordinary serial merges, so it
preserves the Cilksort recursion and buffer schedule without claiming the
parallel merge span. With a parallel binary-splitting merge, the Cilksort
critical path becomes:

\[
T_\infty = \mathcal{O}(\log^3 n)
\]

## Scaling

Mergesort exposes regular divide-and-conquer parallelism in the recursive sorts
and in the two intermediate merges. Unlike quicksort, the split sizes are
predictable, so the task graph is balanced.

The merge step is memory-bandwidth heavy and can become the scaling limit. The
base-case cutoff also matters: small tasks increase scheduling overhead, while
large tasks reduce available parallelism near the leaves. The OpenMP reference
uses a much larger cutoff for task creation; this benchmark keeps the serial
base case small and lets each implementation choose where to stop spawning
parallel work.

This benchmark is the balanced counterpart to [quicksort](quicksort.md). It
does more copying, but it avoids pivot-dependent imbalance.

## Benchmark sizes

The following problem sizes are available:

| Name | Elements | Base case |
|------|----------|-----------|
| test | `10'000` | `32` |
| base | `10'000'000` | `32` |

## Results

TODO: results
