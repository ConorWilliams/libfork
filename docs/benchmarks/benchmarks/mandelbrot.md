---
icon: lucide/aperture
---

# Mandelbrot

The Mandelbrot benchmark computes an escape-time checksum over an `n x n` grid
covering the rectangle `[-2, 1] x [-1.5, 1.5]`. Each pixel iterates the standard
quadratic recurrence up to 256 iterations.

Source:

- [shared Mandelbrot helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/mandelbrot.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/mandelbrot.cpp)

## What It Measures

`test` uses `n = 128`; `base` uses `n = 1024`. The output is not stored as an
image. Instead, the benchmark sums all per-pixel iteration counts and checks the
checksum against a reference computation.

## Scaling

Pixels are independent, so a parallel version should scale well with row, tile,
or chunk partitioning. Load balance matters because pixels near the set boundary
usually run more iterations than pixels that escape quickly. Static row
partitioning can be uneven; tiles or dynamic chunks are usually better.

## Bottlenecks And Granularity

This benchmark is compute-bound for large grids. It has little memory traffic
because the serial version only accumulates a checksum. Branch divergence is the
main irregularity: different pixels exit after different iteration counts.

Granularity should group many pixels per task. Per-pixel tasks are too small,
while very large row blocks can leave workers idle if one block contains more
boundary-heavy pixels.

## References

- [Mandelbrot set overview](https://en.wikipedia.org/wiki/Mandelbrot_set)
- [Escape-time algorithm overview](https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set)
- [OpenMP loop scheduling background](https://www.openmp.org/spec-html/5.2/openmpse66.html)
