#include <benchmark/benchmark.h>

#include "macros.hpp"
#include "skynet.hpp"

import std;

namespace {

auto skynet_recurse(std::int64_t num, int depth) -> std::int64_t {
  if (depth == 0) {
    return num;
  }

  std::int64_t sub = skynet_leaves(depth - 1);
  std::int64_t sum = 0;
  for (int i = 0; i < skynet_branching; ++i) {
    sum += skynet_recurse(num + i * sub, depth - 1);
  }
  return sum;
}

template <typename = void>
void skynet_serial(benchmark::State &state) {
  run_skynet(state, skynet_recurse);
}

} // namespace

BENCH_ALL(skynet_serial, serial, skynet, skynet)
