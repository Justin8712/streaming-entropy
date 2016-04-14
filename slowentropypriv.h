#ifndef SLOWENTROPYPRIV_H
#define SLOWENTROPYPRIV_H

#include "slowentropypub.h"

#define minimum(x,y)	((x) < (y) ? (x) : (y))
#define maximum(x,y)	((x) > (y) ? (x) : (y))

typedef struct Sample_type{
  unsigned long s0, r0, t0, s1, r1, t1;
} Sample_type;

struct Slow_Estimator_type{
  //need array of c Sample_types for sampling and array of k coutners for Misra-Gries
  //plus parallel array to track which token each of the k counters is tracking
  int c, k, count;
  prng_type* prng;
  Sample_type** samplers;  
  freq_type* freq;
};

static Sample_type * Sample_Init(int seed);
static void Sample_Destroy(Sample_type * sm);
static void Sample_Update(Sample_type * sm, prng_type* prng, int token);


#endif

