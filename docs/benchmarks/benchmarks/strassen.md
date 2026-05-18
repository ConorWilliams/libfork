---
icon: lucide/table-2
---

# Strassen

The Strassen benchmark computes `C = A * B` for square `float` matrices. The
default variant uses Strassen's textbook seven-product divide-and-conquer
algorithm: each recursive step forms temporary matrix sums and differences,
computes seven half-size products, and combines them into the four output
quadrants.

The recursion stops at a conventional cubic base case.

As with [matrix multiply](matmul.md), split each matrix into quadrants:

\[
\begin{bmatrix}
C_{00} & C_{01} \\
C_{10} & C_{11}
\end{bmatrix}
=
\begin{bmatrix}
A_{00} & A_{01} \\
A_{10} & A_{11}
\end{bmatrix}
\begin{bmatrix}
B_{00} & B_{01} \\
B_{10} & B_{11}
\end{bmatrix}
\]

Strassen replaces the classical eight half-size products with seven:

\[
\begin{aligned}
M_1 &= (A_{00} + A_{11})(B_{00} + B_{11}) \\
M_2 &= (A_{10} + A_{11})B_{00} \\
M_3 &= A_{00}(B_{01} - B_{11}) \\
M_4 &= A_{11}(B_{10} - B_{00}) \\
M_5 &= (A_{00} + A_{01})B_{11} \\
M_6 &= (A_{10} - A_{00})(B_{00} + B_{01}) \\
M_7 &= (A_{01} - A_{11})(B_{10} + B_{11})
\end{aligned}
\]

The output quadrants are then recovered by addition and subtraction:

\[
\begin{aligned}
C_{00} &= M_1 + M_4 - M_5 + M_7 \\
C_{01} &= M_3 + M_5 \\
C_{10} &= M_2 + M_4 \\
C_{11} &= M_1 - M_2 + M_3 + M_6
\end{aligned}
\]

## Winograd variant

The benchmark suite also includes `strassen/winograd`, a second Strassen-family
benchmark adapted from nowa's optimized Strassen implementation. It uses the
same `float` input buffers, initialization, and checker as the default Strassen
benchmark, but changes the recursive kernel shape.

The Winograd variant uses a lower-addition schedule built around eight
precomputed sums or differences and three scratch product buffers:

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

It computes the seven recursive products into a mixture of scratch buffers and
output quadrants, then performs a fused combine pass. This mirrors nowa's
optimized form more closely than the textbook variant: the recursive products
are still seven half-size multiplications, but the temporary layout, addition
schedule, and combine work are different.

## Complexity

Both variants have the same Strassen work recurrence:

\[
T_1(n) = 7T_1(n / 2) + \mathcal{O}(n^2)
\]

By the [master theorem](https://en.wikipedia.org/wiki/Master_theorem_%28analysis_of_algorithms%29):

\[
T_1 = \mathcal{O}(n^{\log_2 7})
\]

which is approximately \(\mathcal{O}(n^{2.807})\). The seven half-size products
are spawned as independent recursive tasks. The matrix additions and
subtractions that build the temporary inputs, and the final output combination,
are serial quadratic work around those tasks. That gives the span recurrence:

\[
T_\infty(n) = T_\infty(n / 2) + \mathcal{O}(n^2)
\]

With the fixed base-case cutoff used here, this is:

\[
T_\infty = \mathcal{O}(n^2)
\]

Strassen has less work than the classical eight-product recurrence, but the
matrix-sum phases are a larger part of the critical path in this benchmark. The
Winograd variant changes those matrix-sum phases: it reduces and fuses some of
the temporary work, but it is still governed by the same asymptotic recurrence.

## Scaling

The seven products are independent and expose regular divide-and-conquer
parallelism. The matrix additions, subtractions, and final combination are bulk
work around those recursive tasks.

For these sizes, Strassen is a tasking benchmark rather than a tuned linear
algebra kernel. Scaling depends on the cutoff, temporary allocation behavior, and
cache locality.

The default and Winograd variants should be read as different kernel schedules
for the same mathematical operation. The default variant is easier to inspect;
the Winograd variant is closer to nowa's optimized benchmark.

This benchmark is directly comparable to [matrix multiply](matmul.md): both
split matrices into quadrants, but Strassen trades extra additions and
temporaries for one fewer recursive product.

## Benchmark sizes

The following problem sizes are available:

| Variant | Name | Matrix size | Cutoff |
|---------|------|-------------|--------|
| `strassen` | test | `64 x 64` | `32 x 32` |
| `strassen` | base | `1024 x 1024` | `32 x 32` |
| `strassen/winograd` | test | `64 x 64` | divide-and-conquer below `128 x 128`, naive below `16 x 16` |
| `strassen/winograd` | base | `1024 x 1024` | divide-and-conquer below `128 x 128`, naive below `16 x 16` |

## Results

TODO: results
