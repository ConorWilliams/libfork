---
icon: lucide/scan-line
---

# Scan

The scan benchmark repeatedly computes an inclusive prefix sum over a vector of
unsigned integers using `std::inclusive_scan`. The last output value is checked
against the arithmetic-series sum.

Source:

- [shared scan helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/scan.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/scan.cpp)

## What It Measures

`test` uses 1000 elements; `base` uses 8000 elements. Each benchmark iteration
performs 1000 scans to make the small configured sizes measurable.

## Scaling

Prefix scan has less trivial parallelism than a map or reduction because each
output depends on all earlier inputs. Parallel algorithms usually perform an
upsweep over block totals followed by a downsweep or offset pass. That means at
least two global phases and more synchronization than a simple reduction.

For small vectors, the serial implementation should be hard to beat. A parallel
version needs larger inputs or many batched scans to amortize phase overhead.

## Bottlenecks And Granularity

This benchmark is memory-bandwidth-sensitive and synchronization-sensitive. The
working set is small at current sizes, so cache effects and loop overhead are
important. Useful task granularity is a block of contiguous elements, not
individual prefix operations.

## References

- [Blelloch, "Prefix Sums and Their Applications"](https://www.cs.cmu.edu/~scandal/papers/CMU-CS-90-190.html)
- [C++ `std::inclusive_scan`](https://en.cppreference.com/w/cpp/algorithm/inclusive_scan)
- [Prefix sum overview](https://en.wikipedia.org/wiki/Prefix_sum)
