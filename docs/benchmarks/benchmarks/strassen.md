---
icon: lucide/table-2
---

# Strassen

The Strassen benchmark computes `C = A * B` for square `float` matrices using
Strassen's seven-product divide-and-conquer algorithm. Each recursive step
forms temporary matrix sums and differences, computes seven half-size products,
and combines them into the four output quadrants.

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

## Complexity

Strassen's recurrence is:

\[
T_1(n) = 7T_1(n / 2) + \mathcal{O}(n^2)
\]

By the [master theorem](https://en.wikipedia.org/wiki/Master_theorem_%28analysis_of_algorithms%29):

\[
T_1 = \mathcal{O}(n^{\log_2 7})
\]

which is approximately \(\mathcal{O}(n^{2.807})\). The implementation allocates
temporary matrices at each recursive level, so memory traffic is a major part
of the benchmark.

In the libfork benchmark, the seven half-size products are spawned as
independent recursive tasks. The matrix additions and subtractions that build
the temporary inputs, and the final output combination, are serial quadratic
work around those tasks. That gives the span recurrence:

\[
T_\infty(n) = T_\infty(n / 2) + \mathcal{O}(n^2)
\]

With the fixed base-case cutoff used here, this is:

\[
T_\infty = \mathcal{O}(n^2)
\]

The exposed parallelism is therefore roughly
\(\mathcal{O}(n^{\log_2 7 - 2})\). Strassen has less work than the classical
eight-product recurrence, but the serial matrix-sum phases are a larger part
of the critical path in this benchmark.

## Scaling

The seven products are independent and expose regular divide-and-conquer
parallelism. The matrix additions, subtractions, and final combination are
bulk work around those recursive tasks.

For these sizes, Strassen is a tasking benchmark rather than a tuned linear
algebra kernel. Scaling depends on the cutoff, temporary allocation behavior,
cache locality, and floating-point error tolerance.

This benchmark is directly comparable to [matrix multiply](matmul.md): both
split matrices into quadrants, but Strassen trades extra additions and
temporaries for one fewer recursive product.

## Benchmark sizes

The following problem sizes are available:

| Name | Matrix size | Cutoff |
|------|-------------|--------|
| test | `64 x 64` | `32 x 32` |
| base | `1024 x 1024` | `32 x 32` |

## Results

TODO: results
