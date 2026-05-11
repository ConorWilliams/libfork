---
icon: lucide/play
---

# Installation

`libfork` is a C++26 module-based library. The current tree uses CMake module
file sets and C++23 `import std`, so the compiler and CMake invocation matter.

## Requirements

Use Homebrew-provided build tools and the repository toolchain file.

On macOS:

```sh
brew install cmake ninja catch2 google-benchmark clang-format codespell llvm
```

On Linux:

```sh
brew install cmake ninja catch2 google-benchmark clang-format codespell gcc binutils
```

The required configure command differs by platform:

```sh
# macOS
cmake --preset ci-hardened -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake

# Linux
cmake --preset ci-hardened -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-brew-toolchain.cmake
```

!!! warning

    Always pass the toolchain file. Without it, CMake may fail to discover
    `import std` support or the standard library module metadata.

## Build and test

For normal development use the hardened preset:

```sh
cmake --preset ci-hardened -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake
cmake --build --preset ci-hardened
ctest --preset ci-hardened
```

For benchmark builds use the release preset:

```sh
cmake --preset ci-release -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake
cmake --build --preset ci-release
```

On Linux, replace `cmake/llvm-brew-toolchain.cmake` with
`cmake/gcc-brew-toolchain.cmake`.

## First task

A libfork async function is a function object that returns `lf::task<T, Context>`.
The first argument is usually `lf::env<Context>`.

```cpp
import std;
import libfork;

struct answer {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>) -> lf::task<int, Context> {
    co_return 42;
  }
};

auto main() -> int {
  lf::mono_busy_pool<lf::geometric_stack<>> pool{2};
  int value = lf::schedule(pool, answer{}).get();
  return value == 42 ? 0 : 1;
}
```

`schedule` returns an `lf::receiver<T>`. Call `.get()` on the receiver to wait
for completion, consume the receiver, return the task result, or rethrow an
exception stored by the task.

## First fork-join scope

Use `lf::scope()` inside a task when it needs children:

```cpp
struct sum_pair {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int a, int b) -> lf::task<int, Context> {
    co_return a + b;
  }
};

struct parent {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>) -> lf::task<int, Context> {
    int left = 0;
    int right = 0;

    auto sc = co_await lf::scope();
    co_await sc.fork(&left, sum_pair{}, 1, 2);
    co_await sc.call(&right, sum_pair{}, 3, 4);
    co_await sc.join();

    co_return left + right;
  }
};
```

Use `fork` for work that may run in parallel with the continuation. Use `call`
when running the child inline is the better fit. Results written through return
addresses are safe to read only after the matching `join`.

## Consuming from another project

The current project is module-based. A consuming CMake project should link the
library target and compile with a compiler/toolchain that supports C++ module
file sets and `import std`.

```cmake
find_package(libfork CONFIG REQUIRED)

target_link_libraries(app PRIVATE libfork::libfork)
target_compile_features(app PRIVATE cxx_std_26)
```

When building libfork from this source tree, prefer the repository presets above.
