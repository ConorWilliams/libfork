#include <benchmark/benchmark.h>

#include "nqueens.hpp"

#include "helpers.hpp"

import std;

import libfork;

namespace {

using board_t = std::array<char, nqueens_answers.size()>;

struct nqueens_task {
  template <lf::worker_context Context>
  static auto operator()(lf::env<Context>, int row, int n, board_t const *board)
      -> lf::task<std::int64_t, Context> {

    if (row == n) {
      co_return 1;
    }

    std::vector<board_t> boards;
    boards.reserve(static_cast<std::size_t>(n));

    for (int col = 0; col < n; ++col) {
      board_t next = *board;
      next[static_cast<std::size_t>(row)] = static_cast<char>(col);
      if (queens_ok(row + 1, next.data())) {
        boards.push_back(next);
      }
    }

    if (boards.empty()) {
      co_return 0;
    }

    std::vector<std::int64_t> counts(boards.size());
    auto sc = co_await lf::scope();

    for (std::size_t i = 0; i < boards.size(); ++i) {
      if (i + 1 == boards.size()) {
        co_await sc.call(&counts[i], nqueens_task{}, row + 1, n, &boards[i]);
      } else {
        co_await sc.fork(&counts[i], nqueens_task{}, row + 1, n, &boards[i]);
      }
    }

    co_await sc.join();

    std::int64_t total = 0;
    for (auto count : counts) {
      total += count;
    }
    co_return total;
  }
};

template <lf::scheduler Sch>
void run(benchmark::State &state) {
  auto threads = static_cast<std::int64_t>(thread_count<Sch>(state));
  Sch scheduler = make_scheduler<Sch>(state);
  lf_bench::report_threads(state, threads);

  run_nqueens(state, [&](int n, char *) {
    board_t board{};
    return lf::schedule(scheduler, nqueens_task{}, 0, n, &board).get();
  });
}

} // namespace

LIBFORK_BENCH_ALL_MT(run, nqueens, nqueens, mono_busy_pool)
LIBFORK_BENCH_ALL_MT(run, nqueens, nqueens, poly_busy_pool)
