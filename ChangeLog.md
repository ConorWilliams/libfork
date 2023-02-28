# Changelog

<!-- ## Unreleased

### Added

### Changed

### Removed

### Bugfixes

### Meta  -->

## Unreleased

### Added

### Changed

### Removed

### Bugfixes

### Meta 

## Version 2.0.0

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
- The global ``forkpool`` thread pool. This functionality will be reintroduced in a future release but in the form of a customisable scheduler.

### Meta 

- New dependencies for tests/benchmarks and a new CMake project structure.

## Version 1.0.0

The original version of libfork (then forkpool), it used a global thread pool for all task scheduling.
