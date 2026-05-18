---
icon: lucide/waves
---

# FFT

The FFT benchmark computes a complex discrete Fourier transform using the
mixed-radix Cooley-Tukey decomposition from nowa's FFT benchmark.

For an input vector \(x\), the transform is:

\[
X_k = \sum_{j=0}^{n - 1} x_j e^{-2\pi i j k / n}
\]

Each recursive step chooses a radix with nowa's factor rule: prefer radix `16`,
then radix `8`, `4`, `2`, and finally an odd factor if needed. The input is
unshuffled into \(r\) contiguous sub-transforms of size \(m = n / r\), those
sub-transforms are computed recursively, and a twiddle phase performs the
remaining \(r\)-point transforms:

\[
n = r m
\]

For the power-of-two sizes used here, this means the task graph is not a pure
binary tree. It commonly uses radix-8 or radix-16 splits, with explicit
unshuffle and twiddle phases around the recursive work. The benchmark checks a
deterministic set of output frequencies against direct DFT samples.

## Complexity

For a fixed small radix, the work recurrence is:

\[
T_1(n) = rT_1(n / r) + \mathcal{O}(n)
\]

so:

\[
T_1 = \mathcal{O}(n \log n)
\]

The recursive sub-transforms are independent, but the unshuffle and twiddle
phases at each level are bulk memory and arithmetic passes:

\[
T_\infty = \mathcal{O}(n)
\]

## Scaling

FFT exposes regular divide-and-conquer parallelism, but each level has
memory-heavy unshuffle and twiddle passes. At larger sizes, scaling is often
limited by cache traffic, memory bandwidth, and the bulk work along the critical
path.

Compared with [matrix multiply](matmul.md), FFT has asymptotically less work and
lower arithmetic intensity, so it stresses tasking overhead and memory movement
more directly.

## Benchmark sizes

The following problem sizes are available:

| Name | Exponent | Transform size | Base kernel | Bulk cutoffs |
|------|----------|----------------|-------------|--------------|
| test | `12` | `4096` | `32` | unshuffle `16`, twiddle `128` |
| base | `26` | `67'108'864` | `32` | unshuffle `16`, twiddle `128` |

## Results

TODO: results
