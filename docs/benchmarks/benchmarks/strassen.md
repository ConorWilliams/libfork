---
icon: lucide/table-2
---

# Strassen

The Strassen benchmark computes `C = A * B` for square `float` matrices using
Strassen's seven-product divide-and-conquer algorithm. Each recursive step
forms temporary matrix sums and differences, computes seven half-size products,
and combines them into the four output quadrants.

The recursion stops at a conventional cubic base case.

```mermaid
flowchart TD
  Root["2 x 2 block multiply"] --> M1["M1 = (A11 + A22)(B11 + B22)"]
  Root --> M2["M2 = (A21 + A22)B11"]
  Root --> M3["M3 = A11(B12 - B22)"]
  Root --> M4["M4 = A22(B21 - B11)"]
  Root --> M5["M5 = (A11 + A12)B22"]
  Root --> M6["M6 = (A21 - A11)(B11 + B12)"]
  Root --> M7["M7 = (A12 - A22)(B21 + B22)"]
  M1 --> Combine["combine C11, C12, C21, C22"]
  M2 --> Combine
  M3 --> Combine
  M4 --> Combine
  M5 --> Combine
  M6 --> Combine
  M7 --> Combine
```

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
| test | `64 x 64` | `64 x 64` |
| base | `1024 x 1024` | `64 x 64` |

## Results

TODO: results
