#pragma once

#include <benchmark/benchmark.h>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

struct incorrect_result : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

inline constexpr unsigned bench_max_threads = 12;

#define CHECK_RESULT(result, expected)                                                                       \
  do {                                                                                                       \
    auto &&lf_check_result_val = (result);                                                                   \
    auto &&lf_check_expected_val = (expected);                                                               \
    if (lf_check_result_val != lf_check_expected_val) {                                                      \
      LF_THROW(incorrect_result(                                                                             \
          std::format("{}={} != {}={}", #expected, lf_check_expected_val, #result, lf_check_result_val)));   \
    }                                                                                                        \
  } while (0)

template <lf::scheduler Sch>
auto thread_count(benchmark::State &state) -> std::size_t {
  if constexpr (std::constructible_from<Sch, std::size_t>) {
    return static_cast<std::size_t>(state.range(1));
  } else {
    return 1;
  }
}

template <lf::scheduler Sch>
auto make_scheduler(benchmark::State &state) -> Sch {
  if constexpr (std::constructible_from<Sch, std::size_t>) {
    return Sch{static_cast<std::size_t>(state.range(1))};
  } else {
    return Sch{};
  }
}

using mono_busy_pool = lf::mono_busy_pool<lf::geometric_stack<>>;
using poly_busy_pool = lf::poly_busy_pool<lf::geometric_stack<>>;

template <typename T>
concept pool_with_stats = requires (T const &t) {
  { t.steal_count() } -> std::same_as<std::uint64_t>;
};

template <lf::scheduler Sch>
void record_stats(benchmark::State &state, Sch const &scheduler, std::uint64_t total_tasks) {
  state.counters["tasks"] = static_cast<double>(total_tasks);
  if constexpr (pool_with_stats<Sch>) {
    auto iters = static_cast<double>(state.iterations());
    auto avg_steals = static_cast<double>(scheduler.steal_count()) / iters;
    state.counters["steals"] = avg_steals;
    state.counters["steal_frac"] = avg_steals / static_cast<double>(total_tasks);
  }
}
