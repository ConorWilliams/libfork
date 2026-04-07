#include "libfork_benchmark/uts/uts.hpp"

#include "libfork/__impl/exception.hpp"

import std;

namespace {

void reset_uts() {
  type = GEO;
  b_0 = 4.0;
  rootId = 0;
  nonLeafBF = 4;
  nonLeafProb = 15.0 / 64.0;
  gen_mx = 6;
  shape_fn = LINEAR;
  shiftDepth = 0.5;
  computeGranularity = 1;
  debug = 0;
  verbose = 1;
}

// (T1 mini) Geometric [fixed] — gen_mx=7, same other params as T1
void setup_t1_mini() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 7;
  b_0 = 4;
  rootId = 19;
}

// (T1) Geometric [fixed] — size = 4130071, depth = 10, leaves = 3305118
void setup_t1() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 10;
  b_0 = 4;
  rootId = 19;
}

// (T1L) Geometric [fixed] — size = 102181082, depth = 13, leaves = 81746377
void setup_t1l() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 13;
  b_0 = 4;
  rootId = 29;
}

// (T1XXL) Geometric [fixed] — size = 4230646601, depth = 15
void setup_t1xxl() {
  reset_uts();
  type = static_cast<tree_t>(1);
  shape_fn = static_cast<geoshape_t>(3);
  gen_mx = 15;
  b_0 = 4;
  rootId = 19;
}

// (T3 mini) Binomial — b_0=20, same other params as T3
void setup_t3_mini() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 20;
  nonLeafBF = 8;
  nonLeafProb = 0.124875;
  rootId = 42;
}

// (T3) Binomial — size = 4112897, depth = 1572, leaves = 3599034
void setup_t3() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 2000;
  nonLeafBF = 8;
  nonLeafProb = 0.124875;
  rootId = 42;
}

// (T3L) Binomial — size = 111345631, depth = 17844, leaves = 89076904
void setup_t3l() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 2000;
  nonLeafBF = 5;
  nonLeafProb = 0.200014;
  rootId = 7;
}

// (T3XXL) Binomial — size = 2793220501
void setup_t3xxl() {
  reset_uts();
  type = static_cast<tree_t>(0);
  b_0 = 2000;
  nonLeafBF = 2;
  nonLeafProb = 0.499995;
  rootId = 316;
}

} // namespace

void setup_tree(uts_tree tree) {
  switch (tree) {
    case uts_t1_mini:
      setup_t1_mini();
      break;
    case uts_t1:
      setup_t1();
      break;
    case uts_t1l:
      setup_t1l();
      break;
    case uts_t1xxl:
      setup_t1xxl();
      break;
    case uts_t3_mini:
      setup_t3_mini();
      break;
    case uts_t3:
      setup_t3();
      break;
    case uts_t3l:
      setup_t3l();
      break;
    case uts_t3xxl:
      setup_t3xxl();
      break;
  }
}

auto expected_result(uts_tree tree) -> result {
  switch (tree) {
    case uts_t1_mini:
      return {.maxdepth = 7, .size = 63914, .leaves = 51124};
    case uts_t1:
      return {.maxdepth = 10, .size = 4130071, .leaves = 3305118};
    case uts_t1l:
      return {.maxdepth = 13, .size = 102181082, .leaves = 81746377};
    case uts_t1xxl:
      return {.maxdepth = 15, .size = 4230646601, .leaves = 3384495738};
    case uts_t3_mini:
      return {.maxdepth = 67, .size = 6213, .leaves = 5438};
    case uts_t3:
      return {.maxdepth = 1572, .size = 4112897, .leaves = 3599034};
    case uts_t3l:
      return {.maxdepth = 17844, .size = 111345631, .leaves = 89076904};
    case uts_t3xxl:
      return {.maxdepth = 99049, .size = 2793220501, .leaves = 1396611250};
    default:
      LF_THROW(std::invalid_argument("invalid tree type"));
  }
}
