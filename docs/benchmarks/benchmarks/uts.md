---
icon: lucide/tree-pine
---

# Unbalanced tree search

The unbalanced tree search (UTS) benchmark traverses synthetic trees from the
[UTS benchmark suite](https://www.cs.unc.edu/~olivier/LCPC06.pdf). Each node
deterministically generates a pseudo-random number of children from its local
RNG state, and the traversal reports the maximum depth, total nodes, and
leaves.

The configured families are:

- `T1`: geometric trees. The branching factor decreases with depth, giving a
  bounded tree whose imbalance is visible but not extreme.
- `T3`: binomial trees. Each node samples whether it is internal, and internal
  nodes have a fixed branching factor. This creates long, skinny paths and much
  deeper irregular trees. The subtrees are self-similar.

!!! quote

    A binomial tree is an optimal adversary for load balancing strategies, since
    there is no advantage to be gained by choosing to move one node over another
    for load balance.

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

Where \(D\) is the tree's maximum depth. The `T3` family has much larger depth
than `T1`, making it substantially harder for schedulers.

## Scaling

UTS is a canonical irregular tasking benchmark. Work is discovered only by
visiting nodes, and different subtrees can have very different sizes.

The geometric `T1` cases are comparatively regular. The binomial `T3` cases
have long, skinny paths and uneven subtree sizes, so stealing and task
granularity have a larger effect on scaling and memory consumption.

UTS is similar in spirit to [N-Queens](nqueens.md) and
[knapsack](knapsack.md): the useful work is discovered by search, not known
up front.

## Benchmark sizes

The following geometric tree sizes are available:

| Name  | Family    | Nodes         | Leaves       | Max depth |
| ----- | --------- | ------------- | ------------ | --------- |
| test  | `T1_mini` | `63'914`      | `51'124`     | `7`       |
| base  | `T1`      | `4'130'071`   | `3'305'118`  | `10`      |
| large | `T1L`     | `102'181'082` | `81'746'377` | `13`      |

The following binomial tree sizes are available:

| Name  | Family    | Nodes         | Leaves       | Max depth |
| ----- | --------- | ------------- | ------------ | --------- |
| test  | `T3_mini` | `6'213`       | `5'438`      | `67`      |
| base  | `T3`      | `4'112'897`   | `3'599'034`  | `1'572`   |
| large | `T3L`     | `111'345'631` | `89'076'904` | `17'844`  |

!!! warning

    The deep recursion of the binomial `T3` family can cause stack overflows
    for the serial baseline and some libraries. You will likely need to increase
    the stack size for these cases.

## Results

TODO: results
