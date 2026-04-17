#pragma once

#include <benchmark/benchmark.h>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

struct incorrect_result : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

inline constexpr unsigned bench_max_threads = 24;

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
