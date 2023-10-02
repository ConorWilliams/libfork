#ifndef DD1EB460_E250_4A8C_B35B_C99A803E5301
#define DD1EB460_E250_4A8C_B35B_C99A803E5301

#include <stdexcept>

#include "external/uts.h"

struct result {
  counter_t maxdepth, size, leaves;
};

struct pair {
  result res;
  Node child;
};

inline void reset_uts() {
  type = GEO;                // t
  b_0 = 4.0;                 // b
  rootId = 0;                // r
  nonLeafBF = 4;             // m
  nonLeafProb = 15.0 / 64.0; // q
  gen_mx = 6;                // d
  shape_fn = LINEAR;         // a
  shiftDepth = 0.5;          // f
  computeGranularity = 1;    // g
  debug = 0;                 // x
  verbose = 1;               // v
}

// # (T1) Geometric [fixed] ------- Tree size = 4130071, tree depth = 10, num leaves = 3305118 (80.03%)
// export T1="-t 1 -a 3 -d 10 -b 4 -r 19"
inline void setup_t1() {
  reset_uts();

  type = (tree_t)1;
  shape_fn = (geoshape_t)3;
  gen_mx = 10;
  b_0 = 4;
  rootId = 19;
}

// # (T2) Geometric [cyclic] ------ Tree size = 4117769, tree depth = 81, num leaves = 2342762 (56.89%)
// export T2="-t 1 -a 2 -d 16 -b 6 -r 502"
inline void setup_t2() {
  reset_uts();

  type = (tree_t)1;
  shape_fn = (geoshape_t)2;
  gen_mx = 16;
  b_0 = 6;
  rootId = 502;
}

// # (T3) Binomial ---------------- Tree size = 4112897, tree depth = 1572, num leaves = 3599034 (87.51%)
// export T3="-t 0 -b 2000 -q 0.124875 -m 8 -r 42"
inline void setup_t3() {
  reset_uts();

  type = (tree_t)0;
  b_0 = 2000;
  nonLeafBF = 8;
  nonLeafProb = 0.124875;
  rootId = 42;
}

// # (T4) Hybrid ------------------ Tree size = 4132453, tree depth = 134, num leaves = 3108986 (75.23%)
// export T4="-t 2 -a 0 -d 16 -b 6 -r 1 -q 0.234375 -m 4 -r 1"
inline void setup_t4() {
  reset_uts();

  type = (tree_t)2;
  shape_fn = (geoshape_t)0;
  gen_mx = 16;
  b_0 = 6;
  rootId = 1;
  nonLeafBF = 4;
  nonLeafProb = 0.234375;
}

// # (T5) Geometric [linear dec.] - Tree size = 4147582, tree depth = 20, num leaves = 2181318 (52.59%)
// export T5="-t 1 -a 0 -d 20 -b 4 -r 34"
inline void setup_t5() {
  reset_uts();

  type = (tree_t)1;
  shape_fn = (geoshape_t)0;
  gen_mx = 20;
  b_0 = 4;
  rootId = 34;
}

// # (T1L) Geometric [fixed] ------ Tree size = 102181082, tree depth = 13, num leaves = 81746377 (80.00%)
// export T1L="-t 1 -a 3 -d 13 -b 4 -r 29"
inline void setup_t1l() {
  reset_uts();

  type = (tree_t)1;
  shape_fn = (geoshape_t)3;
  gen_mx = 13;
  b_0 = 4;
  rootId = 29;
}

// # (T2L) Geometric [cyclic] ----- Tree size = 96793510, tree depth = 67, num leaves = 53791152 (55.57%)
// export T2L="-t 1 -a 2 -d 23 -b 7 -r 220"
inline void setup_t2l() {
  reset_uts();

  type = (tree_t)1;
  shape_fn = (geoshape_t)2;
  gen_mx = 23;
  b_0 = 7;
  rootId = 220;
}

// # (T3L) Binomial --------------- Tree size = 111345631, tree depth = 17844, num leaves = 89076904 (80.00%)
// export T3L="-t 0 -b 2000 -q 0.200014 -m 5 -r 7"
inline void setup_t3l() {
  reset_uts();

  type = (tree_t)0;
  b_0 = 2000;
  nonLeafBF = 5;
  nonLeafProb = 0.200014;
  rootId = 7;
}

// # (T3XXL) Binomial ------------- Tree size = 2793220501
// export T3XXL="-t 0 -b 2000 -q 0.499995 -m 2 -r 316"
inline void setup_t3xxl() {
  reset_uts();

  type = (tree_t)0;
  b_0 = 2000;
  nonLeafBF = 2;
  nonLeafProb = 0.499995;
  rootId = 316;
}

inline void setup_tree(int i) {
  switch (i) {
    case 1:
      setup_t1();
      break;
    case 2:
      setup_t2();
      break;
    case 3:
      setup_t3();
      break;
    case 4:
      setup_t4();
      break;
    case 5:
      setup_t5();
      break;
    case 6:
      setup_t1l();
      break;
    case 7:
      setup_t2l();
      break;
    case 8:
      setup_t3l();
      break;
    case 9:
      setup_t3xxl();
      break;
    default:
      throw std::runtime_error("Invalid tree id");
      break;
  }
}

#define MAKE_UTS_FOR(some_type)                                                                              \
  BENCHMARK_CAPTURE(some_type, "T1  Geo fixed ", 1)->DenseRange(1, num_threads())->UseRealTime();            \
  BENCHMARK_CAPTURE(some_type, "T2  Geo cycle ", 2)->DenseRange(1, num_threads())->UseRealTime();            \
  BENCHMARK_CAPTURE(some_type, "T3  Binomial  ", 3)->DenseRange(1, num_threads())->UseRealTime();            \
  BENCHMARK_CAPTURE(some_type, "T4  Hybrid    ", 4)->DenseRange(1, num_threads())->UseRealTime();            \
  BENCHMARK_CAPTURE(some_type, "T5  Geo linear", 5)->DenseRange(1, num_threads())->UseRealTime();            \
  BENCHMARK_CAPTURE(some_type, "T1L Geo fixed ", 6)->DenseRange(1, num_threads())->UseRealTime();            \
  BENCHMARK_CAPTURE(some_type, "T2L Geo cyclic", 7)->DenseRange(1, num_threads())->UseRealTime();            \
  BENCHMARK_CAPTURE(some_type, "T3L Binomial  ", 8)->DenseRange(1, num_threads())->UseRealTime()

#endif /* DD1EB460_E250_4A8C_B35B_C99A803E5301 */
