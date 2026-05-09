#include <benchmark/benchmark.h>

#include "fold.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

template <typename T>
struct async_identity {
  template <typename Context>
  static auto operator()(lf::env<Context>, T value) -> lf::task<T, Context> {
    co_return value;
  }
};

template <typename T>
using accum_t = lf_bench::fold_accum_t<T>;

template <lf_bench::fold_chunk_mode Chunk,
          lf_bench::fold_projection_mode Projection,
          typename Range,
          typename T>
auto run_fold(mono_busy_pool &pool, Range &&range) -> accum_t<T> {
  using diff_t = std::ranges::range_difference_t<Range>;

  if constexpr (Projection == lf_bench::fold_projection_mode::sync) {
    if constexpr (Chunk == lf_bench::fold_chunk_mode::deduced) {
      return lf::schedule(
                 pool, lf::fold, std::forward<Range>(range), accum_t<T>{}, std::plus<>{}, std::identity{})
          .get();
    } else if constexpr (Chunk == lf_bench::fold_chunk_mode::explicit_one) {
      return lf::schedule(pool,
                          lf::fold,
                          std::forward<Range>(range),
                          diff_t{1},
                          accum_t<T>{},
                          std::plus<>{},
                          std::identity{})
          .get();
    } else {
      return lf::schedule(pool,
                          lf::fold,
                          std::forward<Range>(range),
                          diff_t{1000},
                          accum_t<T>{},
                          std::plus<>{},
                          std::identity{})
          .get();
    }
  } else {
    if constexpr (Chunk == lf_bench::fold_chunk_mode::deduced) {
      return lf::schedule(
                 pool, lf::fold, std::forward<Range>(range), accum_t<T>{}, std::plus<>{}, async_identity<T>{})
          .get();
    } else if constexpr (Chunk == lf_bench::fold_chunk_mode::explicit_one) {
      return lf::schedule(pool,
                          lf::fold,
                          std::forward<Range>(range),
                          diff_t{1},
                          accum_t<T>{},
                          std::plus<>{},
                          async_identity<T>{})
          .get();
    } else {
      return lf::schedule(pool,
                          lf::fold,
                          std::forward<Range>(range),
                          diff_t{1000},
                          accum_t<T>{},
                          std::plus<>{},
                          async_identity<T>{})
          .get();
    }
  }
}

template <typename T>
void set_fold_counters(benchmark::State &state, std::size_t n) {
  state.counters["n"] = static_cast<double>(n);
  state.counters["bytes"] = static_cast<double>(n * sizeof(T));
}

template <lf_bench::fold_data_mode Data,
          lf_bench::fold_chunk_mode Chunk,
          lf_bench::fold_projection_mode Projection,
          typename T>
void fold_run(benchmark::State &state) {
  auto n = static_cast<std::size_t>(state.range(0));
  set_fold_counters<T>(state, n);

  mono_busy_pool pool{1};

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
      auto result = run_fold<Chunk, Projection, std::span<T>, T>(pool, std::span<T>{values});
      benchmark::DoNotOptimize(result);
    }
  } else {
    auto values = lf_bench::ones_range<T>{.count = n};

    for (auto _ : state) {
      benchmark::DoNotOptimize(values);
      auto result = run_fold<Chunk, Projection, lf_bench::ones_range<T> &, T>(pool, values);
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

template <lf_bench::fold_data_mode Data,
          lf_bench::fold_chunk_mode Chunk,
          lf_bench::fold_projection_mode Projection,
          typename T>
void register_fold_one(std::string_view mode, bool full_sweep) {
  auto name = std::format("std::plus<>/{}/{}/{}/{}",
                          lf_bench::fold_data_name(Data),
                          lf_bench::fold_dtype_name<T>(),
                          lf_bench::fold_projection_name(Projection),
                          lf_bench::fold_chunk_name(Chunk));
  auto *bench = benchmark::RegisterBenchmark(
      lf_bench::format_name(std::string{mode}, "libfork", "fold", std::string{name}),
      fold_run<Data, Chunk, Projection, T>);
  add_fold_args<T>(bench, full_sweep);
}

template <lf_bench::fold_data_mode Data,
          lf_bench::fold_chunk_mode Chunk,
          lf_bench::fold_projection_mode Projection>
void register_fold_types(std::string_view mode, bool full_sweep) {
  register_fold_one<Data, Chunk, Projection, std::int32_t>(mode, full_sweep);
  register_fold_one<Data, Chunk, Projection, float>(mode, full_sweep);
}

template <lf_bench::fold_data_mode Data, lf_bench::fold_chunk_mode Chunk>
void register_fold_projection_modes(std::string_view mode, bool full_sweep) {
  register_fold_types<Data, Chunk, lf_bench::fold_projection_mode::sync>(mode, full_sweep);
  register_fold_types<Data, Chunk, lf_bench::fold_projection_mode::async>(mode, full_sweep);
}

template <lf_bench::fold_data_mode Data>
void register_fold_chunk_modes(std::string_view mode, bool full_sweep) {
  register_fold_projection_modes<Data, lf_bench::fold_chunk_mode::explicit_one>(mode, full_sweep);
  register_fold_projection_modes<Data, lf_bench::fold_chunk_mode::deduced>(mode, full_sweep);
  register_fold_projection_modes<Data, lf_bench::fold_chunk_mode::k1000>(mode, full_sweep);
}

void register_fold_data_modes(std::string_view mode, bool full_sweep) {
  register_fold_chunk_modes<lf_bench::fold_data_mode::memory>(mode, full_sweep);
  register_fold_chunk_modes<lf_bench::fold_data_mode::lazy>(mode, full_sweep);
}

struct fold_registrar {
  fold_registrar() {
    register_fold_data_modes("test", false);
    register_fold_data_modes("base", true);
  }
} fold_registrar_instance;

} // namespace
