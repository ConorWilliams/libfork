---
icon: lucide/triangle
---

# Cholesky

The Cholesky benchmark computes the lower-triangular factorization:

\[
A = LL^T
\]

for a symmetric positive-definite matrix. It is adapted from the nowa Cholesky
benchmark, but uses deterministic generated inputs with a known factor `L` so
the result can be checked directly.

The recursive step factors the upper-left quadrant, solves the lower-left block,
updates the lower-right Schur complement, and then factors the lower-right
quadrant. The base case is a conventional dense Cholesky kernel.

## Complexity

The dense factorization has cubic work:

\[
T_1 = \mathcal{O}(n^3)
\]

The top-level quadrant dependency chain is sequential, but the triangular solve
and Schur update expose parallel row ranges:

\[
T_\infty = \mathcal{O}(n^2)
\]

The benchmark stores the dense matrix and the expected lower factor, so the
space complexity is \(\mathcal{O}(n^2)\).

## Scaling

Cholesky is a structured linear-algebra workload with less available
parallelism than matrix multiply because each trailing update depends on the
previous panel factorization. Scaling mostly comes from the solve and Schur
update phases.

This makes it a useful contrast with [matrix multiply](matmul.md) and
[LU](lu.md): all three are dense matrix kernels, but Cholesky has a stronger
left-to-right dependency chain.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Base case |
|------|-------------|-----------|
| test | `64 x 64` | `32 x 32` |
| base | `1024 x 1024` | `32 x 32` |

## Results

TODO: results
