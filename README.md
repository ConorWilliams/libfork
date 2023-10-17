
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
inline constexpr lf::async fib = [](auto fib, int n) -> lf::task<int> { 
  
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

See the [benchmark's README](bench/README.md) for a comparison of `libfork` to openMP and Intel's TBB, as well as some ARM/weak-memory-model benchmarks.

## Building and installing

See the [BUILDING](BUILDING.md) document for full details on compilation, installation and optional dependencies.

<h4 align="center"> ⚠️ COMPILER NIGGLES ⚠️ </h4>

Some very new C++ features are used in `libfork`, most compilers have buggy implementations of coroutines, we do our best to work around known bugs:

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

This section provides some background and highlights the `core` API, for details on implementing your own schedulers on-top of `libfork` see the [extension documentation](https://conorwilliams.github.io/libfork/).

### Contents

- [Fork-join introduction](#fork-join-introduction)
- [The cactus stack](#the-cactus-stack)
- [Details on the first argument](#details-on-the-first-argument)
- [Restrictions on references](#restrictions-on-references)
- [Directly invoking async functions](#directly-invoking-async-functions)
- [Exception in `libfork`](#exceptions)
- [Delaying construction with `lf::eventually<T>`](#delaying-construction)
- [Contexts and schedulers](#schedulers)
- [Explicit scheduling]()

### Fork-join introduction

The tasking/fork-join interface is designed to mirror [Cilk](https://en.wikipedia.org/wiki/Cilk) and other fork-join frameworks. With `libfork` we have already seen that the canonical recursive-Fibonacci is a simple as:

```c++
 1| #include "libfork/core.hpp"
 2|
 3| inline constexpr lf::async fib = [](auto fib, int n) -> lf::task<int> { 
 4|  
 5|   if (n < 2) {
 6|     co_return n;
 7|   }
 8|
 9|   int a, b
10|
11|   co_await lf::fork[a, fib](n - 1);    
12|   co_await lf::call[b, fib](n - 2);  
13|
14|   co_await lf::join;                  
15|
16|   co_return a + b;                    
17| };
```

If your compiler does not support `lf::fork[a, fib]` syntax then you can use `lf::fork(a, fib)`. This can be launched on the ``lazy_pool`` scheduler as follows:

```c++
1| #include "libfork/schedule.hpp"
2|
2| int main() {
3|
4|   lf::lazy_pool pool(num_threads);
5|
6|   int fib_10 = lf::sync_wait(pool, fib, 10);
7|
8| }
```

The call to `sync_wait` will block the _main_ (i.e. the thread that calls `main()`) thread until the pool has completed execution of the task.

__NOTE:__ `libfork` implements _strict_ fork-join which means:

- Tasks can only wait on their children.
- All children __must__ be joined __before__ a task returns.

These restrictions give some nice mathematical properties to the underlying directed acyclic graph (DAG) of tasks that enables many optimizations.

#### Step by step

Lets break down what is going on line by line:

- __Line 3:__ First we define an async function (an instance of `lf::async`). An async function must have a templated first argument, this is used by the library to pass static and dynamic context from parent to child (like the type of the underlying scheduler), additionally it acts as a [y-combinator](https://en.wikipedia.org/wiki/Fixed-point_combinator) allowing the lambda to be recursive. An async function can be constructed from any stateless callable (like a non-capturing lambda) that returns an `lf::task<T>` where `T` specifies the type of the return value.
- __Line 9:__ Next we construct the variables that will be bound to the return values of following forks/calls.
- __Line 11:__ This is the first call to `lf::fork` which marks the beginning of an _async scope_. `lf::fork[a, fib]` binds the the return address of the function `fib` to the integer `a`, Internally the child coroutine will store a pointer to return variable variable. The bound function is then invoked with the argument `n - 1`. The semantics of all of this is: the execution of the forked function (in this case `fib`) can continue concurrently with the execution of the next line of code i.e. the _continuation_. As `libfork` is a continuation stealing library the worker/thread that performed the fork will immediately begin executing the forked function while another thread may _steal_ the continuation.
- __Line 12:__ An `lf::call` binds arguments and return address in the same way as `lf::fork` however, it has the semantics of a serial function call. This is done instead of an `lf::fork` as there is no further work to do in the current task so stealing it would be a waste of resources.
- __Line 13:__ Execution cannot continue past a join until all child tasks have completed. After this point it is safe to access the results (`a` and `b`) of the child task. Only a single worker will continue execution after the join. This marks the end of the async scope that began at the `fork`.
- __Line 16:__ Finally we return the result of the to a parent task, this has similar semantics to a regular return however, behind the scenes an assignment of the return value to the parent's return address is performed. This is the end of the async function. The worker will attempt to resume the parent task (if it has not already been stolen) just as a regular function would resume execution of the caller.

__Note:__ that at every ``co_await`` the OS-thread executing the task may change!

#### Ignoring a result

If you wanted to ignore the result of a fork/call (i.e. if you wanted the side effect only) you can simply omit return address from lines 11 and 12 e.g.:

```c++
co_await lf::fork[fib](n - 1);    
co_await lf::call[fib](n - 2); 
```

### The cactus-stack

Normally each call to a coroutine would allocate on the heap. However, `libfork` implements a cactus-stack supported by stack-staling which allows each coroutine to be allocated on a fragment of linear stack, this has the same overhead as allocating on the real stack. This means the overhead of a fork/call in `libfork` is very low compared to most traditional library-based implementations (about 8x the overhead of a bare function call).

### Details on the first argument

Quite a lot of additional information is passed in the first argument to an async function which is part of the API, have a look at the [full docs](https://conorwilliams.github.io/libfork/api/core.html#_CPPv4I0EN2lf4core9first_argE) once you've finished with this README.

### Restrictions on references

References as inputs to coroutines can be error prone, for example:

```c++
co_await lf::fork[process_string](std::string("32")); 
```

This would dangle if `process_string` accepted arguments by reference. Specifically `std::string &` would not compile by the standard reference semantics while `std::string const &` and `std::string &&` would compile but would dangle. To avoid this `libfork` coroutines ban r-value references as inputs for forked async-functions, as passing a temporary is always an error. If you want to move a value into a forked coroutine then pass by value.

__Note:__ This does not stop you from dangling with `std::string const &`...

### Directly invoking async functions

Sometimes you may want to invoke an async function directly instead of having to bind a result with `lf::call`. For example if you are using one of the parallel algorithms:

```c++
inline constexpr lf::async modify_vec = [](auto, std::vector<int> & numbers) -> lf::task<int> { 
  co_await lf::for_each(numbers, modify)
};
```

Where `modify` is a regular or async function that modifies a single element of the vector such as:

```c++
inline constexpr lf::async modify = [](auto, int &n) -> lf::task<void> { 
  // ... Some expensive modifying operation on n ...
};
```

If `lf::for_each` returned a value then so would the `co_await` expression. Invoking and awaiting an async function directly has the same semantics as `lf::call` however, it will automatically bind the return value to a temporary variable.

### Exceptions

In libfork exceptions are not allowed to escape a coroutine. If they do the program will terminate. This is because exceptions are very unergenomic in fork-join (without language support) as any exceptions in an async scope could bypass a join. Requiring the user to call `co_await lf::join` in a catch block would be very error prone, especially as the join would have to propagate exceptions from child tasks. If C++ adds the ability to `co_await` in a destructor then this position will be re-evaluated.

### Delaying construction

Some types are expensive or impossible to default construct, for these instances `libfork` provides the `lf::eventually` type which functions like `std::optional` but with the semantics of guaranteed construction. For example, if you have an immovable and non-default-constructible type `difficult`:

```c++
struct difficult : lf::impl::immovable<difficult> {
  difficult(int){};
  difficult(int, int){};
};
```

And a function which consumes values of this type.

```c++
void use_difficult(difficult const &) {
  // Do something...
}
```

You can write an async function that returns a `difficult`:

```c++
inline constexpr lf::async make_difficult = [](auto, bool opt) -> lf::task<difficult> {
  if (opt) {
    co_return 34;
  } else {
    co_return lf::in_place{1, 2};
  }
};
```

This mirror a regular function but uses the `lf::in_place` wrapper to forward arguments to the constructor, instead of C++17's guaranteed RVO. Now we can use `make_difficult` in an async function with `lf::eventually`:

```c++
inline constexpr lf::async eventually_example = [](auto) -> lf::task<> {
  // Delay construction:
  lf::eventually<difficult> a, b;

  // Make two difficult objects (in parallel):
  co_await lf::fork[a, make_difficult](true);
  co_await lf::call[b, make_difficult](false);

  // Wait for both to complete:
  co_await lf::join;

  // Now we can access the values:
  use_difficult(*a);
  use_difficult(*b);
};
```

We could have used `std::optional` here (and maybe that is the safer thing to do as it is UB to destroy an `lf::eventually` without first assigning to it) however, `lf::eventually` is slightly more efficient (no checks in the destructor/assignment).

### Contexts and schedulers

We have already encountered a scheduler in the [fork-join introduction](#fork-join-introduction) however, we have not yet discussed what a scheduler is or how to implement one. A scheduler is a type that implements the `lf::scheduler` concept, this is a customization point that allows you to implement your own scheduling strategy. This makes a type suitable for use with `lf::sync_wait` The `lf::scheduler` concept requires a type to define a nested `context_type` and a `schedule` member function. Have a look at the [extensions api]() for further details.

Three schedulers are provided by `libfork`:

- [`lf::lazy_pool`](https://conorwilliams.github.io/libfork/api/schedule.html#lazy-pool) is a NUMA-aware work-stealing scheduler that is suitable for general use. This should be the default choice for most applications.
- [`lf::busy_pool`](https://conorwilliams.github.io/libfork/api/schedule.html#busy-pool) is also a NUMA-aware work-stealing scheduler however workers will busy-wait for work instead of sleeping. This often gains very little performance over `lf::lazy_pool` and should only be preferred if you have an otherwise idle machine and you are willing to sacrifice a lot of power consumption for very little performance.
- [`lf::unit_pool`](https://conorwilliams.github.io/libfork/api/schedule.html#lazy-pool) is single threaded scheduler that is suitable for testing and debugging.
