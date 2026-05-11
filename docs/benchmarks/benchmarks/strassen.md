---
icon: lucide/blocks
---

# Strassen

The Strassen benchmark computes square `float` matrix multiplication using the
classic seven-product recursive algorithm. It reduces the number of recursive
multiplications from eight to seven, at the cost of extra additions,
subtractions, temporary storage, and more complicated memory access.

Source:

- [shared matrix helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/matmul.hpp)
- [serial Strassen implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/strassen.cpp)
- [matrix multiply comparison](matmul.md)

## What It Measures

`test` uses `64 x 64`; `base` uses `1024 x 1024`. The benchmark compares against
a conventional iterative reference multiply and accepts a larger relative-error
tolerance than the divide-and-conquer cubic benchmark.

The implementation stops recursing at `64 x 64`, where it uses a naive cubic
multiply. Each recursive level allocates temporary buffers for two sums and
seven products.

## Scaling

Strassen exposes seven independent recursive products per level, so a parallel
implementation can scale well above the cutoff. Scaling is often constrained by
temporary allocation, additions and subtractions around each product, cache
locality, and numeric overhead.

The algorithm has lower asymptotic arithmetic count than cubic multiply, but it
is not automatically faster at moderate sizes. Cutoff selection is critical.

## Bottlenecks And Granularity

Memory pressure is higher than regular matrix multiply because each level
allocates nine `m x m` temporary matrices. The extra matrix additions are
bandwidth-sensitive. Tasks below the cutoff are too small to schedule, while
tasks above the cutoff should represent full submatrix products or groups of
matrix additions.

## References

- [Strassen, "Gaussian Elimination is not Optimal"](https://eudml.org/doc/131927)
- [DOI: 10.1007/BF02165411](https://doi.org/10.1007/BF02165411)
- [BLAS reference implementation](https://www.netlib.org/blas/)
