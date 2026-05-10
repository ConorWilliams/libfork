---
icon: lucide/activity
---

# Benchmarks

The benchmark suite is present in the repository but this docs section is a
stub for published results.

Current benchmark groups include:

- recursive Fibonacci task overhead;
- `fold` over memory-backed and lazy ranges;
- unbalanced tree search (UTS);
- explicit scheduler switching between pools;
- serial, OpenMP, bare-metal, and libfork implementations where available.

Build benchmarks in release mode:

```sh
cmake --preset ci-release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake
cmake --build --preset ci-release
```

On Linux, use `cmake/gcc-brew-toolchain.cmake`.

Benchmark code is organized under:

- `benchmark/lib/` for shared benchmark utilities;
- `benchmark/src/libfork/` for libfork implementations;
- `benchmark/src/serial/`, `benchmark/src/openmp/`, and other implementation
  directories for comparisons;
- `benchmark/external/` for bundled benchmark inputs such as UTS.

Future versions of this page should include reproducible machine details,
compiler versions, command lines, raw result artifacts, and plots.
