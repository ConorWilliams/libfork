#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>

#include <benchmark/benchmark.h>

#include <libfork.hpp>
#include <libfork/core/sync_wait.hpp>
#include <numeric>

#include "../util.hpp"
#include "config.hpp"

namespace {

using namespace lf;

constexpr async nqueens = []<std::size_t N>(auto nqueens, int j, std::array<char, N> const &a) -> task<long> {
  if (N == j) {
    co_return 1;
  }

  long res = 0L;

  std::array<std::array<char, N>, N> buf;
  std::array<long, N> parts;

  for (int i = 0; i < N; i++) {

    for (int k = 0; k < j; k++) {
      buf[i][k] = a[k];
    }

    buf[i][j] = i;

    if (queens_ok(j + 1, buf[i].data())) {
      co_await lf::fork(parts[i], nqueens)(j + 1, buf[i]);
    } else {
      parts[i] = 0;
    }
  }

  co_await lf::join;

  co_return std::accumulate(parts.begin(), parts.end(), 0L);
};

template <lf::scheduler Sch, lf::numa_strategy Strategy>
void nqueens_libfork(benchmark::State &state) {

  Sch sch = [&] {
    if constexpr (std::constructible_from<Sch, int>) {
      return Sch(state.range(0));
    } else {
      return Sch{};
    }
  }();

  volatile int output;

  std::array<char, 13> buf{};

  for (auto _ : state) {
    output = lf::sync_wait(sch, nqueens, 0, buf);
  }
}

} // namespace

using namespace lf;

BENCHMARK(nqueens_libfork<busy_pool, numa_strategy::seq>)->DenseRange(1, num_threads())->UseRealTime();
