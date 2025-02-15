#ifndef CE977DFD_3A46_4443_81E7_243C91B6B416
#define CE977DFD_3A46_4443_81E7_243C91B6B416

#include <thread>

#include <benchmark/benchmark.h>

inline auto num_threads() noexcept -> int { return static_cast<int>(std::thread::hardware_concurrency()); }

inline void targs(benchmark::internal::Benchmark *bench) {

  for (auto &&elem : {1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64}) {
    if (elem > num_threads()) {
      return;
    }
    bench->Arg(elem);
  }

  int count = 64 + 16;

  while (count <= num_threads()) {
    bench->Arg(count);
    count += 16;
  }
}

/**
 * @brief Tidy up warnings.
 */
inline auto ignore(auto &&.../* unused */) -> void {}

#endif /* CE977DFD_3A46_4443_81E7_243C91B6B416 */
