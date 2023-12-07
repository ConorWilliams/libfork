#include <benchmark/benchmark.h>

#include <libfork.hpp>

#include <chrono>
#include <thread>

namespace {

LF_NOINLINE constexpr auto sleep() -> void { std::this_thread::sleep_for(std::chrono::milliseconds(500)); };

void calibrate(benchmark::State &state) {

  state.counters["green_threads"] = 1;

  for (auto _ : state) {
    sleep();
  }
}

} // namespace

BENCHMARK(calibrate)->UseRealTime();
