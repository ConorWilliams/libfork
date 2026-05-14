---
icon: lucide/scan-line
---

# Scan

The scan benchmark computes an inclusive prefix sum over the input sequence
`1, 2, ..., n`:

```cpp linenums="1"
out[i] = in[0] + in[1] + ... + in[i];
```

The serial projection uses `std::inclusive_scan`. Each benchmark iteration
repeats the scan many times and checks that the last output element is
\(n(n + 1) / 2\), modulo 32-bit unsigned arithmetic.

## Complexity

For one scan over \(n\) elements, the work is:

\[
T_1 = \mathcal{O}(n)
\]

Parallel scan is usually implemented as an upsweep and downsweep over chunks,
giving logarithmic dependency depth:

\[
T_\infty = \mathcal{O}(\log n)
\]

The benchmark stores both input and output arrays, so the space complexity is
\(\mathcal{O}(n)\).

## Scaling

Scan is a regular bulk-parallel benchmark, but it has more synchronization than
fold because prefix sums must propagate chunk totals back into later chunks.

Scaling is limited by memory bandwidth, the number of global scan phases, and
the fixed per-iteration synchronization cost. Repeating the scan reduces timing
noise for small inputs.

## Benchmark sizes

The following problem sizes are available:

| Name | Elements | Repetitions |
|------|----------|-------------|
| test | `1'000` | `1'000` |
| base | `8'000` | `1'000` |

## Results

TODO: results
