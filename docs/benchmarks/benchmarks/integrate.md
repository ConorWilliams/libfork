---
icon: lucide/area-chart
---

# Integrate

The integrate benchmark computes the definite integral of:

\[
f(x) = \sum_{i=0}^{p - 1}\frac{1}{z + (x - c_i)^2}
\]

It uses [adaptive Simpson's
rule](https://en.wikipedia.org/wiki/Adaptive_Simpson%27s_method): estimate the
area of an interval with Simpson's rule, split the interval in half, compare
the combined child estimates with the parent estimate, and recurse until the
estimated error is below the configured tolerance.

For an interval \([a, b]\) with midpoint \(m\), the Simpson estimate is:

\[
S(a, b) = \frac{b - a}{6}\left(f(a) + 4f(m) + f(b)\right)
\]

After splitting at \(m\), the benchmark accepts the interval when:

\[
\left|S(a, m) + S(m, b) - S(a, b)\right| \le 15\epsilon
\]

and returns the Richardson-corrected value:

\[
S(a, m) + S(m, b) + \frac{S(a, m) + S(m, b) - S(a, b)}{15}
\]

This is a non-uniform row of narrow Lorentzian peaks across the integration
domain. Most points are locally smooth, but each peak has much higher curvature,
so adaptive refinement repeatedly discovers small regions that need extra work.
Compared with a single peak, the multi-peak function creates enough independent
refinement regions for a longer base workload while preserving an exact
closed-form reference:

\[
\int_a^b f(x)\,dx =
\frac{1}{\sqrt{z}}
\sum_{i=0}^{p - 1}
\left[
  \arctan\left(\frac{x - c_i}{\sqrt{z}}\right)
\right]_{x=a}^{x=b}
\]

The benchmark records the total number of accepted leaf intervals, the maximum
recursion depth, and the number of peaks.

## Complexity

The amount of work depends on how many sub-intervals the adaptive test accepts.
If \(m\) leaf intervals are produced, then the recursion tree has linear size:

\[
T_1 = \mathcal{O}(m)
\]

The span is proportional to the deepest refinement path:

\[
T_\infty = \mathcal{O}(d)
\]

where \(d\) is the maximum recursion depth. Smooth regions terminate quickly;
regions requiring more refinement create a deeper, more irregular task graph.

## Scaling

Adaptive integration is an irregular divide-and-conquer benchmark. The two
children of a split may perform different amounts of future work, so good
scheduling depends on exposing enough small subproblems without making the
tasks too fine grained.

It is similar to [UTS](uts.md) in that the useful task graph is discovered
recursively, but here the irregularity comes from numerical error rather than
tree-generation randomness.

## Benchmark sizes

The following problem sizes are available. Both use the same integration
domain; the size controls the requested adaptive tolerance. The benchmark also
reports the peak, leaf, and depth counts as counters.

| Name | Domain | Peaks | Tolerance | Leaves | Max depth |
|------|--------|-------|-----------|--------|-----------|
| test | `[0, 1]` | `76` | `1.0e-6` | `265'489` | `23` |
| base | `[0, 1]` | `76` | `1.0e-9` | `1'806'540` | `45` |

## Results

TODO: results
