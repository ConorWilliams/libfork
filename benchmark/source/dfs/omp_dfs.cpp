#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <omp.h>

using namespace std;
using namespace chrono;

void dfs(size_t depth, size_t breadth, unsigned long* sum) {
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

void test(size_t depth, size_t breadth, unsigned long* sum) {
#pragma omp task shared(depth, breadth, sum)
  dfs(depth, breadth, sum);
#pragma omp taskwait
}

int main(int argc, char* argv[]) {
  size_t depth = 8;
  size_t breadth = 8;
  unsigned long answer;
  size_t nthreads = 0;

  if (argc >= 2)
    nthreads = atoi(argv[1]);
  if (argc >= 3)
    depth = atoi(argv[2]);
  if (argc >= 4)
    breadth = atoi(argv[3]);
  if (nthreads == 0)
    nthreads = thread::hardware_concurrency();

  auto start = system_clock::now();

  omp_set_dynamic(0);
  omp_set_num_threads(nthreads);

#pragma omp parallel shared(depth, breadth, answer)
#pragma omp single
  test(depth, breadth, &answer);

  auto stop = system_clock::now();

  cout << "Scheduler:  omp\n";
  cout << "Benchmark:  dfs\n";
  cout << "Threads:    " << nthreads << "\n";
  cout << "Time(us):   " << duration_cast<microseconds>(stop - start).count() << "\n";
  cout << "Input:      " << depth << " " << breadth << "\n";
  cout << "Output:     " << answer << "\n";

  return 0;
}