---
icon: lucide/table-cells-split
---

# LU

The LU benchmark computes an in-place block LU decomposition:

\[
A = LU
\]

where \(L\) is unit lower-triangular and \(U\) is upper-triangular. It is adapted
from nowa's recursive blocked LU benchmark. The generated matrix is built from a
known deterministic \(L\) and \(U\), so validation compares the computed in-place
factorization with those expected factors.

Each recursive step factors the upper-left quadrant, solves the upper-right and
lower-left quadrants, applies a Schur-complement update to the lower-right
quadrant, and then factors the lower-right quadrant.

## Complexity

Dense LU decomposition has cubic work:

\[
T_1 = \mathcal{O}(n^3)
\]

The solve phases and Schur updates expose parallel subproblems, but the two
panel factorizations remain on the dependency chain. With a fixed block base
case, the span is:

\[
T_\infty = \mathcal{O}(n^2)
\]

The matrix and expected factors require \(\mathcal{O}(n^2)\) storage.

## Scaling

LU has less parallelism than [matrix multiply](matmul.md) because the trailing
submatrix cannot be updated until the current panel is factored and solved. It
still exposes substantial work in the Schur-complement updates.

This benchmark is structurally close to [Cholesky](cholesky.md), but LU has
separate lower and upper solve phases and does not rely on symmetry.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Block size |
|------|-------------|------------|
| test | `256 x 256` | `16 x 16` |
| base | `4096 x 4096` | `16 x 16` |

## Results

TODO: results
