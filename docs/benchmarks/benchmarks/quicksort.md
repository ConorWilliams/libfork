---
icon: lucide/list-ordered
---

# Quicksort

The quicksort benchmark sorts a deterministic random array of 32-bit unsigned
integers. It uses an in-place partition, a middle-element pivot, tail recursion
on one side, and insertion sort for small partitions.

Source:

- [shared quicksort input](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/quicksort.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/quicksort.cpp)

## What It Measures

`test` uses 10000 elements; `base` uses 10000000 elements. Each iteration copies
the original input to a work buffer, sorts it, and validates against
`std::sort` output.

## Scaling

Parallel quicksort can fork the two partitions after each split. Scaling depends
on pivot quality and partition balance. Random input with a middle-element pivot
usually produces enough parallel work, but bad partitions reduce parallelism and
increase span.

## Bottlenecks And Granularity

Partitioning is memory-bandwidth-sensitive and branch-heavy. Sorting small
partitions is better done inline with insertion sort. A parallel implementation
should stop spawning when a partition is below a size cutoff, both for task
overhead and cache locality.

The benchmark includes input-copy time inside the measured loop, so memory
bandwidth for copying is part of the result.

## References

- [Quicksort overview](https://en.wikipedia.org/wiki/Quicksort)
- [C++ `std::sort`](https://en.cppreference.com/w/cpp/algorithm/sort)
- [OpenMP task construct](https://www.openmp.org/spec-html/5.2/openmpse73.html)
