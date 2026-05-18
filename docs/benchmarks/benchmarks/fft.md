---
icon: lucide/waves
---

# FFT

The FFT benchmark computes a complex discrete Fourier transform using a
recursive radix-2 Cooley-Tukey decomposition adapted from nowa's FFT benchmark.

For an input vector \(x\), the transform is:

\[
X_k = \sum_{j=0}^{n - 1} x_j e^{-2\pi i j k / n}
\]

Each recursive step splits the input into even and odd subsequences, computes
two half-size transforms, and combines them with precomputed twiddle factors:

\[
X_k = E_k + \omega_n^k O_k,\qquad
X_{k+n/2} = E_k - \omega_n^k O_k
\]

The benchmark checks a deterministic set of output frequencies against direct
DFT samples.

## Complexity

The work recurrence is:

\[
T_1(n) = 2T_1(n / 2) + \mathcal{O}(n)
\]

so:

\[
T_1 = \mathcal{O}(n \log n)
\]

The two recursive transforms are independent, but the combine phase at each
level is linear in the subproblem size:

\[
T_\infty = \mathcal{O}(n)
\]

## Scaling

FFT exposes regular divide-and-conquer parallelism, but each level has a
memory-heavy combine pass. At larger sizes, scaling is often limited by cache
traffic, memory bandwidth, and the serial combine work along the critical path.

Compared with [matrix multiply](matmul.md), FFT has asymptotically less work and
lower arithmetic intensity, so it stresses tasking overhead and memory movement
more directly.

## Benchmark sizes

The following problem sizes are available:

| Name | Exponent | Transform size | Task cutoff |
|------|----------|----------------|-------------|
| test | `12` | `4096` | `1024` |
| base | `26` | `67'108'864` | `1024` |

## Results

TODO: results
