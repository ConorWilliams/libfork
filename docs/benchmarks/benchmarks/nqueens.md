---
icon: lucide/crown
---

# N-Queens

The N-Queens benchmark counts all valid placements of `n` queens on an `n x n`
board. It uses recursive backtracking and checks the result against known
solution counts.

Source:

- [shared N-Queens helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/nqueens.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/nqueens.cpp)

## What It Measures

`test` uses `n = 8`; `base` uses `n = 14`. The board is represented as one
column choice per row. At each level, the benchmark tries every column and calls
`queens_ok` over the current prefix.

## Scaling

Backtracking search can scale well when high-level row choices are distributed
across workers. The search tree is irregular because many partial boards are
pruned early while others continue deeply. Static splitting by the first row is
often not enough for large worker counts; deeper dynamic work sharing improves
balance.

## Bottlenecks And Granularity

The benchmark is branch-heavy and uses a small working set. It stresses
recursive control flow and pruning rather than memory bandwidth. The simple
validity check scans queen pairs in the prefix, so per-node cost grows with
depth.

Parallel granularity should fork high in the tree and switch to serial
backtracking below a cutoff. Creating a task for every candidate placement would
be much more expensive than the placement check itself.

## References

- [N-Queens problem overview](https://en.wikipedia.org/wiki/Eight_queens_puzzle)
- [Known N-Queens solution counts](https://oeis.org/A000170)
- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
