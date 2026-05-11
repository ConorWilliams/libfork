---
icon: lucide/thermometer-sun
---

# Heat

The heat benchmark runs a fixed number of Jacobi sweeps over a square grid.
Interior cells are updated from their four direct neighbors, while boundary
cells remain clamped. The final grid is compared against a reference run.

Source:

- [shared heat setup](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/heat.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/heat.cpp)

## What It Measures

`test` uses a `64 x 64` grid; `base` uses `1024 x 1024`. Each benchmark run
performs 16 sweeps using two grids and swaps the active buffers between sweeps.
The initial condition is a deterministic analytic profile.

## Scaling

A parallel version should scale by splitting rows, tiles, or bands among
workers. Each sweep has a global phase boundary because the next sweep depends
on the previous sweep's full output. Good scaling is expected within a sweep
until memory bandwidth dominates, but the per-sweep synchronization point limits
the benefit of very small grids.

## Bottlenecks And Granularity

This is a memory-bandwidth benchmark. Each interior update reads four doubles
and writes one double, with little arithmetic per byte. Cache blocking can help
larger grids by improving locality, but the simple serial baseline streams rows.

Granularity should be coarse enough that each task owns many contiguous rows or
tiles. Per-cell tasks would be dominated by scheduling overhead. Boundary
handling is small and should not be parallelized separately.

## References

- [Jacobi method overview](https://en.wikipedia.org/wiki/Jacobi_method)
- [Stencil computation overview](https://en.wikipedia.org/wiki/Stencil_code)
- [OpenMP loop scheduling background](https://www.openmp.org/spec-html/5.2/openmpse66.html)
