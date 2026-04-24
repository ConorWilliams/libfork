#pragma once

#include <benchmark/benchmark.h>

// Use `import std;` by default. Textually `#include <thread>` drags in
// `<stop_token>`, which triggers a libc++ 22 link-time bug (undefined
// `__atomic_unique_lock::__set_locked_bit`) in TUs that later instantiate
// anything touching std::stop_*. Targets that can't use modules (e.g. the
// openmp benchmarks, see benchmark/src/openmp/CMakeLists.txt) define
// LF_BENCH_NO_IMPORT_STD and get textual includes instead.
#ifdef LF_BENCH_NO_IMPORT_STD
  #include <algorithm>
  #include <thread>
#else
import std;
#endif

inline void bench_thread_args(benchmark::Benchmark *bench, auto make_args) {
  unsigned hw = std::max(1U, std::thread::hardware_concurrency());
  for (unsigned t : {1U, 2U, 4U, 6U, 8U, 12U, 16U, 24U, 32U, 48U, 64U, 96U}) {
    if (t > hw) {
      return;
    }
    make_args(bench, t);
  }
}
