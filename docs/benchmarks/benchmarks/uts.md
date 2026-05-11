---
icon: lucide/tree-pine
---

# Unbalanced Tree Search

Unbalanced Tree Search, or UTS, traverses a deterministic random tree and
returns the maximum depth, node count, and leaf count. It is designed to stress
dynamic load balancing because the amount of work below each node is not known
until traversal reaches that node.

Source:

- [shared UTS helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/uts.hpp)
- [shared UTS setup](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/uts.cpp)
- [bundled C UTS code](https://github.com/conorwilliams/libfork/tree/main/benchmark/external/uts)
- [serial variants](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/uts.cpp)
- [libfork variants](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/libfork/uts.cpp)
- [OpenMP variant](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/openmp/uts.cpp)

## What It Measures

The suite registers small `T1_mini` and `T3_mini` smoke inputs, base `T1` and
`T3` inputs, and large `T1L` and `T3L` inputs. T1 is a geometric tree with more
regular branching than T3; T3 is binomial and more irregular. The traversal is
checked against known result triples for each tree.

The serial implementation has an allocation-heavy version that stores child
results in a vector and a `serial/no_alloc` version that traverses one child at
a time. The libfork and OpenMP implementations fork child subtrees and join
their result triples.

## Scaling

UTS should scale well when stealing balances the irregular frontier and the tree
is large enough to keep all workers busy. Scaling is normally worse on tiny
trees because random-number setup, task creation, and joins dominate. On very
large trees, memory allocation, cache locality, and steal traffic can become
visible.

The expected work is proportional to the generated node count. Span is driven by
the deepest path plus scheduling delays, so unlucky imbalance near the root can
limit speedup.

## Bottlenecks And Granularity

Each node performs random child generation plus a small amount of reduction
work. High fan-out creates many short tasks; low fan-out creates long serial
paths. The vector-based implementations allocate per internal node, which makes
allocator behavior and cache locality part of the measurement. The no-allocation
serial baseline isolates traversal work from child-result storage cost.

## References

- [UTS paper DOI](https://doi.org/10.1007/978-3-540-72521-3_18)
- [UTS publication record](https://scholars.uky.edu/en/publications/uts-an-unbalanced-tree-search-benchmark/)
- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
- [OpenMP 5.2 task construct](https://www.openmp.org/spec-html/5.2/openmpse73.html)
