---
icon: lucide/tree-pine
---

# Unbalanced Tree Search

The unbalanced tree search benchmark traverses synthetic trees from the UTS
suite. Each node deterministically generates a pseudo-random number of children
from its local RNG state, and the traversal reports the maximum depth, total
nodes, and leaves.

The configured families are:

- `T1`: geometric trees with a fixed maximum depth.
- `T3`: binomial trees with highly irregular depths.

## Complexity

For a generated tree with \(N\) nodes, a complete traversal performs linear
work:

\[
T_1 = \mathcal{O}(N)
\]

The span is the maximum root-to-leaf depth:

\[
T_\infty = \mathcal{O}(D)
\]

where \(D\) is the tree's maximum depth. The `T3` family has much larger depth
than `T1`, making it substantially harder for schedulers.

## Scaling

UTS is a canonical irregular tasking benchmark. Work is discovered only by
visiting nodes, and different subtrees can have very different sizes.

The geometric `T1` cases are comparatively regular. The binomial `T3` cases
have long, skinny paths and uneven subtree sizes, so stealing and task
granularity have a larger effect on scaling and memory consumption.

## Benchmark sizes

The following problem sizes are available:

| Name | Family | Nodes | Leaves | Max depth |
|------|--------|-------|--------|-----------|
| test | `T1_mini` | `63'914` | `51'124` | `7` |
| test | `T3_mini` | `6'213` | `5'438` | `67` |
| base | `T1` | `4'130'071` | `3'305'118` | `10` |
| base | `T3` | `4'112'897` | `3'599'034` | `1'572` |
| large | `T1L` | `102'181'082` | `81'746'377` | `13` |
| large | `T3L` | `111'345'631` | `89'076'904` | `17'844` |

## Results

TODO: results
