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

template <fold_projection_mode Projection, typename T>
constexpr auto make_projection() {
  if constexpr (Projection == fold_projection_mode::sync) {
    return std::identity{};
  } else {
    return async_identity<T>{};
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
                        fold_accum_t<T>{},
                        std::plus<>{},
                        std::move(projection))
        .get();
  } else {
    using diff_t = std::ranges::range_difference_t<Range>;
    constexpr diff_t chunk = Chunk == fold_chunk_mode::explicit_one ? diff_t{1} : diff_t{1000};
    return lf::schedule(pool,
                        lf::fold,
                        std::ranges::begin(range),
                        std::ranges::end(range),
                        chunk,
                        fold_accum_t<T>{},
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
