#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../bench.hpp"

#include "libfork/schedule/busy_pool.hpp"
#include "libfork/task.hpp"

using namespace std;
using namespace lf;

template <context Context>
auto dfs(size_t depth, size_t breadth, unsigned long* sum) -> basic_task<void, Context> {
  if (depth == 0) {
    *sum = 1;
    co_return;
  }

  vector<unsigned long> sums(breadth);

  for (size_t i = 0; i < breadth - 1; ++i) {
    co_await dfs<Context>(depth - 1, breadth, &sums[i]).fork();
  }
  co_await dfs<Context>(depth - 1, breadth, &sums.back());

  co_await join();

  *sum = 0;
  for (size_t i = 0; i < breadth; ++i) {
    *sum += sums[i];
  }
}

void run(std::string name, size_t depth = 8, size_t breadth = 8) {
  benchmark(name, [&](std::size_t num_threads, auto&& bench) {
    // Set up
    unsigned long answer;

    auto pool = busy_pool{num_threads};

    bench([&] {
      unsigned long tmp = 0;

      pool.schedule(dfs<lf::busy_pool::context>(depth, breadth, &tmp));

      answer = tmp;
    });

    return answer;
  });
}

int main(int argc, char* argv[]) {
  run("libfork, dfs 3,3", 3, 3);
  run("libfork, dfs 5,5", 5, 5);
  run("libfork, dfs 6,6", 5, 6);
  run("libfork, dfs 7,7", 7, 7);
  return 0;
}