---
icon: lucide/triangle
---

# Cholesky

The Cholesky benchmark computes the lower-triangular factorization:

\[
A = LL^T
\]

for a symmetric positive-definite matrix. It is adapted from the nowa Cholesky
benchmark and keeps the same sparse quadtree representation: internal nodes
split the matrix into four quadrants, missing all-zero quadrants are represented
by null children, and leaves are fixed `4 x 4` dense blocks.

The recursive step factors the upper-left quadrant, solves the lower-left block,
updates the lower-right Schur complement, and then factors the lower-right
quadrant. If the lower-left child is absent, the two diagonal quadrants can be
factored independently, matching nowa's sparse fast path. The base case is a
conventional dense Cholesky kernel over a `4 x 4` leaf block.

The benchmark constructs the same style of sparse positive-definite workload as
nowa: diagonal entries are present, additional lower-triangular entries are
inserted at random positions, and the matrix is padded to a power of two with an
identity block. Each timed iteration copies the sparse tree, factors the copy,
and verifies the result by checking \(A - LL^T\).

## Complexity

Dense Cholesky has cubic work:

\[
T_1 = \mathcal{O}(n^3)
\]

The sparse quadtree workload can do less work when large subtrees are absent.
Its exact cost depends on the fill pattern introduced during factorization, but
the dense bound is still the useful upper bound. The top-level quadrant
dependency chain is sequential, while independent sparse subtrees, block
back-substitution, and Schur updates expose parallel work:

\[
T_\infty = \mathcal{O}(n^2)
\]

The benchmark stores only allocated quadtree nodes and `4 x 4` leaves, so the
space usage scales with the number of populated sparse blocks rather than always
allocating \(\mathcal{O}(n^2)\) entries.

## Scaling

Cholesky is a structured linear-algebra workload with less available
parallelism than matrix multiply because each trailing update depends on the
previous panel factorization. In this sparse form, scaling also depends on the
quadtree shape: null quadrants reduce arithmetic work, but they make the task
graph less uniform.

This makes it a useful contrast with [matrix multiply](matmul.md) and
[LU](lu.md): all three use recursive block structure, but Cholesky has a
stronger dependency chain and this benchmark additionally exercises sparse-tree
task pruning.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Leaf cutoff | Nonzeros |
|------|-------------|-------------|----------|
| test | `64 x 64` | `4 x 4` | `640` |
| base | `1024 x 1024` | `4 x 4` | `10'240` |

## Results

TODO: results
