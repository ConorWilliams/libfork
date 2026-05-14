---
icon: lucide/backpack
---

# Knapsack

The knapsack benchmark solves the exact 0/1 knapsack problem with recursive
branch and bound. Items are generated deterministically, sorted by value
density, and the capacity is set to half the total item weight.

At each item, the search either takes the item, if it fits, or skips it. A
fractional-knapsack relaxation gives an upper bound; subtrees whose bound cannot
beat the current best solution are pruned.

## Complexity

In the worst case, the search explores both choices for every item:

\[
T_1 = \mathcal{O}(2^n)
\]

The branch-and-bound relaxation can make practical work much smaller, but the
amount of pruning is input dependent. The maximum recursion depth is linear:

\[
T_\infty = \mathcal{O}(n)
\]

The serial benchmark validates the answer against a dynamic-programming oracle.

## Scaling

Knapsack is an irregular search benchmark. Early branches may discover a strong
incumbent and prune later work, while other branches may remain large. This
makes task sizes unpredictable.

A parallel implementation must also coordinate updates to the current best
value. More frequent updates improve pruning but can increase contention.

## Benchmark sizes

The following problem sizes are available:

| Name | Items |
|------|-------|
| test | `16` |
| base | `28` |

## Results

TODO: results
