#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../bench.hpp"

using namespace std;

auto dfs(size_t depth, size_t breadth, unsigned long* sum) -> void {
  if (depth == 0) {
    *sum = 1;
    return;
  }

  vector<unsigned long> sums(breadth);

  for (size_t i = 0; i < breadth - 1; ++i) {
    dfs(depth - 1, breadth, &sums[i]);
  }
  dfs(depth - 1, breadth, &sums.back());

  *sum = 0;
  for (size_t i = 0; i < breadth; ++i) {
    *sum += sums[i];
  }
}

void run(std::string name, size_t depth, size_t breadth) {
  benchmark(name, [&](std::size_t num_threads, auto&& bench) {
    // Set up
    unsigned long answer;

    bench([&] {
      unsigned long tmp = 0;

      dfs(depth, breadth, &tmp);

      answer = tmp;
    });

    return answer;
  });
}

int main(int argc, char* argv[]) {
  run("serial-dfs-3,3", 3, 3);
  run("serial-dfs-5,5", 5, 5);
  run("serial-dfs-6,6", 5, 6);
  run("serial-dfs-7,7", 7, 7);
  return 0;
}