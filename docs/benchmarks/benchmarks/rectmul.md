---
icon: lucide/table
---

# Rectangular Matrix Multiply

The rectangular matrix multiply benchmark is derived from nowa's `rectmul` test.
It stores matrices as `16 x 16` blocks and recursively multiplies block
matrices `A(x, y) * B(y, z)` into `R(x, z)`.

The suite uses deliberately skewed rectangular block dimensions. For a benchmark
argument `n`, the row block count is `n / 16`, while the shared and column block
counts are approximately one third and one fifth of that value, rounded up to an
odd count. At each recursive step the algorithm splits the largest block
dimension. Splitting the shared `y` dimension requires a temporary matrix for
one half and a recursive add into `R`.

## Complexity

The benchmark performs classical matrix multiplication over rectangular dense
dimensions \(m \times k\) and \(k \times p\):

\[
T_1 = \mathcal{O}(mkp)
\]

The recursive decomposition exposes independent subproblems when splitting the
row or column dimensions. Splitting the shared dimension has an additional
dependency because the temporary product must be added into the result.

## Scaling

`rectmul` has larger leaf work than the scalar matrix multiply benchmark because
each leaf is a `16 x 16` block multiply. It is most comparable to
[Matrix multiply](matmul.md), but exercises a blocked storage layout, rectangular
subproblems, and ragged odd splits.

## Benchmark sizes

The following problem sizes are available:

| Name | Block dimensions `x,y,z` | Dense multiply | Block |
|------|--------------------------|----------------|-------|
| test | `32, 11, 7` | `(512 x 176) * (176 x 112) -> (512 x 112)` | `16 x 16` |
| base | `256, 85, 51` | `(4096 x 1360) * (1360 x 816) -> (4096 x 816)` | `16 x 16` |

## Results

TODO: results

## Implementation

The block matrices use the same deterministic low-rank generator as the square
matrix multiply benchmarks. `A` is filled from the shared left-hand
diagonal-plus-rank-3 formula and `B` from the shared right-hand formula. The
checker verifies each block entry against the corresponding closed-form
rectangular product, using the actual shared dense dimension `y * 16`.
