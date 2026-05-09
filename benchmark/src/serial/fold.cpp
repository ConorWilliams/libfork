#include <benchmark/benchmark.h>

#include "fold.hpp"
#include "macros.hpp"

import std;

namespace {

template <typename T>
using accum_t = fold_accum_t<T>;

template <fold_data_mode Data, typename T>
void fold_reduce(benchmark::State &state) {
  run_fold_input<Data, T>(state, [](auto &&values) -> accum_t<T> {
    return std::reduce(std::ranges::begin(values), std::ranges::end(values), accum_t<T>{}, std::plus<>{});
  });
}

} // namespace

#define LF_REGISTER_FOLD_REDUCE(data, dtype)                                                                 \
  LF_FOLD_BENCH_SIZES(fold_reduce, serial, fold / std_reduce, data, dtype)

LF_REGISTER_FOLD_REDUCE(memory, int32)
LF_REGISTER_FOLD_REDUCE(memory, float32)
LF_REGISTER_FOLD_REDUCE(lazy, int32)
LF_REGISTER_FOLD_REDUCE(lazy, float32)
