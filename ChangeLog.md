# Changelog

<!-- ## [**Version x.x.x**](https://github.com/ConorWilliams/libfork/compare/v2.1.0...dev)

### Added

### Changed

### Removed

### Bugfixes

### Meta  -->

## [**Version 3.x.0**](https://github.com/ConorWilliams/libfork/compare/v3.6.0...v3.x.0)

### Added

- New static asserts for better error messages when passing or capturing the first argument incorrectly.
- New exception types.
- `schedule` now generalizes `sync_wait`.
- The `future` class template.
- The `detach` function as an alternative to `sync_wait`.
- More exception handling tests.
- New optional (macro based) mechanism for forward declaring async-functions to speed up compilation.

### Changed

- Made `task` a RAII type.
- Exposed the `referenceable` concept.
- Exposed the `storable` concept.
- Quasi-pointer and async function object concepts generalized to reference types to reduce the number of moves.
- `unit_pool` now uses a separate thread for running tasks instead of running them inline.
- `sync_wait` now throws an exception if called from a worker thread.
- `sync_wait` is now exception safe.

### Removed

- Outdated tests.
- The `algorithm.hpp` meta-header (this was encouraging users to include all the algorithms).

### Bugfixes

- Fixed `#pragma unroll` on GCC.
- Made `lazy_pool` and `busy_pool` movable.

### Meta

- Fixed-up some extended warnings.

## [**Version 3.6.0**](https://github.com/ConorWilliams/libfork/compare/v3.5.0...v3.6.0)

### Added

- A new ``lf::dispatch`` higher order function object for fine grained exception control.
- A new call `lf::just` to simplify `co_await lf::call[...](...); co_await lf::join;`.
- A set of constrained algorithms `for_each`, `fold`, `scan`, etc.
- `eventually` supports capturing exceptions via the `stash_exception` customization point.
- Generalized the explicit scheduling mechanism.
- A few new benchmarks for the new constrained algorithms.

### Changed

- Improved memory safety of awaitables and made many type immovable to reduce erroneous use.
- Improved the API for stack allocation.
- The underlying types of `submit_handle` and `task_handle`.
- Re-introduced exception handling and added a customization point for individual exception handling.

### Bugfixes

- Fixed a few bugs that could occur if an exception was thrown during a `co_await` expression.

### Meta

- Package tests for conan/vcpkg/manual installs and some package bugfixes.

## [**Versions 3.5.0**](https://github.com/ConorWilliams/libfork/compare/v2.1.1...v3.5.0)

This release is a full overhaul of the API and implementation of libfork that should bring large performance improvements.

### Added

- NUMA support for all schedulers (using `hwloc` for topology discovery).
- A segmented-stack backed cactus-stack to manage all coroutine allocations.
- A new ``lf::lazy_pool`` scheduler.
- A new concepts-centric design including concepts for invocability.
- A new API for explicit scheduling.
- The `lf::eventually` class template.
- New benchmarks.
- Exposed the `lf::defer` class template for handling cleanup.

### Changed

- A new API for defining tasks.
- A new API for forking/calling/joining tasks.
- A new API for defining schedulers.

### Removed

- Old benchmarks that are no longer relevant.
- Allocator support -

### Meta

- Split CI into jobs for each major platform.
- Use vcpkg for package management.

## **Versions 3.0.0-3.4.0**

No releases were made for libfork during the initial stages of the 3.x.x series as the API was in a state of flux.

## [**Version 2.1.1**](https://github.com/ConorWilliams/libfork/compare/v2.1.0...v2.1.1)

### Added

- Conan support

### Bugfixes

- CMake install interface now uses correct include path
- CMake config module exports its dependencies correctly

## [**Version 2.1.0**](https://github.com/ConorWilliams/libfork/compare/v2.0.0...v2.1.0)

This release primarily backports ``jthread`` to ``thread`` for Apple clang and libc++.

### Added

- Support for libc++ and Apple clang.

### Changed

- Conditional ``<syncstream>`` support.

### Meta

- CI builds on Apple Silicon.

## [**Version 2.0.0**](https://github.com/ConorWilliams/libfork/compare/v1.0.0...v2.0.0)

This release overhauled libfork bringing many changes including:

### Added

- The ``busy_pool`` scheduler.
- Custom schedulers supported.
- Tasks support allocators.
- Benchmarks!!
- Many more tests.

### Changed

- Decoupling scheduling from task graph creation.
- Void tasks no longer require a future.

### Removed

- Exception handling (tasks now call ``terminate()`` if they exit with an unhandled exception).
- The global ``forkpool`` thread pool. This functionality will be reintroduced in a future release but in the form of a customizable scheduler.

### Meta

- New dependencies for tests/benchmarks and a new CMake project structure.

## Version 1.0.0

The original version of libfork (then forkpool), it used a global thread pool for all task scheduling.
