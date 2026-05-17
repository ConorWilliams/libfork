---
icon: lucide/refresh-cw
---

# Matrix transpose

The matrix transpose benchmark transposes a square `float` matrix in place. For
an input matrix `A`, every element moves from row `i`, column `j` to row `j`,
column `i`:

\[
A^T_{ij} = A_{ji}
\]

The benchmark uses the cache-oblivious divide-and-conquer structure described
by Algorithmica's [matrix transposition
article](https://en.algorithmica.org/hpc/external-memory/oblivious/#matrix-transposition).
Split the matrix into quadrants:

\[
\begin{bmatrix}
A_{00} & A_{01} \\
A_{10} & A_{11}
\end{bmatrix}^T
=

\begin{bmatrix}
A_{00}^T & A_{10}^T \\
A_{01}^T & A_{11}^T
\end{bmatrix}
\]

The diagonal quadrants are transposed recursively in place. The off-diagonal
quadrants are swapped while being transposed, so the implementation does not
need temporary submatrix copies:

\[
\operatorname{swap\_transpose}(A_{01}, A_{10})
\]

The recursion stops at a small base case that performs the direct triangular
swap:

```cpp linenums="1"
for (unsigned i = 0; i < n; ++i)
  for (unsigned j = 0; j < i; ++j)
    swap(A[i, j], A[j, i]);
```

## Complexity

The in-place transpose touches each matrix entry a constant number of times.
The work recurrence is:

\[
T_1(n) = 2T_1(n / 2) + S_1(n / 2) + \mathcal{O}(1)
\]

where \(S_1\) is the recursive off-diagonal swap-transpose:

\[
S_1(n) = 4S_1(n / 2) + \mathcal{O}(1)
\]

Thus:

\[
T_1 = \mathcal{O}(n^2)
\]

The libfork implementation exposes the two diagonal transposes and the
off-diagonal swap-transpose as child tasks. The swap-transpose itself splits
into four independent child tasks, giving a regular divide-and-conquer task
graph with a fixed base-case cutoff.

## Scaling

Transpose is memory-bandwidth dominated: it performs little arithmetic and
mostly moves cache lines. The recursive layout improves locality by eventually
working on cache-sized submatrices without hard-coding cache parameters.

Compared with [matrix multiply](matmul.md) and [Strassen](strassen.md),
transpose has much lower arithmetic intensity. It is useful for measuring the
cost of fine-grained recursive tasking on a bandwidth-bound matrix workload.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Base case |
|------|-------------|-----------|
| test | `64 x 64` | `32 x 32` |
| base | `8192 x 8192` | `32 x 32` |

## Results

TODO: results

## Implementation

The benchmark reuses the matrix-value generator from the matrix multiply
benchmark to create a dense, non-symmetric input matrix. Before each timed
iteration, the working matrix is reset outside the timing window. The checker
then verifies every output entry against the closed-form transposed input value:

\[
A_{ij} = \operatorname{input}(j, i)
\]
