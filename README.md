
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

<h3 align="center"> **Now with 🌵**  </h1>

`libfork` is primarily an abstraction for strict [fork-join parallelism](https://en.wikipedia.org/wiki/Fork%E2%80%93join_model). This is made possible without the use of any macros/inline assembly using C++20's coroutines. Ultra-fine grained parallelism (the ability to spawn tasks with very low overhead) is enabled by an innovative implementation of a non-allocating [cactus-stack](https://en.wikipedia.org/wiki/Parent_pointer_tree) that utilizes _stack-stealing_.

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

`libfork` presents a cross-platform API that decouples scheduling tasks (a customization point) from writing tasks and expressing their dependencies. Additionally, `libfork` provides performant NUMA-aware work-stealing schedulers for general use. If you like to learn more check out [the tour of `libfork`](#a-tour-of-libfork) below.

## Benchmarks

See the [benchmark's README](benchmark/README.md) for a comparison of `libfork` to openMP and Intel's TBB, as well as some ARM/weak-memory-model benchmarks.

## Building and installing

See the [BUILDING](BUILDING.md) document for full details on compilation, installation and optional dependencies.

<h4 align="center"> ⚠️ COMPILER NIGGLES ⚠️ </h4>

Some very knew C++ features are used in `libfork`, most compilers have buggy implementations of coroutines, we do our best to work around known bugs:

- __gcc__ `libfork` is tested on versions 11.x-13.x however gcc [does not perform a guaranteed tail call](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100897) for coroutine's symmetric transfer unless compiling with optimization greater than or equal to `-O2` and sanitizers are not in use. This will result in stack overflows for some programs in un-optimized builds.

- __clang__ `libfork` compiles on versions 15.x-17.x however for versions 16.x and below bugs [#63022](https://github.com/llvm/llvm-project/issues/63022) and [#47179](https://github.com/llvm/llvm-project/issues/47179) will cause crashes for optimized builds in multithreaded programs. We work around this in these versions by isolating access to `thread_local` storage in non-inlined functions however, this introduces a performance penalty.

- __Apple's clang__ `libfork` is compatible with the standard library that Apple ships with Xcode 2015 however Xcode 15 itself segfaults when compiling `libfork` and Xcode 14 is not supported.

- __msvc__ `libfork` compiles on versions 19.35-19.37 however due to [this bug](https://developercommunity.visualstudio.com/t/Incorrect-code-generation-for-symmetric/1659260?scope=follow) (duplicate [here](https://developercommunity.visualstudio.com/t/Using-symmetric-transfer-and-coroutine_h/10251975?scope=follow&q=symmetric)) it will always seg-fault due to an erroneous double delete.

## Contributing

1. See the [CONTRIBUTING](CONTRIBUTING.md) document.
2. Have a snoop around the `impl` namespace.
3. Ask as many questions as you can think of!

## API reference/documentation

See the generated [docs](https://conorwilliams.github.io/libfork/).

## Changelog

See the [ChangeLog](ChangeLog.md) document.

## A tour of `libfork`

### Contents

- [Fork-join introduction](#fork-join-introduction)
  - [The cactus stack](#the-cactus-stack)
- [Non default-constructible types and `eventually<T>`](#eventually)
- [Exception in `libfork`](#exceptions)
- [Scheduling and contexts](#schedulers)
  - [Explicit scheduling]()
- [Algorithms and the high-level API]()

This section provides some background and highlights the `core` API, for details on implementing your own schedulers on-top of `libfork` see the [extension documentation]().

### Fork-join introduction

The tasking/fork-join interface is designed to mirror [Cilk](https://en.wikipedia.org/wiki/Cilk) and other fork-join frameworks. With `libfork` we have already seen that the canonical recursive-Fibonacci is a simple as:

```c++
1.  #include "libfork/core.hpp"
2.
3.  inline constexpr async fib = [](auto fib, int n) -> lf::task<int> { 
4.  
5.    if (n < 2) {
6.      co_return n;
7.    }
8.
9.    int a, b
10.
11.   co_await lf::fork[a, fib](n - 1);    
12.   co_await lf::call[b, fib](n - 2);  
13.
14.   co_await lf::join;                  
15.
16.   co_return a + b;                    
17. };
```

If your compiler does not support `fork[a, fib]` syntax then you can use `fork(a, fib)`. This can be launched on the ``lazy_pool`` scheduler as follows:

```c++

#include "libfork/schedule.hpp"

int main(){

  lf::lazy_pool pool(num_threads);

  int fib_10 = lf::sync_wait(pool, fib, 10);

}
```

The call to `sync_wait` will block the main thread until the pool has completed execution of the task.

Lets break down what is going on line by line:

__Line 3:__ Here we define an async function, an async function must have a templated first argument. This is used by the library to pass context from parent to child, additionally it acts as a y-combinator allowing the lambda to be recursive. An async function (an instance of `lf::async`) can be constructed from any non-capturing lambda that returns an `lf::task<T>` where `T` specifies the type of the return value.

__Line 9:__ Here we construct the variables that will be bound to the return values of following forks/calls.

__Line 11:__ This is the first call to `lf::fork` which marks the beginning of an _async scope_. `lf::fork[a, fib]` binds the the return address of the function `fib` to the integer `a` this bound function is then invoked with the argument `n - 1`. The semantics of all of this is: the execution of the forked function (in this case `fib`) can continue concurrently with the execution of the next line of code i.e. the _continuation_. As `libfork` is a continuation stealing library the worker/thread that performed the fork will immediately begin executing the forked function while another thread may _steal_ the continuation.

__Line 12:__ An `lf::call` binds arguments and return address in the same way as `lf::fork` however it has the semantics of a serial function call. This is done instead of a `fork` as there is no further work to do in the current task so stealing it would be a waste of resources.

__Line 13:__ Execution cannot continue past a join until all child tasks have completed. After this point it is safe to access the results (`a` and `b`) of the child task. Only a single worker will continue execution after the join. This marks the end of the async scope that began at the `fork`.

__NOTE:__ `libfork` implements _strict_ fork-join which means:

- Tasks can only wait on their children.
- All children __must__ be joined __before__ a task can return.

Furthermore, observe that at every ``co_await`` the thread executing the task may change!

### The cactus-stack

Normally each call to a coroutine would allocate on the heap. However, `libfork` implements a cactus-stack supported by stack-staling which allows each coroutine to be allocated on a fragment of linear stack, this has the same overhead as allocating on the real stack.

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
