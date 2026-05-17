---
icon: lucide/merge
---

# Merge Sort

The mergesort benchmark stably sorts a deterministic random array of 32-bit
unsigned integers. Mergesort is a divide-and-conquer sorting algorithm: split
the input, sort each half, then merge the two sorted halves back together. The
serial projection is top-down mergesort:

```cpp linenums="1"
mergesort(first, mid);
mergesort(mid, last);
merge(first, mid, last);
```

Small partitions are handled by [insertion
sort](https://en.wikipedia.org/wiki/Insertion_sort). A scratch buffer is
allocated once per benchmark iteration and threaded through the recursion, so
the measured work is the sort and copy traffic rather than repeated allocation.

The parallel version follows
[Cilksort](https://publications.csail.mit.edu/lcs/pubs/pdf/MIT-LCS-TR-785.pdf),
as in the original Cilk benchmark and later OpenMP translations. Cilksort is
still mergesort, but it exposes more independent fork-join work by splitting the
array into four quarters, sorting those quarters, merging the first two quarters
and the last two quarters, then merging the resulting halves:

```cpp linenums="1"
cilksort(a1, tmp1);
cilksort(a2, tmp2);
cilksort(a3, tmp3);
cilksort(a4, tmp4);
merge(a1, a2, tmp12);
merge(a3, a4, tmp34);
merge(tmp12, tmp34, out);
```

The two intermediate merges write into the scratch buffer, and the final merge
writes back to the input array. This ping-pong between input and scratch avoids
allocating temporary storage at each recursive level.

The parallel merge is the key difference from a basic serial mergesort. It
chooses a split point in the larger sorted range, uses binary search to find the
matching split point in the other range, then recursively merges the two
independent output halves. The gist notes this median-splitting idea from S. G.
Akl and N. Santoro's 1987 paper, [Optimal Parallel Merging and Sorting Without
Memory Conflicts](https://doi.org/10.1109/TC.1987.5009478).

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
\(\mathcal{O}(n)\). With a serial merge, the span would be
\(\mathcal{O}(n)\). With a parallel binary-splitting merge, the Cilksort
critical path is:

\[
T_\infty = \mathcal{O}(\log^3 n)
\]

The original Cilk notes also mention that a logarithmic factor can be removed
with a more sophisticated merge. This benchmark uses the simpler recursive
binary-splitting merge shape because it maps directly onto fork-join tasking.

## Scaling

Mergesort exposes regular divide-and-conquer parallelism in the two recursive
sorts. Unlike quicksort, the split sizes are predictable, so the task graph is
balanced.

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
