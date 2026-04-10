#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "libfork/__impl/exception.hpp"

import std;

import libfork;

// ============================================================
//  Task-tree specification
// ============================================================

namespace {

// Describes a single node in a randomly generated task tree.
//
// cancel_mode — how the PARENT creates this node:
//   Inherit : scope::call/fork(fn)  — child shares parent's cancel token
//   New     : scope::call/fork(&tok, fn) — child gets a fresh isolated token
//
// child_kind — how THIS node creates ITS children:
//   Call : sequential scope::call, no explicit join
//   Fork : parallel  scope::fork + co_await join()
//
// signal_cancel — stop my own cancel token BEFORE spawning children.
//
// is_leaf() — true when children is empty.  Leaf tasks increment the counter.
//
// Internal Fork nodes also increment the counter AFTER co_await join() (only
// if the join did not cascade due to cancellation).  This lets the fuzz test
// verify cascade behaviour independently of leaf counts.

struct NodeSpec {
  enum class CancelMode : std::uint8_t { Inherit, New };
  enum class ChildKind : std::uint8_t { Call, Fork };

  CancelMode cancel_mode = CancelMode::Inherit;
  ChildKind child_kind = ChildKind::Call;
  bool signal_cancel = false;
  std::vector<NodeSpec> children;

  [[nodiscard]]
  bool is_leaf() const noexcept { return children.empty(); }
};

// ============================================================
//  Random tree generation
// ============================================================

NodeSpec gen_node(std::mt19937 &rng, int depth, int max_depth) {
  NodeSpec n;

  std::bernoulli_distribution coin{0.5};
  std::bernoulli_distribution rare{0.2};
  std::bernoulli_distribution very_rare{0.08};

  // Leaves: at max depth or by chance
  if (depth >= max_depth || rare(rng)) {
    return n; // leaf (no children set)
  }

  n.cancel_mode = coin(rng) ? NodeSpec::CancelMode::Inherit : NodeSpec::CancelMode::New;
  n.child_kind = coin(rng) ? NodeSpec::ChildKind::Fork : NodeSpec::ChildKind::Call;
  n.signal_cancel = very_rare(rng);

  std::uniform_int_distribution<int> nc_dist{1, 4};
  int nc = nc_dist(rng);
  n.children.reserve(static_cast<std::size_t>(nc));
  for (int i = 0; i < nc; ++i) {
    n.children.push_back(gen_node(rng, depth + 1, max_depth));
  }
  return n;
}

// ============================================================
//  Reference simulation (inline-scheduler semantics)
//
//  simulate() returns the expected counter value contributed by this subtree.
//
//  Precondition: the parent's token was NOT stopped when we were created
//  (the caller already verified this with its own stopped[] check).
//
//  stopped[id] tracks whether token `id` has been stop-requested.
//  next_id is the allocator for fresh token ids.
//
//  Token semantics:
//    Inherit → child shares my_tok (same id)
//    New     → child gets a new id, initially false, with no link to my_tok
//
//  inline-scheduler execution order (sequential, depth-first):
//    For Fork mode: each child runs inline before parent resumes.
//      After child i completes, stopped[my_tok] is re-checked before child i+1
//      (this is the await_transform check in the parent).
//    For Call mode: identical ordering, no explicit join.
//    Fork cascade: if stopped[my_tok] is true after all children, the
//      post-join counter increment is suppressed.
// ============================================================

int simulate(const NodeSpec &spec, int my_tok, std::vector<bool> &stopped, int &next_id) {

  if (spec.signal_cancel) {
    stopped[static_cast<std::size_t>(my_tok)] = true;
  }

  if (spec.is_leaf()) {
    return 1; // leaf increments counter
  }

  int total = 0;
  bool is_fork = (spec.child_kind == NodeSpec::ChildKind::Fork);

  for (const auto &child : spec.children) {
    // await_transform in parent: if my token stopped, skip this child
    if (stopped[static_cast<std::size_t>(my_tok)]) {
      break;
    }

    // Allocate the child's token
    int child_tok;
    if (child.cancel_mode == NodeSpec::CancelMode::Inherit) {
      child_tok = my_tok; // shared
    } else {
      child_tok = next_id++;
      stopped.push_back(false);
    }

    total += simulate(child, child_tok, stopped, next_id);
    // After child returns, stopped[my_tok] might be true if child (or a
    // descendant) stopped an inherited token.  The next loop iteration's
    // stopped[] check handles this correctly.
  }

  if (is_fork) {
    // Post-join: runs only if join did NOT cascade (my_tok not stopped)
    if (!stopped[static_cast<std::size_t>(my_tok)]) {
      ++total;
    }
  }
  // Call mode: no post-join observable (call always returns to parent)

  return total;
}

int expected_count(const NodeSpec &root) {
  // The root always gets a fresh token (provided by fuzz_root below)
  std::vector<bool> stopped{false}; // id 0 = root token, initially not stopped
  int next_id = 1;
  return simulate(root, 0, stopped, next_id);
}

// ============================================================
//  libfork execution
// ============================================================

using lf::cancellation;
using lf::env;
using lf::task;

// Forward declaration: execute_node is mutually recursive with itself.
template <typename Context>
auto execute_node(env<Context>, const NodeSpec *, cancellation *, std::atomic<int> *)
    -> task<void, Context>;

// Root wrapper: schedule() always creates the root task with cancel=nullptr.
// We give the root node its own fresh token so the fuzz spec can signal it.
template <typename Context>
auto fuzz_root(env<Context>, const NodeSpec *spec, std::atomic<int> *counter)
    -> task<void, Context> {
  cancellation root_tok;
  using S = lf::scope<Context>;
  co_await S::call(&root_tok, execute_node<Context>, spec, &root_tok, counter);
  co_return;
}

// Interprets a NodeSpec subtree as libfork coroutines.
//
// my_tok — the cancel token this task was bound to (either inherited from parent
//           or a fresh token created by the parent for this node).  This matches
//           self.frame.cancel for the Inherit case, and the explicit token for
//           the New case.
//
// Counter semantics (must match reference simulation):
//   Leaf                        → counter++ unconditionally
//   Internal Fork, no cascade   → counter++ after co_await join()
//   Internal Fork, cascade      → post-join code not reached (frame destroyed)
//   Internal Call               → no counter increment (pure structure)
template <typename Context>
auto execute_node(env<Context>, const NodeSpec *spec, cancellation *my_tok,
                  std::atomic<int> *counter) -> task<void, Context> {
  using S = lf::scope<Context>;

  // Signal before children (matches reference: stopped[my_tok]=true before loop)
  if (spec->signal_cancel) {
    my_tok->request_stop();
  }

  if (spec->is_leaf()) {
    counter->fetch_add(1, std::memory_order_relaxed);
    co_return;
  }

  // Heap-allocate tokens for New-mode children.  Lifetime: this coroutine
  // frame, which outlives all children (they complete before or at the join).
  auto child_toks = std::make_unique<cancellation[]>(spec->children.size());

  if (spec->child_kind == NodeSpec::ChildKind::Fork) {

    for (std::size_t i = 0; i < spec->children.size(); ++i) {
      const auto &ch = spec->children[i];
      // await_transform checks self.frame.is_cancelled() before creating child.
      // If my_tok is stopped the awaitable returns {.child=nullptr} and the
      // co_await resumes the parent immediately (same as a no-op).  The loop
      // continues but all remaining co_awaits also short-circuit.
      if (ch.cancel_mode == NodeSpec::CancelMode::Inherit) {
        // Child inherits my_tok: no explicit cancel arg
        co_await S::fork(execute_node<Context>, &ch, my_tok, counter);
      } else {
        // Child gets fresh isolated token
        co_await S::fork(&child_toks[i], execute_node<Context>, &ch, &child_toks[i], counter);
      }
    }

    co_await lf::join();

    // Post-join: counter increment only if we reach here (join did not cascade)
    counter->fetch_add(1, std::memory_order_relaxed);

  } else { // Call mode

    for (std::size_t i = 0; i < spec->children.size(); ++i) {
      const auto &ch = spec->children[i];
      if (ch.cancel_mode == NodeSpec::CancelMode::Inherit) {
        co_await S::call(execute_node<Context>, &ch, my_tok, counter);
      } else {
        co_await S::call(&child_toks[i], execute_node<Context>, &ch, &child_toks[i], counter);
      }
    }
    // No post-join for Call (no explicit join point, call always returns)
  }

  co_return;
}

// ============================================================
//  Fuzz runner
// ============================================================

template <typename Sch>
void run_fuzz(Sch &scheduler, std::mt19937 &rng, int n_trees, int max_depth) {
  using Ctx = lf::context_t<Sch>;

  for (int t = 0; t < n_trees; ++t) {
    NodeSpec root = gen_node(rng, 0, max_depth);

    int expected = expected_count(root);

    std::atomic<int> counter{0};
    auto recv = lf::schedule(scheduler, fuzz_root<Ctx>, &root, &counter);
    REQUIRE(recv.valid());
    std::move(recv).get();

    int actual = counter.load();
    if (actual != expected) {
      // Helpful failure message
      FAIL("counter mismatch: expected " << expected << " got " << actual
                                         << " (tree depth " << max_depth
                                         << ", iteration " << t << ")");
    }
  }
}

} // namespace

// ============================================================
//  Test cases
// ============================================================

using mono_inline_ctx = lf::mono_context<lf::geometric_stack<>, lf::adapt_vector>;
using poly_inline_ctx = lf::derived_poly_context<lf::geometric_stack<>, lf::adapt_vector>;

// The fuzz test uses the inline scheduler because:
// - The reference simulation is sequential/deterministic (inline semantics)
// - With a thread pool, cancel races make the expected count ambiguous
// The per-scheduler cancel semantics are already covered by cancel.cpp.
TEMPLATE_TEST_CASE("Cancellation fuzz: random task trees (inline)", "[cancel][fuzz]",
                   mono_inline_ctx, poly_inline_ctx) {

  lf::inline_scheduler<TestType> scheduler;

  SECTION("fixed seed, shallow trees (reproducible)") {
    std::mt19937 rng{0xDEAD'BEEF};
    run_fuzz(scheduler, rng, 2000, 4);
  }

  SECTION("random seed, deeper trees") {
    std::mt19937 rng{std::random_device{}()};
    run_fuzz(scheduler, rng, 500, 6);
  }
}
