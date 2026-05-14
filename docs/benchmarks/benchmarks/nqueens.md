---
icon: lucide/crown
---

# N-Queens

The N-Queens benchmark counts the number of ways to place `n` queens on an
`n x n` chessboard so that no two queens attack each other. The search places
one queen per row and recursively tries every column that remains valid.

```cpp linenums="1"
for (int column = 0; column < n; ++column) {
  place(row, column);
  if (board_is_valid()) {
    count += search(row + 1);
  }
}
```

## Complexity

The worst-case search tree is exponential. A loose upper bound is:

\[
T_1 = \mathcal{O}(n!)
\]

because at most one queen can occupy each column. Diagonal checks prune many
branches, but the amount of pruning depends strongly on the partial board.

The longest dependency chain places one queen per row:

\[
T_\infty = \mathcal{O}(n)
\]

## Scaling

N-Queens is an irregular recursive search benchmark. Branches near the top of
the tree can contain very different numbers of valid descendants, so static
partitioning is fragile.

Good scaling requires enough search subtrees to balance workers. Very fine
tasks improve balance but increase scheduler overhead and duplicate board-state
management.

## Benchmark sizes

The following problem sizes are available:

| Name | Board |
|------|-------|
| test | `8 x 8` |
| base | `14 x 14` |

## Results

TODO: results
