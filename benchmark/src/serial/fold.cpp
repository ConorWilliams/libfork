#include <benchmark/benchmark.h>

#include "fold.hpp"
#include "macros.hpp"

import std;

namespace {

template <typename T>
using accum_t = lf_bench::fold_accum_t<T>;

template <typename T>
void set_fold_counters(benchmark::State &state, std::size_t n) {
  state.counters["n"] = static_cast<double>(n);
  state.counters["bytes"] = static_cast<double>(n * sizeof(T));
}

template <lf_bench::fold_data_mode Data, typename T>
void fold_reduce(benchmark::State &state) {
  auto n = static_cast<std::size_t>(state.range(0));
  set_fold_counters<T>(state, n);

  if constexpr (Data == lf_bench::fold_data_mode::memory) {
    std::vector<T> values;
    try {
      values.assign(n, T{1});
    } catch (std::bad_alloc const &) {
      state.SkipWithError("allocation failed");
      return;
    }

    for (auto _ : state) {
      benchmark::DoNotOptimize(values.data());
      auto result = std::reduce(values.begin(), values.end(), accum_t<T>{}, std::plus<>{});
      benchmark::DoNotOptimize(result);
    }
  } else {
    auto values = lf_bench::ones_range<T>{.count = n};

    for (auto _ : state) {
      benchmark::DoNotOptimize(values);
      auto result = std::reduce(values.begin(), values.end(), accum_t<T>{}, std::plus<>{});
      benchmark::DoNotOptimize(result);
    }
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
  state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(n * sizeof(T)));
}

template <typename T>
void add_fold_args(benchmark::Benchmark *bench, bool full_sweep) {
  if (full_sweep) {
    for (auto n : lf_bench::fold_base_sizes<T>()) {
      bench->Arg(n);
    }
  } else {
    bench->Arg(lf_bench::fold_test);
  }
  bench->UseRealTime();
}

template <lf_bench::fold_data_mode Data, typename T>
void register_fold_one(std::string_view mode, bool full_sweep) {
  auto name = std::format("{}/{}/std_reduce", lf_bench::fold_data_name(Data), lf_bench::fold_dtype_name<T>());
  auto *bench = benchmark::RegisterBenchmark(
      lf_bench::format_name(std::string{mode}, "serial", "fold", std::string{name}), fold_reduce<Data, T>);
  add_fold_args<T>(bench, full_sweep);
}

template <lf_bench::fold_data_mode Data>
void register_fold_types(std::string_view mode, bool full_sweep) {
  register_fold_one<Data, std::int32_t>(mode, full_sweep);
  register_fold_one<Data, float>(mode, full_sweep);
}

void register_fold_data_modes(std::string_view mode, bool full_sweep) {
  register_fold_types<lf_bench::fold_data_mode::memory>(mode, full_sweep);
  register_fold_types<lf_bench::fold_data_mode::lazy>(mode, full_sweep);
}

struct fold_registrar {
  fold_registrar() {
    register_fold_data_modes("test", false);
    register_fold_data_modes("base", true);
  }
} fold_registrar_instance;

} // namespace
