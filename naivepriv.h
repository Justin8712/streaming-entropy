
#ifndef NAIVEPRIV_H
#define NAIVEPRIV_H
#include "frequent.h"
#include "naivesymtab.h"
#include "heap.h"
#include "prng.h"
#include "naivepub.h"

typedef struct Sample_type{
  int val_c_s0;
  double t0;
  c_a* c_s0;
  int prim; //next item to sample
} Sample_type;

struct Naive_Estimator_type{
  int c, k, count;
  symtab* hashtable;
  prng_type* prng;
  Sample_type** samplers;
  freq_type* freq; //data structure for Misra-Gries algorithm
  heap* prim_heap;
};

static Sample_type* Naive_Sample_Init();
static void Naive_Sample_Destroy(Sample_type * sm);
static void naive_reset_wait_times(Sample_type* cur, Naive_Estimator_type* est);
static void naive_handle_first(Naive_Estimator_type* est, c_a* first);

#endif
