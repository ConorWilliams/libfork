#include <benchmark/benchmark.h>

#include "fold.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

template <typename T>
struct sync_projection {
  static constexpr auto operator()(T value) -> fold_accum_t<T> { return static_cast<fold_accum_t<T>>(value); }
};

template <typename T>
struct async_projection {
  template <typename Context>
  static auto operator()(lf::env<Context>, T value) -> lf::task<fold_accum_t<T>, Context> {
    co_return static_cast<fold_accum_t<T>>(value);
  }
};

template <fold_projection_mode Projection, typename T>
constexpr auto make_projection() {
  if constexpr (Projection == fold_projection_mode::sync) {
    return sync_projection<T>{};
  } else {
    return async_projection<T>{};
  }
}

template <fold_chunk_mode Chunk, fold_projection_mode Projection, typename T, typename Range>
auto run_fold(mono_busy_pool &pool, Range &&range) -> fold_accum_t<T> {

  auto projection = make_projection<Projection, T>();

  if constexpr (Chunk == fold_chunk_mode::deduced) {
    return lf::schedule(pool,
                        lf::fold,
                        std::ranges::begin(range),
                        std::ranges::end(range),
                        std::plus<>{},
                        std::move(projection))
        .get();
  } else {
    using diff_t = std::ranges::range_difference_t<Range>;
    constexpr diff_t chunk = Chunk == fold_chunk_mode::explicit_one ? diff_t{1} : diff_t{4096};
    return lf::schedule(pool,
                        lf::fold,
                        std::ranges::begin(range),
                        std::ranges::end(range),
                        chunk,
                        std::plus<>{},
                        std::move(projection))
        .get();
  }
}

template <fold_data_mode Data, fold_chunk_mode Chunk, fold_projection_mode Projection, typename T>
void fold_run(benchmark::State &state) {
  mono_busy_pool pool{1};
  run_fold_input<Data, T>(state, [&](auto &&values) -> fold_accum_t<T> {
    return run_fold<Chunk, Projection, T>(pool, std::forward<decltype(values)>(values));
  });
}

} // namespace

// Chunked/sync/sync versions to mirror serial benchmarks.
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, memory, chunk_fixed, sync_proj, int32)
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, memory, chunk_fixed, sync_proj, float32)
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, lazy, chunk_fixed, sync_proj, int32)
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, lazy, chunk_fixed, sync_proj, float32)


// Compare specialised for sync/async
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, memory, chunk_1, sync_proj, float32)
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, memory, chunk_deduced, sync_proj, float32)
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, memory, chunk_1, async_proj, float32)
LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, memory, chunk_deduced, async_proj, float32)
