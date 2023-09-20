# Changelog

<!-- ## [**Version x.x.x**](https://github.com/ConorWilliams/libfork/compare/v2.1.0...dev)

### Added

### Changed

### Removed

### Bugfixes

### Meta  -->

`HWLOC_COMPONENTS=-opencl,-gl hwloc-bind pu:0 pu:2 pu:4 pu:6 pu:8 pu:10 pu:12 pu:14 pu:16 pu:18 pu:20 pu:22 pu:24 pu:26 pu:28 pu:30 -v ./build/dev/benchmark/overhead/overhead.out`

| relative |               ns/op |                op/s |    err% | Fibonacci |      ----------   | relative |               ns/op |                op/s |    err% | Fibonacci
|---------:|--------------------:|--------------------:|--------:|-----------: |---|---------:|--------------------:|--------------------:|--------:|:----------
|   100.0% |       73,174,822.00 |               13.67 |    0.5% | `old 1` |   |   100.0% |       44,535,653.58 |               22.45 |    0.1% | `new 1`
|   196.9% |       37,167,994.00 |               26.90 |    0.1% | `old 2` |   |   197.4% |       22,563,473.42 |               44.32 |    0.9% | `new 2`
|   293.1% |       24,964,143.00 |               40.06 |    0.3% | `old 3` |   |   296.3% |       15,028,852.09 |               66.54 |    0.5% | `new 3`
|   379.5% |       19,280,293.00 |               51.87 |    0.3% | `old 4` |   |   390.7% |       11,397,491.00 |               87.74 |    1.3% | `new 4`
|   489.5% |       14,948,657.00 |               66.90 |    0.1% | `old 5` |   |   484.8% |        9,186,724.92 |              108.85 |    0.9% | `new 5`
|   568.0% |       12,883,695.00 |               77.62 |    1.3% | `old 6` |   |   522.4% |        8,525,230.45 |              117.30 |    2.4% | `new 6`
|   666.8% |       10,973,722.00 |               91.13 |    0.2% | `old 7` |   |   616.9% |        7,219,750.58 |              138.51 |    1.0% | `new 7`
|   727.3% |       10,060,978.00 |               99.39 |    0.3% | `old 8` |   |   699.0% |        6,371,316.30 |              156.95 |    1.0% | `new 8`
|   812.4% |        9,007,553.00 |              111.02 |    1.7% | `old 9` |   |   797.6% |        5,583,710.55 |              179.09 |    0.4% | `new 9`
|   912.5% |        8,018,762.00 |              124.71 |    0.8% | `old 10` |   |   850.3% |        5,237,685.55 |              190.92 |    0.4% | `new 10`
|   988.3% |        7,404,393.00 |              135.05 |    0.6% | `old 11` |   |   940.3% |        4,736,095.00 |              211.14 |    0.0% | `new 11`
| 1,085.7% |        6,739,737.00 |              148.37 |    0.6% | `old 12` |   | 1,018.8% |        4,371,551.60 |              228.75 |    0.2% | `new 12`
| 1,095.0% |        6,682,528.00 |              149.64 |    0.4% | `old 13` |   | 1,059.3% |        4,204,303.09 |              237.85 |    0.1% | `new 13`
| 1,141.5% |        6,410,180.00 |              156.00 |    0.2% | `old 14` |   | 1,172.3% |        3,799,051.70 |              263.22 |    0.1% | `new 14`
| 1,199.3% |        6,101,642.00 |              163.89 |    0.3% | `old 15` |   | 1,250.1% |        3,562,497.17 |              280.70 |    0.1% | `new 15`
| 1,269.2% |        5,765,228.00 |              173.45 |    0.3% | `old 16` |   | 1,313.2% |        3,391,400.42 |              294.86 |    0.5% | `new 16`



## [**Version x.x.x**](https://github.com/ConorWilliams/libfork/compare/v2.1.0...dev)

### Added

### Changed

### Removed

### Bugfixes

### Meta 

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
