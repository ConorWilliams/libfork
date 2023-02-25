


# Welcome to libfork (🍴) [![Continuous Integration](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml/badge.svg)](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml) [![codecov](https://codecov.io/gh/ConorWilliams/libfork/branch/main/graph/badge.svg?token=89MTSXI85F)](https://codecov.io/gh/ConorWilliams/libfork)

Libfork is primarily an abstraction for strict, lock-free, wait-free, continuation-stealing [fork-join parallelism](https://en.wikipedia.org/wiki/Fork%E2%80%93join_model). This is made possible without the use of any macros/inline assembly using C++20's coroutines. 

Libfork presents an API that decouples scheduling tasks (a customization point) from writing tasks and expressing their dependencies. Additionally, libfork provides performant work-stealing schedulers for general use.


## Building and installing

See the [BUILDING](BUILDING.md) document.

## Benchmarks

See the benchmark's [README](benchmark/README.md).

## Contributing

See the [HACKING](HACKING.md) document.

## Tasks and futures

The tasking fork-join interface is designed to mirror Cilk and other fork-join frameworks. With libfork the canonical recursive-Fibonacci is a simple as:

```c++
#include "libfork/task.hpp"
#include "libfork/schedule/busy_pool.hpp"

/// Compute the n'th fibonacci number
auto fib(int n) -> basic_task<int, lf::busy_pool::context> { 

  if (n < 2) {
    co_return n;
  }

  auto a = co_await fib(n - 1).fork(); // Spawn a child task.
  auto b = co_await fib(n - 2);        // Execute inline.

  co_await lf::join();                 // Wait for children.

  co_return *a + b;                    // Use * to dereference a future.
}
```
which can be launched on a scheduler of your choice (such as libfork's ``busy_pool``):
```c++
lf::busy_pool pool(num_threads);

int fib_10 = pool.schedule(fib(10));
```
Above ``lf::busy_pool`` is a scheduler, see below for a description.

Note:
- Tasks **must** join **before** returning or dereferencing a future.
- Futures must join in their parent.

## Schedulers

A scheduler is responsible for distributing available work (tasks) to physical CPUs/executors. Each executor must own an execution-context object satisfying:
```c++
template <typename T>
concept context = requires(T context, work_handle<T> task) {
    { context.push(task) } -> std::same_as<void>;
    { context.pop() } -> std::convertible_to<std::optional<work_handle<T>>>;
};
```
An execution-context models a FILO stack. Tasks hold a pointer to their executor's context and push/pop tasks onto it. Whilst an executor is running a task, other executors can steal from the top of the stack in a FIFO manner. 

## API reference

See the [API documentation](https://conorwilliams.github.io/libfork/) document.

## Reference

This project implements many of the ideas from (available in [`reference/`](reference)):

```bibtex
@inproceedings{Schmaus2021,
  title     = {Nowa: A Wait-Free Continuation-Stealing Concurrency Platform},
  author    = {Florian Schmaus and Nicolas Pfeiffer and Timo Honig and Jorg Nolte and Wolfgang Schroder-Preikschat},
  year      = 2021,
  month     = may,
  booktitle = {2021 {IEEE} International Parallel and Distributed Processing Symposium ({IPDPS})},
  publisher = {{IEEE}},
  doi       = {10.1109/ipdps49936.2021.00044},
  url       = {https://doi.org/10.1109/ipdps49936.2021.00044}
}
```

```bibtex
@inproceedings{Lin2020,
  title     = {An Efficient Work-Stealing Scheduler for Task Dependency Graph},
  author    = {Chun-Xun Lin and Tsung-Wei Huang and Martin D. F. Wong},
  year      = 2020,
  month     = dec,
  booktitle = {2020 {IEEE} 26th International Conference on Parallel and Distributed Systems ({ICPADS})},
  publisher = {{IEEE}},
  doi       = {10.1109/icpads51040.2020.00018},
  url       = {https://doi.org/10.1109/icpads51040.2020.00018}
}
```

```bibtex
@inproceedings{L2013,
  title     = {Correct and efficient work-stealing for weak memory models},
  author    = {Nhat Minh L{\^{e}} and Antoniu Pop and Albert Cohen and Francesco Zappa Nardelli},
  year      = 2013,
  month     = feb,
  booktitle = {Proceedings of the 18th {ACM} {SIGPLAN} symposium on Principles and practice of parallel programming},
  publisher = {{ACM}},
  doi       = {10.1145/2442516.2442524},
  url       = {https://doi.org/10.1145/2442516.2442524}
}
```

```bibtex
@inproceedings{Chase2005,
  title     = {Dynamic circular work-stealing deque},
  author    = {David Chase and Yossi Lev},
  year      = 2005,
  month     = jul,
  booktitle = {Proceedings of the seventeenth annual {ACM} symposium on Parallelism in algorithms and architectures},
  publisher = {{ACM}},
  doi       = {10.1145/1073970.1073974},
  url       = {https://doi.org/10.1145/1073970.1073974}
}
```

