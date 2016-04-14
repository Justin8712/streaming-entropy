/* naive.c
 *"Naive" implemenation of the entropy-estimation algorithm presented in
 *"A Near-Optimal Algorithm for Computing the Entropy of a Stream." Does
 *not keep backup samples*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "massdal.h"
#include "naivepriv.h"
#include "util.h"

#define INVALID_TOKEN INT_MIN
//#define M_E 2.71828183
#define MAX_WAIT 90000000

static int prim_cmp(void* p, void* q)
{
  int a = ((Sample_type*) p)->prim;
  int b = ((Sample_type*) q)->prim;
  return (a==b) ? 0 : (a<b) ? -1 : 1;
}

static Sample_type * Naive_Sample_Init()
{
  Sample_type * sm;
  
  sm=(Sample_type *) safe_malloc(sizeof(Sample_type));
  sm->val_c_s0=0;
  sm->t0 = 1;
  sm->prim = 0; 
  return sm;
}

//initialize estimator with c samplers and k counters (used by Misra-Gries alg)
Naive_Estimator_type * Naive_Estimator_Init(int c, int k)
{
  Naive_Estimator_type* est = (Naive_Estimator_type*) safe_malloc(sizeof(Naive_Estimator_type));
  est->c=c;
  est->k=k;
  est->count = 0;
  est->prng=prng_Init(drand48(), 2); 
  // initialize the random number generator
  est->freq=Freq_Init((float)1.0/k);
	
  est->samplers = (Sample_type**) safe_malloc(sizeof(Sample_type*) * c);	
  for(int i = 0; i < c; i++)
  {
    est->samplers[i]=Naive_Sample_Init();
  }
  
  est->hashtable=new_naivesymtab(c);
  est->prim_heap = new_heap(prim_cmp, c); 
  return est;
}

static void Naive_Sample_Destroy(Sample_type * sm)
{
  if(!sm) return;
  free(sm);
}

void Naive_Estimator_Destroy(Naive_Estimator_type * est)
{
  prng_Destroy(est->prng);
  for(int i=0; i < est->c; i++)
  {
    Naive_Sample_Destroy(est->samplers[i]);
  }
  free(est->samplers);
  Freq_Destroy(est->freq);
  free_heap(est->prim_heap);
  free_naivesymtab(est->hashtable);
  free(est);
}

// return the size of the estimator in bytes
int Naive_Estimator_Size(Naive_Estimator_type * est)
{
 //include size of random number generator?
  int freq, samplers, admin, hash, prim;
  if (!est) return 0;
  admin=sizeof(Naive_Estimator_type);
  freq=Freq_Size(est->freq);
  //note Freq_Size just a placeholder function at the moment
  samplers = est->c*sizeof(Sample_type);
  hash = sizeof_naivesymtab(est->hashtable);
  prim = sizeof_heap(est->prim_heap);
  return(admin + samplers + freq + hash + prim);
}

//called by Estimator_Update to handle first token in stream
//slightly more efficient than just using handle_nondistinct
static void naive_handle_first(Naive_Estimator_type* est, c_a* first)
{
  for(int i = 0; i < est->c; i++)
  {
    est->samplers[i]->c_s0=first;
	est->samplers[i]->val_c_s0=1;  
	est->samplers[i]->t0=prng_float(est->prng);
	naive_increment_prim_samplers(first);
	naive_reset_wait_times(est->samplers[i], est);
	insert_heap(est->prim_heap, est->samplers[i]);
  }
}

//recalculates cur's next item to sample
static void naive_reset_wait_times(Sample_type* cur, Naive_Estimator_type* est)
{
  double r0 = prng_float(est->prng);
  int wait;
	
  //resample prim_wait_time from geometric distribution with p=t0
  //special case if r0==0 since log(0) undef
  if(r0 == 0)
    cur->prim = est->count+1;
  else if(cur->t0 == 0)
  { //t0 == 0 should cause longest possible wait time
	cur->prim = MAX_WAIT;
  }
  else
  {
	wait = ceil(log(r0)/log(1-cur->t0)); 
	 
    if(wait < 0 || wait > MAX_WAIT) //check for overflow
	  cur->prim = MAX_WAIT;
    else cur->prim = wait + est->count;
  }  	  
  //nuance: if r0 == t0 == 0, however unlikely this is, we
  //should technically use more random bits to determine if cur->prim
}

//process a new token read from the stream
void Naive_Estimator_Update(Naive_Estimator_type * est, int token)
{
  est->count++;
  
  Freq_Update(est->freq, token);
  //end of Misra-Gries part of algorithm

  //increment count of token, sets processing to 1
  c_a* counter = naive_increment_count(est->hashtable, token);
  
  if(est->count == 1)
  {
    naive_handle_first(est, counter);
	return;
  }
  
  Sample_type* min;
  while(((Sample_type*) peek_min(est->prim_heap))->prim <= est->count)
  {
    min=delete_min(est->prim_heap);
	if(min->prim < est->count)
	{
	  fprintf(stderr, "a sampler's prim decreased. fatal error\n");
	  fprintf(stderr, "min->c_s0_key: %d, min->prim %d, est->count %d\n", 
	                   min->c_s0->key, min->prim, est->count);
	  exit(1);
	}
	naive_decrement_prim_samplers(est->hashtable, min->c_s0);
	naive_increment_prim_samplers(counter);
	//have min take a new sample
	min->c_s0 = counter;
	min->val_c_s0 = counter->count;
	min->t0 *= prng_float(est->prng);
	naive_reset_wait_times(min, est);
	//reinsert min into primary heap
	insert_heap(est->prim_heap, min);
  }
  naive_done_processing(est->hashtable, counter);
}

//end of stream reached. Compute estimate for entropy  
double Naive_Estimator_end_stream(Naive_Estimator_type* est)
{
  int r;
  double sum_Xis = 0;
  int m = est->count;
  
  for(int i=0; i < est->c; i++)
  {
	r=est->samplers[i]->c_s0->count-
	  est->samplers[i]->val_c_s0+1;
	
	if(r!=0)
	  sum_Xis += (double) r * log10((double) m/r)/log10(2);
	if(r > 1) //treat (r-1)log(m/(r-1)) as 0 if r=1, also ignore r=0
	  sum_Xis -= (double) (r-1) * log10((double) m/(r-1))/log10(2);
  }
  return sum_Xis /est->c;  
}