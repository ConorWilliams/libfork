---
icon: lucide/area-chart
---

# Quadrature Integrate

The quadrature integrate benchmark is adapted from nowa's `integrate` benchmark.
It computes:

\[
\int_0^n (x^2 + 1)x\,dx
\]

using adaptive trapezoidal quadrature. For an interval \([x_1, x_2]\), it
compares the trapezoid estimate over the whole interval with the sum of two
half-interval trapezoids. If the estimates differ by less than the fixed
tolerance, the interval is accepted; otherwise both halves are refined
recursively.

The exact integral is:

\[
\frac{n^4}{4} + \frac{n^2}{2}
\]

and is used as the benchmark oracle.

## Complexity

The amount of work is proportional to the number of accepted intervals:

\[
T_1 = \mathcal{O}(m)
\]

where \(m\) is the leaf count of the adaptive recursion tree. The span is the
maximum refinement depth:

\[
T_\infty = \mathcal{O}(d)
\]

The benchmark reports both values as counters.

## Scaling

Quadrature integration is an irregular divide-and-conquer workload. The right
side of the interval needs more refinement because the cubic integrand has
higher curvature there, so the two children of a split can have different
future costs.

This benchmark sits alongside [Simpson integrate](integrate.md). Both are
adaptive integration workloads; this one uses a simpler trapezoid rule and a
monotone polynomial, while Simpson integrate uses a multi-peak function and
Simpson error correction.

## Benchmark sizes

The following problem sizes are available:

| Name | Domain | Tolerance |
|------|--------|-----------|
| test | `[0, 64]` | `1.0e-9` |
| base | `[0, 10'000]` | `1.0e-9` |

## Results

TODO: results
