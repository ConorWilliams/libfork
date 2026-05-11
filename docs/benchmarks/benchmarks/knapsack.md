---
icon: lucide/package-check
---

# Knapsack

The knapsack benchmark solves a deterministic 0/1 knapsack instance exactly
with branch and bound. Items are sorted by value density, and a fractional
relaxation bound prunes subtrees that cannot beat the current best value.

Source:

- [shared knapsack setup](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/knapsack.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/knapsack.cpp)

## What It Measures

`test` uses 16 items; `base` uses 28 items. The item weights and values are
generated from a fixed random seed. The benchmark verifies the branch-and-bound
answer against a dynamic-programming optimum.

## Scaling

Parallel branch and bound can expose large amounts of independent search, but
speedup depends on pruning order and on how quickly workers see improved
incumbent values. If the best value is shared globally, synchronization on that
incumbent can become visible. If it is not shared aggressively, workers may
waste time exploring subtrees that a newer bound would prune.

## Bottlenecks And Granularity

The serial benchmark is branch-heavy and cache-light. Most time goes into
recursive search and repeated bound calculations over the remaining items.
Granularity is irregular: high-level branches are valuable tasks, while leaf
subtrees are too small to schedule individually. A parallel implementation
should stop forking below a depth or estimated-subtree cutoff.

## References

- [Knapsack problem overview](https://en.wikipedia.org/wiki/Knapsack_problem)
- [Branch and bound overview](https://en.wikipedia.org/wiki/Branch_and_bound)
- [OR-Tools knapsack solver reference implementation](https://developers.google.com/optimization/pack/knapsack)
