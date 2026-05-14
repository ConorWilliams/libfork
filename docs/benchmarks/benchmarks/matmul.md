---
icon: lucide/grid-3x3
---

# Matrix Multiply

The matrix multiply benchmark computes `C = A * B` for square `float`
matrices. The serial implementation is recursive divide and conquer: split each
matrix into quadrants, compute the eight half-size products, and add the second
wave of products into the output quadrants.

The recursion stops at a conventional cubic base case:

```cpp linenums="1"
for (unsigned i = 0; i < n; ++i)
  for (unsigned j = 0; j < n; ++j)
    for (unsigned k = 0; k < n; ++k)
      C[i, j] += A[i, k] * B[k, j];
```

## Complexity

The recursive algorithm does eight subproblems of half the size:

\[
T_1(n) = 8T_1(n / 2) + \mathcal{O}(1)
\]

so the work is cubic:

\[
T_1 = \mathcal{O}(n^3)
\]

If the independent products at each level are exposed as tasks, the critical
path follows one product per level down to the base case:

\[
T_\infty = \mathcal{O}(\log n)
\]

with a constant factor from the base-case multiply.

## Scaling

Matrix multiply has high arithmetic intensity at large sizes, so there is
substantial parallel work. The task graph is regular: every recursive split
creates products of the same shape.

The benchmark is intentionally not a tuned BLAS kernel. Scaling can still be
limited by cache locality, write traffic when accumulating output quadrants,
and the chosen base-case size.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Base case |
|------|-------------|-----------|
| test | `64 x 64` | `32 x 32` |
| base | `1024 x 1024` | `32 x 32` |

## Results

TODO: results
