#include <benchmark/benchmark.h>

#include "fold.hpp"

import std;

namespace {

template <fold_data_mode Data, typename T>
void fold_serial(benchmark::State &state) {
  run_fold_input<Data, T>(state, [](auto &&values) -> fold_accum_t<T> {
    fold_accum_t<T> result{};

    for (T value : values) {
      result += static_cast<fold_accum_t<T>>(value);
    }

    return result;
  });
}

} // namespace

#define LF_REGISTER_FOLD_SERIAL(data, dtype)                                                                 \
  LF_FOLD_BENCH_SIZES(fold_serial, serial, fold / std_plus, data, dtype)

LF_REGISTER_FOLD_SERIAL(memory, int32)
LF_REGISTER_FOLD_SERIAL(memory, float32)
LF_REGISTER_FOLD_SERIAL(lazy, int32)
LF_REGISTER_FOLD_SERIAL(lazy, float32)
