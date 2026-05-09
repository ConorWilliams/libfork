#include <benchmark/benchmark.h>

#include "fold.hpp"
#include "macros.hpp"

import std;

namespace {

template <typename T>
using accum_t = lf_bench::fold_accum_t<T>;

template <lf_bench::fold_data_mode Data, typename T>
void fold_reduce(benchmark::State &state) {
  lf_bench::run_fold_input<Data, T>(state, [](auto &&values) -> accum_t<T> {
    return std::reduce(std::ranges::begin(values), std::ranges::end(values), accum_t<T>{}, std::plus<>{});
  });
}

} // namespace

#define LF_DEFINE_FOLD_REDUCE(data, dtype_name, dtype)                                                       \
  template <typename = void>                                                                                 \
  void fold_reduce_##data##_##dtype_name(benchmark::State &state) {                                          \
    fold_reduce<lf_bench::fold_data_mode::data, dtype>(state);                                               \
  }

#define LF_REGISTER_FOLD_REDUCE(data, dtype_name, dtype)                                                     \
  LF_DEFINE_FOLD_REDUCE(data, dtype_name, dtype)                                                             \
  LF_FOLD_BENCH_SIZES(fold_reduce_##data##_##dtype_name, serial, fold / data / dtype_name / std_reduce)

LF_REGISTER_FOLD_REDUCE(memory, int32, std::int32_t)
LF_REGISTER_FOLD_REDUCE(memory, float, float)
LF_REGISTER_FOLD_REDUCE(lazy, int32, std::int32_t)
LF_REGISTER_FOLD_REDUCE(lazy, float, float)
