# Benchmarks

Benchmarking is hard! The result depend fairly strongly on the compiler + machine. Especially as compiler optimisation of coroutines in C++ is relatively unpredictable with HALO. 

## Fibonacci (fib)

From [Weave](https://github.com/mratsim/weave), this benchmark evenly spawns many-tasks that do almost no computation hence, this benchmark predominantly test creation/scheduling/deletion overhead.

⚠️ Disclaimer:
   Please don't use parallel fibonacci in production!
   Use the fast doubling method with memoization instead.

Fibonacci benchmark has 3 draws:

1. It's very simple to implement
2. It's unbalanced and efficiency requires distributions to avoid idle cores.
3. It's a very effective scheduler overhead benchmark, because the basic task is very trivial and the task spawning grows at 2^n scale.

## Depth first search (dfs)

From [Staccato](https://github.com/rkuchumov/staccato), this benchmark also spawns many tasks in a broader, shallower tree. Similar to fibonacci it primarily tests scheduler overhead.

## Reduction (reduce)

This benchmark computes the sum of the numbers in an array. For smaller arrays it is CPU bound and tests scheduler overhead, for larger arrays it should become memory bound.

## Cache oblivious matrix multiplication (matmul)

From [Staccato](https://github.com/rkuchumov/staccato) and Cilk, see the paper ``Cache-Oblivious Algorithms'', by Matteo Frigo, Charles E. Leiserson, Harald Prokop, and Sridhar Ramachandran, FOCS 1999, for an explanation of why this algorithm is good for caches.

This benchmark recursively subdivides a matrix before computing a matrix multiplication, hence it test a balance of CPU/memory/scheduler overhead.




