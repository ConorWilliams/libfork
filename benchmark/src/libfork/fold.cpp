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

template <lf_bench::fold_projection_mode Projection, typename T>
constexpr auto make_projection() {
  if constexpr (Projection == lf_bench::fold_projection_mode::sync) {
    return std::identity{};
  } else {
    return async_identity<T>{};
  }
}

template <lf_bench::fold_chunk_mode Chunk,
          lf_bench::fold_projection_mode Projection,
          typename T,
          typename Range>
auto run_fold(mono_busy_pool &pool, Range &&range) -> accum_t<T> {
  using diff_t = std::ranges::range_difference_t<Range>;
  auto projection = make_projection<Projection, T>();

  if constexpr (Chunk == lf_bench::fold_chunk_mode::deduced) {
    return lf::schedule(
               pool, lf::fold, std::forward<Range>(range), accum_t<T>{}, std::plus<>{}, std::move(projection))
        .get();
  } else {
    constexpr diff_t chunk = Chunk == lf_bench::fold_chunk_mode::explicit_one ? diff_t{1} : diff_t{1000};
    return lf::schedule(pool,
                        lf::fold,
                        std::forward<Range>(range),
                        chunk,
                        accum_t<T>{},
                        std::plus<>{},
                        std::move(projection))
        .get();
  }
}

template <lf_bench::fold_data_mode Data,
          lf_bench::fold_chunk_mode Chunk,
          lf_bench::fold_projection_mode Projection,
          typename T>
void fold_run(benchmark::State &state) {
  mono_busy_pool pool{1};
  lf_bench::run_fold_input<Data, T>(state, [&](auto &&values) -> accum_t<T> {
    return run_fold<Chunk, Projection, T>(pool, std::forward<decltype(values)>(values));
  });
}

} // namespace

#define LF_REGISTER_FOLD_BENCH(data, chunk, proj, dtype)                                                     \
  LF_FOLD_BENCH_SIZES(fold_run, libfork, fold / std_plus, data, chunk, proj, dtype)

#define LF_REGISTER_FOLD_CHUNKS(data, proj, dtype)                                                           \
  LF_REGISTER_FOLD_BENCH(data, chunk_1, proj, dtype)                                                         \
  LF_REGISTER_FOLD_BENCH(data, chunk_deduced, proj, dtype)                                                   \
  LF_REGISTER_FOLD_BENCH(data, chunk_1000, proj, dtype)

#define LF_REGISTER_FOLD_PROJECTIONS(data, dtype)                                                            \
  LF_REGISTER_FOLD_CHUNKS(data, sync_proj, dtype)                                                            \
  LF_REGISTER_FOLD_CHUNKS(data, async_proj, dtype)

#define LF_REGISTER_FOLD_TYPES(data)                                                                         \
  LF_REGISTER_FOLD_PROJECTIONS(data, int32)                                                                  \
  LF_REGISTER_FOLD_PROJECTIONS(data, float32)

LF_REGISTER_FOLD_TYPES(memory)
LF_REGISTER_FOLD_TYPES(lazy)
