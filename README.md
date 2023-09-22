



<h1 align="center"> Welcome to <tt>libfork</tt> 🍴 </h1>

<p align="center">
<a href="https://github.com/ConorWilliams/libfork/actions/workflows/linux.yml">
    <img src="https://github.com/ConorWilliams/libfork/actions/workflows/linux.yml/badge.svg">
</a>
<a href="https://github.com/ConorWilliams/libfork/actions/workflows/windows.yml">
    <img src="https://github.com/ConorWilliams/libfork/actions/workflows/windows.yml/badge.svg">
</a>
<a href="https://github.com/ConorWilliams/libfork/actions/workflows/macos.yml">
    <img src="https://github.com/ConorWilliams/libfork/actions/workflows/macos.yml/badge.svg">
</a>
<a href="https://github.com/ConorWilliams/openFLY/actions/workflows/pages/pages-build-deployment">
    <img src="https://github.com/ConorWilliams/openFLY/actions/workflows/pages/pages-build-deployment/badge.svg">
</a>
<a href="https://codecov.io/gh/ConorWilliams/libfork">
    <img src="https://codecov.io/gh/ConorWilliams/libfork/branch/main/graph/badge.svg?token=89MTSXI85F)">
</a>
</p>

<p align="center">
  A bleeding edge, lock-free, wait-free, continuation-stealing coroutine-tasking library.
</p>

<h3 align="center"> ** Now with 🌵 **  </h1>

`libfork` is primarily an abstraction for strict [fork-join parallelism](https://en.wikipedia.org/wiki/Fork%E2%80%93join_model). This is made possible without the use of any macros/inline assembly using C++20's coroutines. Ultra-fine grained parallelism (the ability to spawn tasks with near zero overhead) is enabled by an innovative implementation of a non-allocating [cactus-stack](https://en.wikipedia.org/wiki/Parent_pointer_tree) that utilizes _stack-stealing_.

__TLDR:__
```c++

inline constexpr async fib = [](auto fib, int n) -> lf::task<int> { 
  
  if (n < 2) {
    co_return n;
  }

  int a, b

  co_await lf::fork[a, fib](n - 1);    // Spawn a child task.
  co_await lf::call[b, fib](n - 2);    // Execute inline.

  co_await lf::join;                   // Wait for children.

  co_return a + b;                     // Safe to access after join.
};
```

`libfork` presents a cross-platform API that decouples scheduling tasks (a customization point) from writing tasks and expressing their dependencies. Additionally, `libfork` provides performant NUMA-aware work-stealing schedulers for general use. 




## Benchmarks

See the [benchmark's README](benchmark/README.md) for a comparison of `libfork` to openMP and Intel's TBB, as well as some ARM/weak-memory-model benchmarks. 

## Building and installing

See the [BUILDING](BUILDING.md) document for full details on compilation, installation and optional dependencies.

<h4 align="center"> ⚠️ COMPILER NIGGLES ⚠️ </h4>

- __gcc__ `libfork` is tested on versions 11.x-13.x however gcc [does not perform a guaranteed tail call](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100897) for coroutine's symmetric transfer unless compiling with optimization greater than or equal to `-O2` and sanitizers are not in use. This will result in stack overflows for some programs in un-optimized builds. 

- __clang__ `libfork` compiles on versions 15.x-17.x however for versions 16.x and below bugs [#63022](https://github.com/llvm/llvm-project/issues/63022) and [#47179](https://github.com/llvm/llvm-project/issues/47179) will cause crashes for optimized builds in multithreaded programs. We work around this in these versions by isolating access to `thread_local` storage in non-inlined functions however, this introduces around a 25% performance penalty vs GCC.

- __msvc__ `libfork` compiles on versions 19.35-19.37 however due to [this bug](https://developercommunity.visualstudio.com/t/Incorrect-code-generation-for-symmetric/1659260?scope=follow) (duplicate [here](https://developercommunity.visualstudio.com/t/Using-symmetric-transfer-and-coroutine_h/10251975?scope=follow&q=symmetric)) it will always seg-fault due to an erroneous double delete.

## Contributing

1. See the [CONTRIBUTING](CONTRIBUTING.md) document.
2. Have a snoop around the [`impl` api]().
3. Ask as many questions as you can think of!

## API reference/documentation

See the generated [docs](https://conorwilliams.github.io/libfork/).

## Changelog

See the [ChangeLog](ChangeLog.md) document.

## A tour of `libfork`

<!-- This section provides some background and highlights the [consumer api](), for details on implementing your own schedulers on top of `libfork` see the [context api](). -->

#### Contents:

- [Fork-join introduction](#Tasks-and-futures)
   - [The cactus stack]()
   - [What is that first argument? (y-combinators)](#y-combinator)
- [Core API:](#Features-of-the-consumer-API)
  - [Non default-constructible types and `eventually<T>`](#eventually)
  - [Exotic calls with `invoke`, `tail` and `ignore`](#Invoke-and-tail)
- [Asynchronous stack-tracing]()
- [Exception in `libfork`](#exceptions)
- [Scheduling and contexts](#Schedulers)
- [Algorithms and the high-level API]()

### Tasks and futures

The tasking fork-join interface is designed to mirror Cilk and other fork-join frameworks. With libfork the canonical recursive-Fibonacci is a simple as:

```c++
#include "libfork/libfork.hpp"
#include "libfork/schedule/busy.hpp"

/// Compute the n'th fibonacci number
inline constexpr auto fib = lf::async([](auto fib, int n) -> lf::task<int> { 

  if (n < 2) {
    co_return n;
  }

  int a, b

  co_await lf::fork[a, fib](n - 1);    // Spawn a child task.
  co_await lf::call[b, fib](n - 2);    // Execute inline.

  co_await lf::join;                   // Wait for children.

  co_return a + b;                     // Safe to access after join
});
```
which can be launched on the ``busy_pool`` scheduler as follows:
```c++
lf::busy thread_pool(num_threads);

int fib_10 = lf::wait(thread_pool, fib, 10);
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

It is recommended that custom schedulers use lock-free stacks for their execution contexts such as the one provided in [libfork](include/libfork/deque.hpp)

## Reference

This project builds on many of the ideas (available in [`reference/`](reference)) developed by the following papers:

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

