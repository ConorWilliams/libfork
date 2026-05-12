---
icon: lucide/terminal
---

# Reproducing Results

The benchmark suite is built on Google Benchmark and compiled through the
project's release CMake preset.

## Compiling

Configure and build with the release preset and the platform toolchain file.

=== "macOS"

    ```sh
    cmake --preset ci-release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake
    cmake --build --preset ci-release
    ```

=== "Linux"

    ```sh
    cmake --preset ci-release -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-brew-toolchain.cmake
    cmake --build --preset ci-release
    ```

The benchmark executable is:

```sh
build/ci-release/benchmark/libfork_benchmark
```

## Running

Benchmark names use this shape:

```text
<mode>/<category>/<name>[/template-or-argument-tags]
```

The main modes are:

- `test`: smoke-sized inputs used by CTest and quick local checks.
- `base`: default comparison inputs for normal benchmark runs.
- `large`: large UTS inputs for machines where the runtime and working set are
  acceptable.

Useful commands:

```sh
# List registered benchmarks.
build/ci-release/benchmark/libfork_benchmark --benchmark_list_tests

# Fast smoke run. This is also what CTest dry-runs.
build/ci-release/benchmark/libfork_benchmark --benchmark_filter='^test/'

# Run the default comparison inputs.
build/ci-release/benchmark/libfork_benchmark --benchmark_filter='^base/'

# Run one family or implementation category.
build/ci-release/benchmark/libfork_benchmark --benchmark_filter='^base/.*/fib'
build/ci-release/benchmark/libfork_benchmark --benchmark_filter='^base/libfork/'

# Save JSON output for plotting or later analysis.
build/ci-release/benchmark/libfork_benchmark \
  --benchmark_filter='^base/' \
  --benchmark_out=build/ci-release/benchmarks.json \
  --benchmark_out_format=json
```

Approximate runtime depends heavily on CPU, compiler, worker count, and thermal
policy. As a rule of thumb, `test` should take seconds, `base` should take
minutes for the full suite, and `large` should be treated as a longer-running
UTS stress run rather than part of the default loop.

## Plotting

Plotting documentation will be added once the plotting scripts and expected
output layout are checked in.
