---
icon: lucide/backpack
---

# Knapsack

The knapsack benchmark solves the exact
[0/1 knapsack problem](https://en.wikipedia.org/wiki/Knapsack_problem). Given
items with weights and values, choose a subset whose total weight is at most
the knapsack capacity and whose total value is as large as possible. In the 0/1
version, each item can be taken once or skipped.

This benchmark is adapted from nowa's knapsack benchmark. For each capacity, the
item list is built by cycling through nowa's fixed 32-item data set until the
total item weight just exceeds the capacity. The items are sorted by decreasing
value/weight density before the search.

At each item, the search either skips the item or takes it. If taking the item
makes the capacity negative, that branch returns an infeasible sentinel. Nowa's
branch-and-bound test uses the density of the current item as the upper bound:

\[
\text{ub} = v + c \frac{\text{item.value}}{\text{item.weight}}
\]

Subtrees whose bound is below the shared best-so-far value are pruned.

```cpp
int search(int i, int capacity, int value) {
  if (capacity < 0) {
    return INT_MIN;
  }

  if (i == items.size() || capacity == 0) {
    return value;
  }

  if (upper_bound(i, capacity, value) < best_so_far) {
    return INT_MIN;
  }

  int without = search(i + 1, capacity, value);
  int with = search(i + 1, capacity - items[i].weight,
                    value + items[i].value);
  int best = max(with, without);
  best_so_far = max(best_so_far, best);
  return best;
}
```

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

The benchmark validates the answer against a dynamic-programming oracle.

## Scaling

Knapsack is an irregular search benchmark. Early branches may discover a strong
incumbent and prune later work, while other branches may remain large. This
makes task sizes unpredictable.

A parallel implementation must also coordinate updates to the current best
value. More frequent updates improve pruning but can increase contention.

Knapsack is similar to [N-Queens](nqueens.md) and [UTS](uts.md): it is a search
problem whose task graph is discovered as the computation runs.

## Benchmark sizes

The following problem sizes are available:

| Name | Items | Capacity |
|------|-------|----------|
| test | `20` | `500` |
| base | `32` | `900` |
| large | `39` | `1100` |

## Results

TODO: results
