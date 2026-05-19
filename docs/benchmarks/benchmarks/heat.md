---
icon: lucide/thermometer-sun
---

# Heat

The heat benchmark follows nowa's two-dimensional heat diffusion test. It solves
on an `nx x 1024` grid over a short fixed time interval using an explicit
Jacobi-style finite-difference step:

```cpp linenums="1"
out[i, j] = in[i, j]
          + dtdysq * (in[i, j + 1] - 2 * in[i, j] + in[i, j - 1])
          + dtdxsq * (in[i + 1, j] - 2 * in[i, j] + in[i - 1, j]);
```

The initial condition is `sin(x) * sin(y)`. Boundary values are set from the
known analytic solution `exp(-2t) * sin(x) * sin(y)`, and the final grid is
checked against that solution.

Each update reads only the four direct neighbors from the previous grid and
writes one cell in the next grid. That local stencil is why neighboring rows or
tiles tend to reuse cache lines well, while each time step still needs a global
swap before the next step can begin.

## Complexity

For an \(n \times 1024\) grid and \(k\) iterations, the work is:

\[
T_1 = \mathcal{O}(k n)
\]

Each time step depends on the previous one, so the span contains the iteration
loop. Within a single step, the interior cells are independent:

\[
T_\infty = \mathcal{O}(k)
\]

The benchmark uses two grids, so the space complexity is \(\mathcal{O}(n)\) for
the fixed-width nowa configuration.

## Scaling

Heat is a regular bulk-parallel stencil. It should distribute evenly across
workers because every interior cell performs the same amount of arithmetic.

Scaling is normally limited by memory bandwidth and cache behavior rather than
scheduler imbalance. The global iteration barrier between stencil steps also
prevents parallelism across time.

This is the most regular bulk-parallel benchmark in the suite, and is a useful
contrast with irregular per-element workloads such as [Mandelbrot](mandelbrot.md).

## Benchmark sizes

The following problem sizes are available:

| Name | Grid | Iterations |
|------|------|------------|
| test | `64 x 1024` | `100` |
| base | `4096 x 1024` | `100` |

## Results

TODO: results
