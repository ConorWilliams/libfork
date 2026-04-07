#ifndef A0179FFF_4078_4EEB_BB6E_1E8C75CC694C
#define A0179FFF_4078_4EEB_BB6E_1E8C75CC694C
/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *
 *  Copyright (c) 2010 See AUTHORS file for copyright holders
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 */

#ifndef _UTS_H
  #define _UTS_H

  #ifdef __cplusplus
extern "C" {
  #endif

  #include "rng/rng.h"

  #define UTS_VERSION "2.1"

  /***********************************************************
   *  Tree node descriptor and statistics                    *
   ***********************************************************/

  #define MAXNUMCHILDREN 100 // cap on children (BIN root is exempt)

struct node_t {
  int type;        // distribution governing number of children
  int height;      // depth of this node in the tree
  int numChildren; // number of children, -1 => not yet determined

  /* for RNG state associated with this node */
  struct state_t state;
};

typedef struct node_t Node;

/* Tree type
 *   Trees are generated using a Galton-Watson process, in
 *   which the branching factor of each node is a random
 *   variable.
 *
 *   The random variable can follow a binomial distribution
 *   or a geometric distribution.  Hybrid tree are
 *   generated with geometric distributions near the
 *   root and binomial distributions towards the leaves.
 */
enum uts_trees_e { BIN = 0, GEO, HYBRID, BALANCED };
enum uts_geoshape_e { LINEAR = 0, EXPDEC, CYCLIC, FIXED };

typedef enum uts_trees_e tree_t;
typedef enum uts_geoshape_e geoshape_t;

/* Strings for the above enums */
extern char *uts_trees_str[];
extern char *uts_geoshapes_str[];

/* Tree  parameters */
extern tree_t type;
extern double b_0;
extern int rootId;
extern int nonLeafBF;
extern double nonLeafProb;
extern int gen_mx;
extern geoshape_t shape_fn;
extern double shiftDepth;

/* Benchmark parameters */
extern int computeGranularity;
extern int debug;
extern int verbose;

/* For stats generation: */
typedef unsigned long long counter_t;

  /* Utility Functions */
  #define max(a, b) (((a) > (b)) ? (a) : (b))
  #define min(a, b) (((a) < (b)) ? (a) : (b))

void uts_error(char *str);
void uts_parseParams(int argc, char **argv);
int uts_paramsToStr(char *strBuf, int ind);
void uts_printParams();
void uts_helpMessage();

void uts_showStats(
    int nPes, int chunkSize, double walltime, counter_t nNodes, counter_t nLeaves, counter_t maxDepth);
double uts_wctime();

double rng_toProb(int n);

/* Common tree routines */
void uts_initRoot(Node *root, int type);
int uts_numChildren(Node *parent);
int uts_numChildren_bin(Node *parent);
int uts_numChildren_geo(Node *parent);
int uts_childType(Node *parent);

/* Implementation Specific Functions */
char *impl_getName();
int impl_paramsToStr(char *strBuf, int ind);
int impl_parseParam(char *param, char *value);
void impl_helpMessage();
void impl_abort(int err);

  #ifdef __cplusplus
}
  #endif

#endif /* _UTS_H */

#endif /* A0179FFF_4078_4EEB_BB6E_1E8C75CC694C */
