---
icon: lucide/git-pull-request
---

# Contributing

This repository currently targets C++26, CMake module file sets, and C++23
`import std`. Build configuration must use the platform toolchain file.

## Dependencies

Install development dependencies with Homebrew.

macOS:

```sh
brew install cmake ninja catch2 google-benchmark clang-format codespell llvm
```

Linux:

```sh
brew install cmake ninja catch2 google-benchmark clang-format codespell gcc binutils
```

## Configure, build, test

Use `ci-hardened` for normal development:

```sh
cmake --preset ci-hardened -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake
cmake --build --preset ci-hardened
ctest --preset ci-hardened
```

On Linux, use:

```sh
cmake --preset ci-hardened -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-brew-toolchain.cmake
```

Use `ci-release` for benchmarks:

```sh
cmake --preset ci-release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake
cmake --build --preset ci-release
```

Expected warnings include CMake's experimental `import std` warning and the
benchmark warning about release mode when building benchmarks through
`ci-hardened`.

## Documentation

The docs site is built with zensical. With the Python project environment:

```sh
uv sync --group dev
uv run zensical serve
uv run zensical build --clean
```

Without `uv`, install zensical directly:

```sh
python -m pip install zensical
zensical serve
zensical build --clean
```

The configured output directory is `build/site`.

## Source changes

Module files live under `src/`. If a source or public header file is added or
removed, update the root `CMakeLists.txt` module/header file sets. Tests live in
`test/src/` and are discovered recursively by CMake.

For API changes, update the matching docs page under `docs/api/` and add or
adjust tests that exercise the behavior.

## Benchmarks

Benchmark sources live under `benchmark/`. Use the release preset before
comparing timings. The benchmark target depends on Google Benchmark and is
enabled when `libfork_DEV_MODE=ON`, which is set by the CI presets.
