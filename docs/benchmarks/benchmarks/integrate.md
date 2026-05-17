---
icon: lucide/area-chart
---

# Integrate

The integrate benchmark computes the definite integral of:

\[
f(x) = \frac{1}{1 + 0.01(x - 7234.5)^2}
\]

Over the fixed interval \([0, 10000]\). It uses
[adaptive Simpson's rule](https://en.wikipedia.org/wiki/Adaptive_Simpson%27s_method):
estimate the area of an interval with Simpson's rule, split the interval in
half, compare the combined child estimates with the parent estimate, and recurse
until the estimated error is below the configured tolerance.

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
S(a, m) + S(m, b) + (S(a, m) + S(m, b) - S(a, b)) / 15
\]

This is a Lorentzian peak placed off-center in the integration domain. Most of
the interval is nearly flat, but the narrow peak has much higher curvature, so
adaptive refinement produces a less balanced tree than a polynomial workload.
The benchmark records the total number of accepted leaf intervals and the
maximum recursion depth.

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

The benchmark also checks the result against the exact antiderivative, so
incorrect pruning or floating-point instability is caught.

It is similar to [UTS](uts.md) in that the useful task graph is discovered
recursively, but here the irregularity comes from numerical error rather than
tree-generation randomness.

## Benchmark sizes

The following problem sizes are available. Both use the same integration
domain; the size controls the requested adaptive tolerance. The benchmark also
reports the leaf and depth counts as counters, in the same spirit as UTS.

| Name | Domain | Tolerance | Leaves | Max depth |
|------|--------|-----------|--------|-----------|
| test | `[0, 10000]` | `1.0e-6` | `413` | `16` |
| base | `[0, 10000]` | `1.0e-9` | `2'262` | `19` |

## Results

TODO: results
