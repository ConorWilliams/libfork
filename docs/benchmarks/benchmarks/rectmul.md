---
icon: lucide/table
---

# Rectangular Matrix Multiply

The rectangular matrix multiply benchmark follows nowa's `rectmul` test. It
stores matrices as `16 x 16` blocks and recursively multiplies block matrices
`A(x, y) * B(y, z)` into `R(x, z)`.

For the suite sizes used here `x == y == z`, so the shape is square, but the
algorithm keeps nowa's rectangular recursive split. At each step it splits the
largest block dimension. Splitting the shared `y` dimension requires a temporary
matrix for one half and a recursive add into `R`.

## Complexity

The benchmark performs classical matrix multiplication:

\[
T_1 = \mathcal{O}(n^3)
\]

The recursive decomposition exposes independent subproblems when splitting the
row or column dimensions. Splitting the shared dimension has an additional
dependency because the temporary product must be added into the result.

## Scaling

`rectmul` is a regular divide-and-conquer benchmark with larger leaf work than
the scalar matrix multiply benchmark because each leaf is a `16 x 16` block
multiply. It is most comparable to [Matrix multiply](matmul.md), but exercises a
different blocked storage layout and recursive shape.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Block |
|------|-------------|-------|
| test | `512 x 512` | `16 x 16` |
| base | `4096 x 4096` | `16 x 16` |

## Results

TODO: results

## Implementation

Following nowa, both inputs are initialized to `1.0`. Every output entry must
therefore equal the shared dense dimension `n`; the checker scans the block
matrix for that value.
