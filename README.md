# `riften::Thiefpool`

A blazing-fast, lightweight, work-stealing thread-pool for C++20. Built on the lock-free concurrent [`riften::Deque`](https://github.com/ConorWilliams/ConcurrentDeque).

## Usage

```C++
#include "riften/thiefpool.hpp"

// Create thread pool with 4 worker threads.
riften::Thiefpool pool(4);

// Enqueue and return future.
auto result = pool.enqueue([](int x) { return x; }, 42);

// Get result from future.
std::cout << result.get() << std::endl;
```

Additionally, `riften::Thiefpool` supplies a detaching version of enqueue:

```C++
// Enqueue and return nothing
pool.enqueue_detach([](int x) { do_work(x); }, x);
```
Which elides the allocation of a `std::future`'s shared state.

## Installation

The recommended way to consume this library is through [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake), just add:

```CMake
CPMAddPackage("gh:ConorWilliams/Threadpool#v2.1.1")
```
to your `CMakeLists.txt` and you're good to go!

## Tests

To compile and run the tests:
```zsh
mkdir build && cd build
cmake ../test
make && make test
```
