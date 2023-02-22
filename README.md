


# Welcome to libfork (🍴) [![Continuous Integration](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml/badge.svg)](https://github.com/ConorWilliams/libfork/actions/workflows/ci.yml) [![codecov](https://codecov.io/gh/ConorWilliams/libfork/branch/main/graph/badge.svg?token=89MTSXI85F)](https://codecov.io/gh/ConorWilliams/libfork)

Libfork (🍴) is primarily an abstraction for lock-free, wait-free, continuation-stealing [fork-join parallelism](https://en.wikipedia.org/wiki/Fork%E2%80%93join_model). This is made possible without the use of any macros/inline assembly using C++20's coroutines. Libfork presents an API that decouples scheduling tasks (a customisation point) from writing tasks and expressing their dependencies. Secondarily, libfork provides a performant work-stealing scheduler for general use.

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Tasking

The tasking fork-join interface is designed to mirror Cilk.

# Scheduling

# Benchmarks

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



