---
icon: lucide/grid-3x3
---

# Matrix Multiply

The matrix multiply benchmark computes `C = A * B` for square `float` matrices.
The serial benchmark uses a recursive divide-and-conquer implementation with
eight half-size multiplications and a conventional cubic base case.

Source:

- [shared matrix helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/matmul.hpp)
- [serial matrix multiply](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/matmul.cpp)

## What It Measures

`test` uses `64 x 64`; `base` uses `1024 x 1024`. Inputs are deterministic
random matrices. A straightforward iterative multiply computes a reference
matrix, and the benchmark checks the maximum relative error after the recursive
multiply.

The recursive implementation cuts down to a `32 x 32` base case.

## Scaling

Matrix multiply has high arithmetic intensity at large sizes, so a tuned
parallel implementation can scale well until cache, memory bandwidth, or core
floating-point throughput saturates. This benchmark is not a tuned BLAS kernel;
it is useful as a structured divide-and-conquer workload.

Parallelizing the eight recursive products exposes regular fork-join work.
However, base-case size matters: too small a base case increases task overhead
and loses cache efficiency; too large a base case underuses workers near the top
of the recursion.

## Bottlenecks And Granularity

The naive `i, k, j` base multiply and row-major layout drive cache behavior.
The recursive split improves structure but does not implement packing,
vectorized microkernels, or cache-tuned blocking. Memory pressure is dominated
by the input, output, and reference matrices.

## References

- [BLAS reference implementation](https://www.netlib.org/blas/)
- [Matrix multiplication overview](https://en.wikipedia.org/wiki/Matrix_multiplication_algorithm)
- [GotoBLAS paper](https://www.cs.utexas.edu/users/flame/pubs/GotoTOMS_revision.pdf)
