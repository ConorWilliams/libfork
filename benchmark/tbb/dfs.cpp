#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../bench.hpp"

#include <tbb/task_arena.h>
#include <tbb/task_group.h>

using namespace std;

auto dfs(size_t depth, size_t breadth, unsigned long* sum) -> void {
  if (depth == 0) {
    *sum = 1;
    return;
  }

  vector<unsigned long> sums(breadth);

  tbb::task_group g;

  for (size_t i = 0; i < breadth - 1; ++i) {
    g.run([&] {
      dfs(depth - 1, breadth, &sums[i]);
    });
  }
  dfs(depth - 1, breadth, &sums.back());

  g.wait();

  *sum = 0;
  for (size_t i = 0; i < breadth; ++i) {
    *sum += sums[i];
  }
}

void run(std::string name, size_t depth, size_t breadth) {
  benchmark(name, [&](std::size_t num_threads, auto&& bench) {
    // Set up
    unsigned long answer;

    tbb::task_arena(num_threads).execute([&] {
      bench([&] {
        unsigned long tmp = 0;

        dfs(depth, breadth, &tmp);

        answer = tmp;
      });
    });

    return answer;
  });
}

int main(int argc, char* argv[]) {
  run("tbb, dfs 3,3", 3, 3);
  run("tbb, dfs 5,5", 5, 5);
  run("tbb, dfs 6,6", 5, 6);
  run("tbb, dfs 7,7", 7, 7);
  return 0;
}