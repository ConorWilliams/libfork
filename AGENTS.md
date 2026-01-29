# Libfork Copilot Instructions

## Project Overview

**libfork** is a continuation-stealing coroutine-tasking library implementing
strict fork-join parallelism using C++20 coroutines.

- **Type**: C++ library with module/`import std` support
- **Languages**: C++26

## Critical Build Requirements

### Compiler & Module Support

This project **requires C++23 `import std`** and **MUST** use the appropriate
toolchain file:

- **MacOS**: Use `-DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake`
- **Linux**: Use `-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-brew-toolchain.cmake`

**Common Error**: Without the toolchain file, CMake will fail.

**Always include the toolchain file** in configure commands.

### Dependencies (Homebrew)

Make sure Homebrew is installed and `brew` is in your `PATH`:

```bash
brew --version
```

**Required for building/testing:**

- `cmake`
- `ninja`
- `catch2`
- `google-benchmark`
- `clang-format`
- `codespell`

If on MacOS, also require:

- `llvm`

If on Linux, also require:

- `gcc`
- `binutils`

Install all at once (MacOS):

```bash
brew install cmake ninja catch2 google-benchmark clang-format codespell llvm
```

Install all at once (Linux):

```bash
brew install cmake ninja catch2 google-benchmark clang-format codespell gcc binutils
```

## Build & Test Workflow

### 1. Configure

Always use presets with the toolchain file:

```bash
cmake --preset <preset-name> -DCMAKE_TOOLCHAIN_FILE=cmake/<toolchain>.cmake
```

**Relevant available presets** (from `CMakePresets.json`):

- `ci-hardened` - Debug build with warnings and hardening flags
- `ci-release` - Optimized release build

All presets enable developer mode (`libfork_DEV_MODE=ON`) and use Ninja generator.

You should use the `ci-hardened` preset for development/testing and
`ci-release` for benchmarking.

### 2. Build

```bash
cmake --build --preset <preset-name>
```

**Build warnings** (expected and safe):

- "It is recommended to build benchmarks in Release mode" - only relevant for `ci-hardened`
- CMake experimental `import std;` warning - expected for C++23 modules

### 3. Test

```bash
ctest --preset <preset-name>
```

All tests should pass. If tests fail, check that:

- Configuration used the correct toolchain file
- Build completed without errors
- Any changes you have made are correct

## Linting & Validation

The CI runs two linting tools that you should run before committing:

### codespell (spelling)

```bash
codespell
```

Config: `.codespellrc` (ignores: build/, .git/, etc.)
Should produce no output if passing.

### clang-format (code formatting)

```bash
find src include test benchmark/src -name "*.cpp" -o -name "*.hpp" -o -name "*.cxx" | xargs clang-format --dry-run --Werror
```

Config: `.clang-format` (110 column limit, specific style)
Should produce no output if passing.

**To auto-fix formatting**:

```bash
find src include test benchmark/src -name "*.cpp" -o -name "*.hpp" -o -name "*.cxx" | xargs clang-format -i
```

## Project Structure

### Source Layout

```sh
libfork/
├── cmake/                    # CMake utilities
├── include/libfork/**/*.hpp  # Public headers
├── src/**/                   # Module and source files
│   ├── *.cxx                 # Module files
│   └── *.cpp                 # Source files
├── test/src/**/              # Test suite (Catch2)
│   └── *.cpp                 # Test source files
├── benchmark/src/            # Benchmarking suite (google-benchmark)
│   └── libfork_benchmark/    # Merged source/header files for benchmarks
│          └── fib/           # Each benchmark in its own sub-directory
│              ├── *.hpp      # Benchmark header files
│              └── *.cpp      # Benchmark source files
├── .github/workflows/        # CI workflows
│   ├── linux.yml             # Linux builds
│   ├── macos.yml             # MacOS builds
│   ├── lint.yml              # Linting
│   └── linear.yml            # Enforces linear history (no merge commits)
└── CMakeLists.txt            # Main build configuration
```

## Workflows

### Workflow Command Pattern

All workflows follow this pattern:

```yaml
- Install Dependencies: brew install ...
- Configure: cmake --preset <preset> -DCMAKE_TOOLCHAIN_FILE=<toolchain>.cmake
- Build: cmake --build --preset <preset> 
- Test: ctest --preset <preset>
```

## Common Development Tasks

### Making Code Changes

1. **Modify source files** in `src/`, `include/`, `test/`, or `benchmark/`
2. **Rebuild**: `cmake --build --preset <your-preset>`
3. **Test**: `ctest --preset <your-preset>`
4. **Lint**: Run codespell and clang-format checks

#### Adding/removing files from `src/` or `include/`

- Update the root `CMakeLists.txt` with new/removed files.

#### Adding/removing files from `benchmark/src/`

- Update `benchmark/CMakeLists.txt` with new/removed files.

### Adding Tests

Strive to add tests for new features/bug fixes.

- Add `.cpp` files to `test/src/`
- Tests auto-discovered by CMake (GLOB_RECURSE)
- Links against `libfork::libfork` and `Catch2::Catch2WithMain`

### Modifying Build Configuration

**Warning**: Module-related changes are complex. Test thoroughly with clean builds.

## Troubleshooting

### Build Failures

**Problem**: Configuration/Build fails after adding/removing files or modifying CMakeLists.txt
**Solution**: Try a clean build directory:

```bash
rm -rf build/
```

**Problem**: "compiler does not provide a way to discover the import graph"
**Solution**: Add `-DCMAKE_TOOLCHAIN_FILE=cmake/llvm-brew-toolchain.cmake` to configure

**Problem**: "Could not find 'brew' executable"  
**Solution**: Install Homebrew

**Problem**: "Could not automatically find libc++.modules.json"
**Solution**: Ensure LLVM is installed via Homebrew; toolchain auto-detects the path

### Linting Failures

**Problem**: clang-format errors
**Solution**: Run fix command above to auto-format code

**Problem**: codespell errors  
**Solution**: Fix typos or add to ignore list in `.codespellrc` if false positive
