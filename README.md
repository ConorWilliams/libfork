


# Welcome to libfork (🍴) [![Continuous Integration](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml/badge.svg)](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml) [![codecov](https://codecov.io/gh/ConorWilliams/libfork/branch/main/graph/badge.svg?token=89MTSXI85F)](https://codecov.io/gh/ConorWilliams/libfork)

Libfork is primarily an abstraction for lock-free, wait-free, continuation-stealing [fork-join parallelism](https://en.wikipedia.org/wiki/Fork%E2%80%93join_model). This is made possible without the use of any macros/inline assembly using C++20's coroutines. Libfork presents an API that decouples scheduling tasks (a customization point) from writing tasks and expressing their dependencies. Secondarily, libfork provides a performant work-stealing scheduler for general use.

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Tasking

The tasking fork-join interface is designed to mirror Cilk.

```c++

template <typename T>
using task = lf::basic_task<int, busy_pool::context>


/// Compute the n'th fibonacci number
auto fib(int n) -> task<int> { 

  if (n < 2) {
    co_return n;
  }

  auto a = co_await fib(n - 1).fork(); // Spawn a child task.
  auto b = co_await fib(n - 2);        // Execute inline.

  co_await lf::join();                 // Wait for children.

  co_return *a + b;                    // Use * to deference future
}
```

# Scheduling

# Benchmarks

## Fibonacci

This benchmark evenly spawns many-tasks that do almost no computation hence, this benchmark predominantly test task creation/scheduling/deletion overhead.

| relative |          ns/fib(20) |           fib(20)/s |    err% |     total | Fibonacci
|---------:|--------------------:|--------------------:|--------:|----------:|:----------
|   100.0% |        1,415,052.87 |              706.69 |    1.5% |     11.01 | `openMP 1 threads`
|    23.2% |        6,109,785.29 |              163.67 |    4.5% |     11.14 | `openMP 2 threads`
|    27.6% |        5,129,135.71 |              194.96 |    1.1% |     10.58 | `openMP 3 threads`
|    30.2% |        4,682,091.67 |              213.58 |    2.3% |     11.23 | `openMP 4 threads`
|   171.2% |          826,451.28 |            1,209.99 |    0.9% |     10.81 | `busy_pool 1 threads`
|   260.9% |          542,472.80 |            1,843.41 |    3.9% |     10.91 | `busy_pool 2 threads`
|   304.0% |          465,541.16 |            2,148.04 |    1.4% |     11.08 | `busy_pool 3 threads`
|   319.2% |          443,302.26 |            2,255.80 |    2.3% |     11.01 | `busy_pool 4 threads`
|    82.0% |        1,725,828.99 |              579.43 |    1.3% |     10.86 | `intel TBB 1 threads`
|   133.1% |        1,063,020.11 |              940.72 |    4.3% |     10.96 | `intel TBB 2 threads`
|   158.4% |          893,258.47 |            1,119.50 |    1.5% |     10.97 | `intel TBB 3 threads`
|   164.2% |          861,915.85 |            1,160.21 |    3.1% |     11.11 | `intel TBB 4 threads`

# API reference

See the [API documentation](https://conorwilliams.github.io/libfork/) document.

# Contributing

See the [HACKING](HACKING.md) document.

# Reference

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

