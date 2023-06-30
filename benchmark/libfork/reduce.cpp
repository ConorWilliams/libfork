#include <numeric>
#include <span>
#include <vector>

#include "../bench.hpp"

#include "libfork/core.hpp"
#include "libfork/schedule/busy.hpp"

using namespace lf;

inline constexpr lf::async_fn reduce = [](auto reduce, std::span<unsigned int const> x, std::size_t grain_size) -> task<unsigned int> {
  if (x.size() <= grain_size) {
    co_return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  unsigned int a, b;

  co_await lf::fork(a, reduce)(x.first(h), grain_size);
  co_await lf::call(b, reduce)(x.last(t), grain_size);

  co_await join;

  co_return a + b;
};

void run(std::string name, std::span<unsigned int> data) {
  //
  auto correct = std::reduce(data.begin(), data.end());

  benchmark(name, [&](std::size_t n, auto &&bench) {
    // Set up
    auto pool = lf::busy_pool{n};

    int res = 0;

    bench([&] {
      res = lf::sync_wait(pool, reduce, data, data.size() / (10 * n));
    });

    if (res != correct) {
      throw std::runtime_error("Incorrect result");
    }

    return res;
  });
}

auto main() -> int {
  std::vector<unsigned int> data(100'000'000);

  std::iota(data.begin(), data.end(), 0);

  run("fork-reduce-1_000", std::span{data}.first(1'000));
  run("fork-reduce-10_000", std::span{data}.first(10'000));
  run("fork-reduce-100_000", std::span{data}.first(100'000));
  run("fork-reduce-1_000_000", std::span{data}.first(1'000'000));
  run("fork-reduce-10_000_000", std::span{data}.first(10'000'000));
  run("fork-reduce-100_000_000", std::span{data}.first(100'000'000));

  return 0;
}