#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <numeric>
#include <ranges>

#include "libfork/core.hpp"

#include <benchmark/benchmark.h>

#include <tbb/blocked_range.h>
#include <tbb/global_control.h>
#include <tbb/parallel_scan.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>

#include "../util.hpp"
#include "config.hpp"

namespace {

LF_NOINLINE void test(tbb::task_arena& arena, std::vector<unsigned> &in, std::vector<unsigned> &ou) {

  auto body =  [&](auto r, unsigned sum, bool is_final_scan) {
    unsigned temp = sum;
    for(auto i=r.begin(); i<r.end(); ++i ) {        
        temp = temp + in[i];        
        if(is_final_scan) {
          ou[i] = temp;
        }
    }
    return temp;
  };

  arena.execute([&] {    
    for (std::size_t i = 0; i < scan_reps; ++i) { 
      tbb::parallel_scan(tbb::blocked_range(0UL, scan_n, scan_chunk), 0UL, body, std::plus<>());    
    }
  });
}

void scan_tbb(benchmark::State &state) {

  // TBB uses (2MB) stacks by default
  tbb::global_control global_limit(tbb::global_control::thread_stack_size, 8 * 1024 * 1024);

  state.counters["green_threads"] = static_cast<double>(state.range(0));
  state.counters["n"] = scan_n;
  state.counters["reps"] = scan_reps;
  state.counters["chunk"] = scan_chunk;

  std::size_t n = state.range(0);
  tbb::task_arena arena(n);

  volatile int sink = 0;

  std::vector in = make_vec();
  std::vector ou = std::vector<unsigned>(in.size());

  // in = std::vector<unsigned>{1, 1, 1, 1};
  // ou = in;

  for (auto _ : state) {
   test(arena, in, ou);
  }

  // for (auto && elem : ou ){
  //   std::cout << elem << std::endl;
  // }

  sink = ou.back();
}

} // namespace

BENCHMARK(scan_tbb)->Apply(targs)->UseRealTime();
