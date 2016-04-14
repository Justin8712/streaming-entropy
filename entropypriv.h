
#ifndef ENTROPYPRIV_H
#define ENTROPYPRIV_H
#include "entropypub.h"
#include "frequent.h"
#include "symtab.h"
#include "heap.h"
#include "backup_heap.h"
#include "c_a_heap.h"
#include "prng.h"

#define minimum(x,y)	((x) < (y) ? (x) : (y))
#define maximum(x,y)	((x) > (y) ? (x) : (y))

struct Estimator_type{
  int c, k, two_distinct_tokens;
  int count;
  symtab* hashtable;
  prng_type* prng;
  Sample_type** samplers;
  freq_type* freq; //data structure for Misra-Gries algorithm
  heap* prim_heap;
  backup_heap* bheap;
  c_a* first;
};

extern Sample_type* Sample_Init();
extern void Sample_Destroy(Sample_type * sm);
extern void reset_wait_times(Sample_type* cur, Estimator_type* est);
extern void handle_nondistinct(Estimator_type* est, c_a* token);
extern void handle_second_distinct(Estimator_type* est, c_a* token);
extern void handle_first(Estimator_type* est, c_a* token);
extern void Sample_Update(Sample_type * sm, prng_type* prng, int token);

#endif
