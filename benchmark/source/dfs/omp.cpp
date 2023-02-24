#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <omp.h>

#include <nanobench.h>

using namespace std;
using namespace chrono;

static void dfs(size_t depth, size_t breadth, unsigned long* sum) {
  if (depth == 0) {
    *sum = 1;
    return;
  }

  vector<unsigned long> sums(breadth);

  for (size_t i = 0; i < breadth; ++i) {
    auto s = &sums[i];
#pragma omp task shared(depth, breadth, s)
    dfs(depth - 1, breadth, s);
  }

#pragma omp taskwait

  *sum = 0;
  for (size_t i = 0; i < breadth; ++i)
    *sum += sums[i];
}

static void test(size_t depth, size_t breadth, unsigned long* sum) {
#pragma omp task shared(depth, breadth, sum)
  dfs(depth, breadth, sum);
#pragma omp taskwait
}

void omp_dfs(ankerl::nanobench::Bench& bench, std::size_t depth, std::size_t breadth, std::size_t nthreads) {
  //
  unsigned long answer;

  auto start = system_clock::now();

  omp_set_dynamic(0);
  omp_set_num_threads(nthreads);

  bench.run("busy_pool " + std::to_string(nthreads) + " threads", [&] {

#pragma omp parallel shared(depth, breadth, answer) num_threads(nthreads)
#pragma omp single
    test(depth, breadth, &answer);

    ankerl::nanobench::doNotOptimizeAway(answer);
  });
}