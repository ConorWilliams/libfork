#ifndef DD1EB460_E250_4A8C_B35B_C99A803E5301
#define DD1EB460_E250_4A8C_B35B_C99A803E5301

#include "external/uts.h"

struct result {
  counter_t maxdepth, size, leaves;
};

struct pair {
  result res;
  Node child;
};

inline void setup_uts() {
  // This is the T3L="-t 0 -b 2000 -q 0.200014 -m 5 -r 7"
  nonLeafProb /* q */ = 0.200014;
  nonLeafBF /* m */ = 5;
  rootId /* r */ = 7;
  type /* t */ = (tree_t)0;
  b_0 /* b */ = 2000;

  // This is the T3="-t 0 -b 2000 -q 0.124875 -m 8 -r 42"
  // nonLeafProb /* q */ = 0.124875;
  // nonLeafBF /* m */ = 8;
  // rootId /* r */ = 42;
  // type /* t */ = (tree_t)0;
  // b_0 /* b */ = 2000;
}

#endif /* DD1EB460_E250_4A8C_B35B_C99A803E5301 */
