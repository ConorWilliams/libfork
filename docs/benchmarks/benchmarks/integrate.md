---
icon: lucide/chart-area
---

# Integrate

The integrate benchmark computes an adaptive trapezoidal integral of
`(x * x + 1) * x` over `[0, n]`. The recursion subdivides until the split
trapezoids agree with the parent area within a fixed epsilon. The result is
checked against the exact polynomial integral.

Source:

- [shared integration helpers](https://github.com/conorwilliams/libfork/blob/main/benchmark/lib/integrate.hpp)
- [serial implementation](https://github.com/conorwilliams/libfork/blob/main/benchmark/src/serial/integrate.cpp)

## What It Measures

`test` uses `n = 100`; `base` uses `n = 10000`. The benchmark stresses recursive
adaptive control flow rather than dense numeric kernels. Work is concentrated in
intervals where the trapezoid approximation needs further subdivision.

## Scaling

A parallel version can fork the two subintervals whenever an interval fails the
error test. Scaling depends on how quickly the recursion exposes enough
independent intervals. Smooth functions with balanced subdivision are easier to
parallelize than functions where one side of the domain keeps subdividing much
more deeply than the other.

## Bottlenecks And Granularity

The computation is mostly floating-point arithmetic and branch-heavy recursion.
There is almost no shared memory traffic. Task granularity should use a cutoff
or depth threshold because near the leaves each interval performs only a few
floating-point operations. Without a cutoff, scheduling can cost more than the
quadrature step.

## References

- [Adaptive quadrature overview](https://en.wikipedia.org/wiki/Adaptive_quadrature)
- [Trapezoidal rule overview](https://en.wikipedia.org/wiki/Trapezoidal_rule)
- [Scheduling multithreaded computations by work stealing](https://doi.org/10.1145/324133.324234)
