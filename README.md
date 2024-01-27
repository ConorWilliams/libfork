


# Welcome to libfork 🍴 [![Continuous Integration](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml/badge.svg)](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml) [![codecov](https://codecov.io/gh/ConorWilliams/libfork/branch/main/graph/badge.svg?token=89MTSXI85F)](https://codecov.io/gh/ConorWilliams/libfork)

Libfork is primarily an abstraction for strict, lock-free, wait-free, continuation-stealing [fork-join parallelism](https://en.wikipedia.org/wiki/Fork%E2%80%93join_model). This is made possible without the use of any macros/inline assembly using C++20's coroutines.

Libfork presents a cross-platform API that decouples scheduling tasks (a customization point) from writing tasks and expressing their dependencies. Additionally, libfork provides performant work-stealing schedulers for general use. 

## Benchmarks

See the [benchmark's README](benchmark/README.md) for a comparison of libfork to openMP and Intel's TBB, as well as some ARM/weak-memory-model benchmarks.

## Consuming the library

This package is available via conancenter. Add the following line to your conanfile.txt

```ini
[requires]
libfork/2.1.1
```
and install it to conan2 cache:

```sh
conan install .. --build missing
```

(Plase make sure that you use a c++20 ready conan profile!)

You may then use the library in your project's cmake:

```cmake
find_package(libfork REQUIRED)  
target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```


## Building and installing

See the [BUILDING](BUILDING.md) document for full details. 

Note, libfork is tested on GCC (10,11,12) and Clang (14,15), currently Clang seems to do a much better job at optimizing coroutines. When Microsoft fixes [this bug](https://developercommunity.visualstudio.com/t/Incorrect-code-generation-for-symmetric/1659260?scope=follow) libfork should build on MSVC.

## Contributing

See the [HACKING](HACKING.md) document.

## API reference

See the [API documentation](https://conorwilliams.github.io/libfork/) website.

## Changelog

See the [ChangeLog](ChangeLog.md) document.

## Tasks and futures

The tasking fork-join interface is designed to mirror Cilk and other fork-join frameworks. With libfork the canonical recursive-Fibonacci is a simple as:

```c++
#include "libfork/task.hpp"
#include "libfork/schedule/busy_pool.hpp"

/// Short-hand for a task that uses the busy_pool scheduler.
template <typename T>
using pool_task = lf::basic_task<T, lf::busy_pool::context>;

/// Compute the n'th fibonacci number
auto fib(int n) -> pool_task<int> { 

  if (n < 2) {
    co_return n;
  }

  auto a = co_await fib(n - 1).fork(); // Spawn a child task.
  auto b = co_await fib(n - 2);        // Execute inline.

  co_await lf::join();                 // Wait for children.

  co_return *a + b;                    // Use * to dereference a future.
}
```
which can be launched on the ``busy_pool`` scheduler as follows:
```c++
busy_pool pool(num_threads);

int fib_10 = pool.schedule(fib(10));
```
Note:
- Tasks **must** join **before** returning or dereferencing a future.
- Futures **must** join in their parent task.

The mental execution model here is: 
1. At ``auto a = co_await fib(n - 1).fork()`` the parent/current task is suspended and a child task is spawned, ``a`` is of type ``basic_future<int, busy_pool::context>``. 
2. The current thread pushes the parent task's continuation onto its execution-context and starts executing the child task; the scheduler is free to hand the parent task's continuation to another thread.
3. At ``auto b = co_await fib(n - 2)`` a new child task is spawned and the current thread immediately starts executing it. The continuation is not pushed onto the execution-context. This is preferred to ``.fork()`` as a thieving thread would have no work to do before the ``join()``.
4. At ``co_await lf::join()``, if the children have not completed, the parent task is suspended.
5. The parent task will be resumed by the thread which completes the final child task.

Hence, at every ``co_await`` the thread executing the parent task may change! This is a diamond-shaped dependency graph, where the parent task is the diamond's tip and the children are the diamond's legs. The diamond's base is the continuation following the ``join()``. In this case the children may be recursively more complex sub-graphs. Crucially, the task graph encodes to the scheduler that: the children may be executed in parallel; the children must complete before the parent task can continue past the ``join()``.

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

It is recommended that custom schedulers use lock-free stacks for their execution contexts such as the one provided in [libfork](include/libfork/queue.hpp)

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

