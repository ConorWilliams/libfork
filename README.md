
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

<h3 align="center"> ** Now supplying 🌵! **  </h1>

Libfork is primarily an abstraction for fully-portable, strict, [fork-join parallelism](https://en.wikipedia.org/wiki/Fork%E2%80%93join_model). This is made possible without the use of any macros/inline assembly using C++20's coroutines. Ultra-fine grained parallelism (the ability to spawn tasks with very low overhead) is enabled by an innovative implementation of an (almost) non-allocating [cactus-stack](https://en.wikipedia.org/wiki/Parent_pointer_tree) utilizing _segmented stacks_. Libfork presents a cross-platform API that decouples scheduling tasks (a customization point) from writing tasks. Additionally, libfork provides performant NUMA-aware work-stealing schedulers for general use. If you'd like to learn more check out [the tour of libfork](#a-tour-of-libfork) then try it on [compiler explorer](https://godbolt.org/z/nzTPKqrrq) or, just grok the __TLDR__:

```cpp
#include "libfork/core.hpp"

inline constexpr auto fib = [](auto fib, int n) -> lf::task<int> { 
  
  if (n < 2) {
    co_return n;
  }

  int a, b;

  co_await lf::fork[&a, fib](n - 1);    // Spawn a child task.
  co_await lf::call[&b, fib](n - 2);    // Execute a child inline.

  co_await lf::join;                    // Wait for children.

  co_return a + b;                      // Safe to read after a join.
};
```

## Performance

Libfork is engineered for performance and has a comprehensive [benchmark suit](bench). For a detailed review of libfork see the [paper](TODO), the headline results are linear time/memory scaling:

- Up to 7.5× faster and 19× less memory than OneTBB.
- Up to 24× faster an 24× less memory than openMP (libomp).
- Up to 100× faster and >100× less memory than taskflow.

## Using libfork

Libfork is a header-only library with full CMake support and zero required-dependencies. Refer to the [BUILDING](BUILDING.md) document for full details on compiling the tests/benchmarks/docs, installation, optional dependencies and, tools for developers. See below for the easiest ways to consume libfork in your CMake projects.

### If you have installed libfork

```cmake
find_package(libfork REQUIRED)

target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

### Using CMake's ``FetchContent``

```cmake
include(FetchContent)

FetchContent_Declare(
    libfork
    GIT_REPOSITORY https://github.com/conorwilliams/libfork.git
    GIT_TAG v3.0.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(libfork)

target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

### Using [CMP.cmake](https://github.com/cpm-cmake/CPM.cmake)

```cmake
# Assuming your ``CPM.cmake`` file is the ``cmake`` directory.
include(cmake/CPM.cmake)

CPMAddPackage("gh:conorwilliams/libfork#3.0.0")

target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

### Using git submodules

```cmake
# Assuming you cloned libfork as a submodule into "external/libfork".
add_subdirectory(external/libfork)

target_link_libraries(
    project_target PRIVATE libfork::libfork
)
```

### Single header

Although this is __not recommend__ and primarily exist for easy integration with [godbolt](https://godbolt.org/z/nzTPKqrrq); libfork supplies a [single header](single_header/libfork.hpp) that you can copy-and-paste into your project. See the [BUILDING](BUILDING.md) document's note about hwloc integration.

<!-- TODO: godbolt with include. -->

## API reference

See the generated [docs](https://conorwilliams.github.io/libfork/).

## Contributing

1. Read the [CODE_OF_CONDUCT](CODE_OF_CONDUCT.md) document.
2. Read the [BUILDING](BUILDING.md) document.
3. Have a snoop around the `impl` namespace.
4. Ask as many questions as you can think of!

## Changelog

See the [ChangeLog](ChangeLog.md) document.

## A tour of libfork

This section provides some background and highlights of the `core` API, for details on implementing your own schedulers on-top of libfork see the [extension documentation](https://conorwilliams.github.io/libfork/). Don't forget you can play around with libfork on [godbolt](https://godbolt.org/z/nzTPKqrrq).

### Contents

- [Fork-join](#fork-join)
- [The cactus stack](#the-cactus-stack)
- [Restrictions on references](#restrictions-on-references)
- [Delaying construction with `lf::eventually<T>`](#delaying-construction)
- [Exception in libfork](#exceptions)
- [Immediate invocation](#immediate-invocation)
- [Explicit scheduling](#explicit-scheduling)
- [Contexts and schedulers](#contexts-and-schedulers)

### Fork-join

Definitions:

- __Task:__ A unit of work that can be executed concurrently with other tasks.
- __Parent:__ A task that spawns other tasks.
- __Child:__ A task that is spawned by another task.

The tasking/fork-join interface is designed to mirror [Cilk](https://en.wikipedia.org/wiki/Cilk) and other fork-join frameworks. The best way to learn is by example, lets start with the canonical introduction to fork-join, the recursive Fibonacci function, in regular C++ it looks like this:

```cpp
auto fib(int n) -> int {
  
  if (n < 2) {
    return n;
  }

  int a = fib(n - 1);
  int b = fib(n - 2);

  return a + b;
}
```

We've already seen how to implement this with libfork in the TLDR but, here it is again with line numbers:

```cpp
 1| #include "libfork/core.hpp"
 2|
 3| inline constexpr fib = [](auto fib, int n) -> lf::task<int> { 
 4|  
 5|   if (n < 2) {
 6|     co_return n;
 7|   }
 8|
 9|   int a, b;
10|
11|   co_await lf::fork[&a, fib](n - 1);    
12|   co_await lf::call[&b, fib](n - 2);  
13|
14|   co_await lf::join;                  
15|
16|   co_return a + b;                    
17| };
```

__NOTE:__ If your compiler does not support the `lf::fork[&a, fib]` syntax then you can use `lf::fork(&a, fib)` and similarly for `lf::call`.

This looks almost like the regular recursive Fibonacci function. However, there are some important differences which we'll explain in a moment. First, the above fibonacci function can be launched on a scheduler, like ``lazy_pool``, as follows:

```cpp
#include "libfork/schedule.hpp"

int main() {
  
  lf::lazy_pool pool(4); // 4 worker threads

  int fib_10 = lf::sync_wait(pool, fib, 10);
}
```

The call to `sync_wait` will block the _main_ thread (i.e. the thread that calls `main()`)  until the pool has completed execution of the task. Let's break down what happens after that line by line:

- __Line 3:__ First we define an _async function_. An async function is a function-object with a templated first argument that returns an `lf::task<T>`. The first argument is used by the library to pass static and dynamic context from parent to child. Additionally, it acts as a [y-combinator](https://en.wikipedia.org/wiki/Fixed-point_combinator) - allowing the lambda to be recursive - and provides a few methods which we will discuss later.
- __Line 9:__ Next we construct the variables that will be bound to the return values of following forks/calls.
- __Line 11:__ This is the first call to `lf::fork` which marks the beginning of an _async scope_. `lf::fork[&a, fib]` binds the return address of the function `fib` to the integer `a`. Internally the child coroutine will have to store a pointer to the return variable so we make this explicit at the call site. The bound function is then invoked with the argument `n - 1`. The semantics of all of this is: the execution of the forked function (in this case `fib`) can continue concurrently with the execution of the next line of code i.e. the _continuation_. As libfork is a continuation stealing library the worker/thread that performed the fork will immediately begin executing the forked function while another thread may _steal_ the continuation.
- __Line 12:__ An `lf::call` binds arguments and return address in the same way as `lf::fork` however, it has the semantics of a serial function call. This is done instead of an `lf::fork` as there is no further work to do in the current task so stealing it would be a waste of resources.
- __Line 13:__ Execution cannot continue past a join-point until all child tasks have completed. After this point it is safe to access the results (`a` and `b`) of the child task. Only a single worker will continue execution after the join. This marks the end of the async scope that began at the `fork`.
- __Line 16:__ Finally we return the result of the to a parent task, this has similar semantics to a regular return however, behind the scenes an assignment of the return value to the parent's return address is performed. This is the end of the async function. The worker will attempt to resume the parent task (if it has not already been stolen) just as a regular function would resume execution of the caller.

__NOTE:__ At every ``co_await`` the OS-thread executing the task may change!

__NOTE:__ Libfork implements _strict_ fork-join which means all children __must__ be joined __before__ a task returns. This restriction give some nice mathematical properties to the underlying directed acyclic graph (DAG) of tasks that enables many optimizations.

#### Ignoring a result

If you wanted to ignore the result of a fork/call (i.e. if you wanted the side effect only) you can simply omit return address from lines 11 and 12 e.g.:

```cpp
co_await lf::fork[fib](n - 1);    
co_await lf::call[fib](n - 2); 
```

### The cactus-stack

Normally each call to a coroutine would allocate on the heap. However, libfork implements a cactus-stack - supported by segmented-stacks - which allows each coroutine to be allocated on a fragment of linear stack, this has almost the same overhead as allocating on the real stack. This means the overhead of a fork/call in libfork is very low compared to most traditional library-based implementations (about 10x the overhead of a bare function call).

The internal cactus-stack is exposed to the user vi the `co_new` function:

```cpp
inline constexpr auto co_new_demo = [](auto co_new_demo, std::span<int> inputs) -> lf::task<int> {
 
  // Allocate space for results, outputs is a std::span<int>
  auto [outputs] = co_await lf::co_new<int>(inputs.size());

  // Launch a task for each input.
  for(std::size_t i = 0; i < inputs.size(); ++i) {
    co_await lf::fork[&a[i], some_function](inputs[i]);
  }

  co_await lf::join; // Wait for all tasks to complete.

  co_return std::accumulate(outputs.begin(), outputs.end(), 0);
};
```

Here the `co_await` on the result of `lf::co_new` returns an immovable RAII class which will manage the lifetime of the allocation.

### Restrictions on references

References as inputs to coroutines can be error prone, for example:

```cpp
co_await lf::fork[process_string](std::string("32")); 
```

This would dangle if `process_string` accepted arguments by reference. Specifically a `process_string` accepting `std::string &` would not compile by the standard reference semantics while `std::string const &` and `std::string &&` would compile but would dangle. To avoid this libfork coroutines bans `std::string && -> std::string const &` conversions and r-value reference arguments for forked async-functions. If you want to move a value into a forked coroutine then pass by value.

__Note:__ You can still dangle by ending the lifetime of an l-value referenced object __after__ a fork.

### Delaying construction

Some types are expensive or impossible to default construct, for these instances libfork provides the `lf::eventually` template type. `lf::eventually` functions like a `std::optional` that is only constructed once and supports references:

```cpp
// Not default constructible.
struct difficult {
  difficult(int) {}
};

// Async function that returns a difficult.
inline constexpr auto make_difficult = [](auto) -> lf::task<difficult> {
  co_return 42;
}

// Async function that returns a reference.
inline constexpr auto reference = [](auto) -> lf::task<int &> {
  co_return /* some reference */;
}

inline constexpr auto eventually_demo = [](auto) -> lf::task<> { 

  // Use lf::eventually to delay construction.
  lf::eventually<difficult> a;
  lf::eventually<int &> b;
  
  co_await lf::fork[&a, make_difficult]();    
  co_await lf::fork[&b, reference]();

  co_await lf::join;     

  std::cout << *b << std::endl; // lf::eventually support operators * and ->
};
```

### Exceptions

Libfork supports exceptions in async functions. If an exception escapes an async function then it will be stored in its parent and re-thrown when the parent reaches a join-point. For example:

```cpp
inline constexpr auto exception_demo = [](auto) -> lf::task<> { 
  
  co_await lf::fork[throwing_work](/* args.. */);    
  co_await lf::fork[throwing_work](/* args.. */);   

  co_await lf::join; // Will (re)throw one of the exceptions from the children.                                    
};
```

However, you need to be very careful when throwing exception inside a fork-join scope because it's UB for a task which has forked children to return (regularly or by exception) without first calling `lf::join`. For example:

```cpp
inline constexpr auto bad_code = [](auto) -> lf::task<> { 
  
  co_await lf::fork[work](/* args.. */);    

  function_which_could_throw();  // UB on exception! No join before return.

  co_await lf::join;                                       
};
```

Instead ypu must wrap your potentially throwing code in a try-catch block and call `lf::join`.

```cpp
inline constexpr auto good_code = [](auto good_code) -> lf::task<> { 
  
  co_await lf::fork[&a, work](/* args.. */);    

  try {
    function_which_could_throw(); 
  } catch (...) {
    good_code.stash_exception(); // Store's exception.
  } 

  co_await lf::join; // Exception from child or stash_exception will be re-thrown here.                             
};
```

If/when C++ adds asynchronous RAII then this will be made much cleaner.

If you would like to capture exceptions for each child individually then you can use a return object that supports capturing exceptions, for example:

```cpp
inline constexpr auto exception_stash_demo = [](auto) -> lf::task<> { 
  
  try_eventually<int> ret;
  
  co_await lf::fork[&ret, int_or_throw](/* args.. */);    

  co_await lf::join; // Will not throw, exception stored in ret.

  if (ret.has_exception()) {
    // Handle exception.
  } else {
    // Handle result.
  }                            
};
```

### Immediate invocation

Sometimes you may want to just call an async function without a fork join scope, for example:

```cpp
int result;

co_await lf::fork[&result, some_function](/* args.. */);

co_await lf::join; // Still needed in-case of exceptions
```

In this case you could simplify the above with `lf::just`:

```cpp
int result = co_await lf::just[some_function](/* args.. */);
```

### Explicit scheduling

Normally in libfork _where_ a task is being executed is controlled by the runtime. However, you may want to explicitly schedule a task to be resumed on a certain worker or write an awaitable that transfers execution to a different pool of workers. This is made possible through the `context_switcher` API. Instead of writing a regular awaitable, write one that conforms to the `context_switcher` concept, like this:
  
```cpp
struct my_special_awaitable {
  auto await_ready() -> bool;
  auto await_suspend(lf::submit_handle handle)  -> void;
  auto await_resume()  -> /* [T] */;
};
```

This can be `co_await`ed inside a libfork task, if `await_ready` returns `false` then the task will be suspended and `await_suspend` will be called with a handle to the suspended task, this can be resumed by any worker you like.

This is used by libfork's `template<scheduler T> auto resume_on(T *)` to enable explicit scheduling.

### Contexts and schedulers

We have already encountered a scheduler in the [fork-join](#fork-join) however, we have not yet discussed what a scheduler is or how to implement one. A scheduler is a type that implements the `lf::scheduler` concept, this is a customization point that allows you to implement your own scheduling strategy. This makes a type suitable for use with `lf::sync_wait`. Have a look at the [extensions api](https://conorwilliams.github.io/libfork/) for further details.

Three schedulers are provided by libfork:

- [`lf::lazy_pool`](https://conorwilliams.github.io/libfork/api/schedule.html#lazy-pool) A NUMA-aware work-stealing scheduler that is suitable for general use. This should be the default choice for most applications.
- [`lf::busy_pool`](https://conorwilliams.github.io/libfork/api/schedule.html#busy-pool) Also a NUMA-aware work-stealing scheduler however, workers will busy-wait for work instead of sleeping. This often gains very little performance over `lf::lazy_pool` and should only be preferred if you have an otherwise idle machine and you are willing to sacrifice a lot of power consumption for very little performance.
- [`lf::unit_pool`](https://conorwilliams.github.io/libfork/api/schedule.html#lazy-pool) A is single threaded scheduler that is suitable for testing and debugging.

__NOTE:__ The workers inside libfork's thread pools should never block i.e. __do not__ call `sync_wait` or any other blocking function inside a `task`.
