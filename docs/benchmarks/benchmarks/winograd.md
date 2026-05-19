---
icon: lucide/table-2
---

# Winograd

The Winograd benchmark computes `C = A * B` for square `float` matrices using
the optimized Strassen-family schedule from nowa's Strassen benchmark. It uses
the same initialization and checking code as [Strassen](strassen.md) and
[matrix multiply](matmul.md), so the benchmark isolates the recursive kernel
schedule rather than changing the matrix data.

The algorithm still performs seven half-size recursive products, but it uses a
different addition schedule from the textbook Strassen form. The nowa-derived
kernel builds eight temporary sums or differences:

\[
\begin{aligned}
S_1 &= A_{10} + A_{11} \\
S_2 &= S_1 - A_{00} \\
S_3 &= A_{00} - A_{10} \\
S_4 &= A_{01} - S_2 \\
S_5 &= B_{01} - B_{00} \\
S_6 &= B_{11} - S_5 \\
S_7 &= B_{11} - B_{01} \\
S_8 &= S_6 - B_{10}
\end{aligned}
\]

It then computes recursive products into a mixture of scratch buffers and output
quadrants. A fused combine pass recovers the final quadrants from those
intermediate values. Compared with the textbook [Strassen](strassen.md)
benchmark, this reduces and fuses some temporary work, but makes the kernel less
direct to read.

## Complexity

Winograd has the same asymptotic recurrence as Strassen:

\[
T_1(n) = 7T_1(n / 2) + \mathcal{O}(n^2)
\]

By the [master theorem](https://en.wikipedia.org/wiki/Master_theorem_%28analysis_of_algorithms%29):

\[
T_1 = \mathcal{O}(n^{\log_2 7})
\]

which is approximately \(\mathcal{O}(n^{2.807})\). The optimized schedule
changes the constant factors in the quadratic work around the recursive
products, not the asymptotic work.

The span is still dominated by a recursive product plus the surrounding matrix
preparation and combine phases:

\[
T_\infty = \mathcal{O}(n^2)
\]

## Scaling

The seven recursive products are independent and expose the same
divide-and-conquer task structure as Strassen. The difference is in the
surrounding bulk work: Winograd uses fewer scratch matrices and a fused combine
schedule, which can improve locality and reduce addition traffic, but the
implementation is more sensitive to cutoff choices.

The benchmark switches to a divide-and-conquer classical multiply below
`128 x 128`, and to a naive cubic kernel below `16 x 16`. For these sizes,
Winograd is still primarily a tasking benchmark rather than a tuned BLAS-style
matrix kernel.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Cutoffs |
|------|-------------|---------|
| test | `64 x 64` | divide-and-conquer below `128 x 128`, naive below `16 x 16` |
| base | `2048 x 2048` | divide-and-conquer below `128 x 128`, naive below `16 x 16` |

## Results

TODO: results

## Implementation

Winograd uses the shared matrix multiply benchmark setup: dense
diagonal-plus-rank-3 inputs, with the expected result computed from the
closed-form low-rank product. The input data and checker are therefore the same
as [matrix multiply](matmul.md) and [Strassen](strassen.md); only the recursive
schedule and cutoff behavior differ.
