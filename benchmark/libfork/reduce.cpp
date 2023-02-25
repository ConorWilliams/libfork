#include <numeric>
#include <span>
#include <vector>

#include "../bench.hpp"

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

using namespace lf;

template <context Context>
auto reduce(std::span<unsigned int const> x, std::size_t grain_size) -> basic_task<int, Context> {
  //
  if (x.size() <= grain_size) {
    co_return std::reduce(x.begin(), x.end());
  }

  auto h = x.size() / 2;
  auto t = x.size() - h;

  auto a = co_await reduce<Context>(x.first(h), grain_size).fork();
  auto b = co_await reduce<Context>(x.last(t), grain_size);

  co_await join();

  co_return *a + b;
}

void run(std::string name, std::span<unsigned int> data) {
  auto correct = std::reduce(data.begin(), data.end());

  benchmark(name, [&](std::size_t n, auto&& bench) {
    // Set up
    auto pool = busy_pool{n};

    unsigned int res = 0;

    bench([&] {
      res = pool.schedule(reduce<busy_pool::context>(data, data.size() / (64)));
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

  run("libfork, reduce 1'000", std::span{data}.first(1'000));
  run("libfork, reduce 10'000", std::span{data}.first(10'000));
  run("libfork, reduce 100'000", std::span{data}.first(100'000));
  run("libfork, reduce 1'000'000", std::span{data}.first(1'000'000));
  run("libfork, reduce 10'000'000", std::span{data}.first(10'000'000));
  run("libfork, reduce 100'000'000", std::span{data}.first(100'000'000));

  return 0;
}