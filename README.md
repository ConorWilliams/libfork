# `riften::Forkpool`

A bleeding-edge, lock-free, wait-free, continuation-stealing scheduler for C++20. This project uses C++20's coroutines to implement continuation-stealing scheduler without the use of any macros/inline assembly. The interface is designed to mirror [`Cilk`](https://en.wikipedia.org/wiki/Cilk). 

```C++
#include "riften/thiefpool.hpp"

using namespace riften;

Task<int> fib(int n) { // Define a task to be run on the threadpool
    if (n < 2) {
        co_return n;
    } else {
        Future a = co_await fork(fib, n - 1); // Tasks can recursivly spawn tasks 
        Future b = co_await fork(fib, n - 2);

        co_await tag_sync(); // Sync before consuminging the futures

        co_return *a + *b;
    }
}

int main(){

    int result = root(fib, 10) // Submit a root task to the threadpool and block until it completes

    return 0;
}

```



## Installation

The recommended way to consume this library is through [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake), just add:

```CMake
CPMAddPackage("gh:ConorWilliams/Forkpool#v1.0.0")
```
to your `CMakeLists.txt` and you're good to go!

## Tests

To compile and run the tests:
```zsh
mkdir build && cd build
cmake ../test
make && make test
```

## Reference

This project implements many of the ideas in (available in `reference/`):

1. F. Schmaus et al., “Nowa: A Wait-Free Continuation-Stealing
Concurrency Platform”. In: 2021 IEEE International Parallel and
Distributed Processing Symposium (IPDPS). 2021.

2. C. -X. Lin, T. -W. Huang and M. D. F. Wong, "An Efficient Work-Stealing Scheduler for Task Dependency Graph," 2020 IEEE 26th International Conference on Parallel and Distributed Systems (ICPADS), 2020, pp. 64-71, doi: 10.1109/ICPADS51040.2020.00018.

