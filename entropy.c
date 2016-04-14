/* entropy.c
 *"Fast" implemenation of the entropy-estimation algorithm presented in
 *"A Near-Optimal Algorithm for Computing the Entropy of a Stream." 
 *
 *This program is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation; either version 3 of the License, or
 *any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *Email:  justin.thaler@yale.edu
 *
 *July 7, 2007
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "massdal.h"
#include "entropypriv.h"
#include "util.h"

#define INVALID_TOKEN INT_MIN
//this constant should be defined in math.h
//#define M_E 2.71828183
#define MAX_WAIT 90000000

static int b_cmp(c_a* p, c_a* q);
static int prim_cmp(void* p, void* q);

static int b_cmp(c_a* p, c_a* q)
{
  long a = p->count + 
      peek_min_c_a_heap(p->sample_heap)->backup_minus_delay;
  long b = q->count + 
      peek_min_c_a_heap(q->sample_heap)->backup_minus_delay;
  return (a==b) ? 0 : (a<b) ? -1 : 1;
}

static int prim_cmp(void* p, void* q)
{
  int a = ((Sample_type*) p)->prim;
  int b = ((Sample_type*) q)->prim;
  return (a==b) ? 0 : (a<b) ? -1 : 1;
}

Sample_type * Sample_Init()
{
  Sample_type * sm;
  
  sm=(Sample_type *) safe_malloc(sizeof(Sample_type));
  sm->val_c_s0=sm->val_c_s1=0;
  sm->t0=sm->t1 = 1;
  sm->c_s0_pos = -1;
  sm->prim = sm->backup_minus_delay = 0; 
  sm->c_s0 = sm->c_s1 = NULL;
  return sm;
}

//initialize estimator with c samplers and k counters (used by Misra-Gries alg)
Estimator_type * Estimator_Init(int c, int k)
{
  Estimator_type* est = (Estimator_type*) safe_malloc(sizeof(Estimator_type));
  est->c=c;
  est->k=k;
  est->count = 0;
  est->two_distinct_tokens=0;
  est->prng=prng_Init(drand48(), 2); 
  // initialize the random number generator
  est->freq=Freq_Init((float)1.0/k);
	
  est->samplers = (Sample_type**) safe_malloc(sizeof(Sample_type*) * c);	
  for(int i = 0; i < c; i++)
  {
    est->samplers[i]=Sample_Init();
  }
  
  est->hashtable=new_symtab(2*c);
  est->prim_heap = new_heap(prim_cmp, c);
  est->bheap = new_bheap(b_cmp, c);  
  return est;
}

void Sample_Destroy(Sample_type * sm)
{
  if(!sm) return;
  free(sm);
}

void Estimator_Destroy(Estimator_type * est)
{
  prng_Destroy(est->prng);
  for(int i=0; i < est->c; i++)
  {
    Sample_Destroy(est->samplers[i]);
  }
  free(est->samplers);
  Freq_Destroy(est->freq);
  free_bheap(est->bheap);
  free_heap(est->prim_heap);
  free_symtab(est->hashtable);
  free(est);
}

// return the size of the estimator in bytes
int Estimator_Size(Estimator_type * est)
{
 //include size of random number generator?
  int freq, samplers, admin, hash, prim, backup;
  if (!est) return 0;
  admin=sizeof(Estimator_type);
  freq=Freq_Size(est->freq);
  //note Freq_Size just a placeholder function at the moment
  samplers = est->c*sizeof(Sample_type);
  hash = sizeof_symtab(est->hashtable);
  prim = sizeof_heap(est->prim_heap);
  backup = sizeof_bheap(est->bheap);
  return(admin + samplers + freq + hash + prim + backup);
}

//called by Estimator_Update to handle first token in stream
//slightly more efficient than just using handle_nondistinct
void handle_first(Estimator_type* est, c_a* first)
{
  for(int i = 0; i < est->c; i++)
  {
    est->samplers[i]->c_s0=first;
	est->samplers[i]->val_c_s0=1;  
	est->samplers[i]->t0=prng_float(est->prng);
  }
}

//handles the case when we're reading the kth token where k > 1 and the
//first k tokens have all been identical 
void handle_nondistinct(Estimator_type* est, c_a* token)
{
  double r;
  Sample_type* cur;
  for(int i = 0; i < est->c; i++)
  {
    cur = est->samplers[i];
	r = prng_float(est->prng);
	if(r < cur->t0)
	{
	  cur->t0 = r;
	  cur->val_c_s0 = token->count;
	}
  }
}

//handles token k+1 when the first k tokens are all the same
void handle_second_distinct(Estimator_type* est, c_a* token)
{
  double r;
  Sample_type* cur;
  est->two_distinct_tokens = 1;
  for(int i = 0; i < est->c; i++)
  {
    cur = est->samplers[i];
	r = prng_float(est->prng);
	if(r < cur->t0)
	{
	  cur->val_c_s1 = cur->val_c_s0;
	  cur->c_s1 = cur->c_s0;
	  cur->t1 = cur->t0;
	  
	  cur->val_c_s0 = 1;
	  cur->c_s0=token;
	  cur->t0=r;
	}
	else
	{
	  cur->val_c_s1 = 1;
	  cur->c_s1 = token;
	  cur->t1 = r;
	}
	reset_wait_times(cur, est);
	//must reset wait times before inserting into prim heap or
	//incrementing prim samplers (because increment_prim_samplers() handles
	//insertion/restoring heap property in c_s0's heap and est's bheapneeds 
	//and hence needs the wait times to be set properly as precondition
	insert_heap(est->prim_heap, cur);
	increment_prim_samplers(cur->c_s0, est->bheap, cur);
	increment_backup_samplers(cur->c_s1);
  }
}

//recalculates both cur's primary and backup sample wait times
//drawing from geometric distributions
void reset_wait_times(Sample_type* cur, Estimator_type* est)
{
  double r0 = prng_float(est->prng);
  double r1 = prng_float(est->prng);
  int wait;
	
  //resample prim_wait_time from geometric distribution with p=t0
  //special case if r0==0 since log(0) undef
  if(r0 == 0) cur->prim = est->count+1;
  else if(cur->t0 == 0)
  { //t0 == 0 should cause longest possible wait time
	cur->prim = MAX_WAIT;
  }
  else
	cur->prim = ceil(log(r0)/log(1-cur->t0)) + est->count; 
	 
  if(cur->prim < 0 || cur->prim > MAX_WAIT) //check for overflow
	cur->prim = MAX_WAIT;
	  	  
  //nuance: if r0 == t0 == 0, however unlikely this is, we
  //should technically use more random bits to determine if cur->prim
	
  //resample backup wait time from geometric distribution with p=t1-t0 
  //special case if r1==0 since log(0) undef
  if(r1 == 0) cur->backup_minus_delay = est->count + 1 - cur->c_s0->count;
  else
  {
	if(cur->t1-cur->t0 == 0)
	{ //t0 == t1 should cause longest possible wait time
	  cur->backup_minus_delay = MAX_WAIT-cur->c_s0->count;
	}
	else
	{
	  wait = ceil(log(r1)/log(1.0-(cur->t1-cur->t0)));
								
	  if(wait < 0 || wait > MAX_WAIT) //check for overflow
	    cur->backup_minus_delay = MAX_WAIT-cur->c_s0->count;
	  else
	    cur->backup_minus_delay = wait + est->count - cur->c_s0->count;
    }
  }
}

//process a new token read from the stream
void Estimator_Update(Estimator_type * est, int token)
{
  int old_cs0pos, old_backupminuswait, wait;
  est->count++;
  
  Freq_Update(est->freq, token);
  //end of Misra-Gries part of algorithm

  //In the case that a sampler is scheduled to take a new backup and
  //primary sample at the same time, we should use more random bits to
  //break the tie. But for now, for simplicity, we'll break all such
  //ties by having the sampler take a new *primary* sample

  //increment count of token, sets processing to 1
  c_a* counter = increment_count(est->hashtable, token);
  
  //check for special cases
  if(est->count == 1)
  {
    est->first = counter;
    handle_first(est, counter);
	return;
  }
  if(counter->count == est->count)
  { 
    handle_nondistinct(est, counter);
	return;
  }
  if(est->two_distinct_tokens == 0)
  {
    handle_second_distinct(est, counter);
	//indicate that we are done for the time being with two
	//distinct tokens in the stream so they can be removed from
	//the hashtable if no samplers are sampling them
	done_processing(est->hashtable, counter);
	done_processing(est->hashtable, est->first);
	return;
  }
  
  //only restore heap prop if samplers have been put in bheap
  restore_bheap_property(est->bheap, counter->backup_pos);
  
  Sample_type* min;
  c_a* old_c_s1 = NULL;

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
	//have min take a new primary sample
	if(min->c_s0 == counter)
	{
	  min->val_c_s0 = counter->count;
	  min->t0 *= prng_float(est->prng);
	  //resample primary and backup wait times using new values of t0 and t1
	  reset_wait_times(min, est);
	  restore_c_a_heap_property(min->c_s0->sample_heap, min->c_s0_pos);
	  restore_bheap_property(est->bheap, min->c_s0->backup_pos);
	}
	else
	{
	  old_c_s1 = min->c_s1;
	  min->c_s1 = min->c_s0;
	  min->val_c_s1 = min->val_c_s0;
	  min->t1 = min->t0;
	  min->c_s0 = counter;
	  min->val_c_s0 = counter->count;
	  min->t0 *= prng_float(est->prng);
	  
	  //resample primary and backup wait times using new values of t0 and t1
	  old_cs0pos = min->c_s0_pos;
	  old_backupminuswait = min->backup_minus_delay;
	  reset_wait_times(min, est);
	  
	  //increment backup samplers for c_s1 first, b/c if we decremented 
	  //prim samplers first and min was the only primary sampler of c_s1 and 
	  //c_s1 had no backup samplers, then c_s1 would be removed from the hashtable
	  //which we don't want. Note increment_backup_samplers does *not* change
	  //min->c_s0_pos, so the subsequent call to decrement_prim_samplers will work fine
	  //when it tries to remove min from c_s1's heap of samplers
	  increment_backup_samplers(min->c_s1);
	  decrement_backup_samplers(est->hashtable, old_c_s1);
	  decrement_prim_samplers(est->hashtable, min->c_s1, est->bheap, min);	  
	  increment_prim_samplers(counter, est->bheap, min);
	}
	//reinsert min into primary heap
	insert_heap(est->prim_heap, min);
  }
	
  c_a* min2 = peek_min_bheap(est->bheap);
  min = peek_min_c_a_heap(min2->sample_heap);

  double r1;
  while(min->backup_minus_delay + min2->count <= est->count)
  {
	if(min->backup_minus_delay + min2->count < est->count)
	{ //error check
	  fprintf(stderr, "error: sampler's backup wait time decreased\n");
	  fprintf(stderr, "bminusd %d, min2->count %d est->count %d\n", 
	          min->backup_minus_delay, min2->count, est->count);
	  exit(1);
	}

	decrement_backup_samplers(est->hashtable, min->c_s1);
	increment_backup_samplers(counter);
	min->t1 -= prng_float(est->prng) * (min->t1-min->t0);
	min->c_s1 = counter;
	min->val_c_s1 = counter->count;
	
	//recalculate just min's backup wait time
	r1 = prng_float(est->prng);
	if(r1 == 0) min->backup_minus_delay = est->count + 1 - min->c_s0->count;
	else
	{
	  if(min->t1-min->t0 == 0)
	  { //t0 == t1 should cause longest possible wait time
		min->backup_minus_delay = MAX_WAIT-min->c_s0->count;
	  }
	  else
	  {
	    wait = ceil(log(r1)/log(1.0-(min->t1-min->t0)));
								
	    if(wait < 0 || wait > MAX_WAIT) //check for overflow
	      min->backup_minus_delay = MAX_WAIT-min->c_s0->count;
	    else
	      min->backup_minus_delay = wait + est->count - min->c_s0->count;
	  }
	}
	//fprintf(stderr, "%d ", min->backup_minus_delay);

	//put min in proper position in its primary sample's heap
	restore_c_a_heap_property(min->c_s0->sample_heap, min->c_s0_pos);

	//put min's primary sample in proper position in backup heap
	restore_bheap_property(est->bheap, min->c_s0->backup_pos);
	
	min2 = peek_min_bheap(est->bheap);
    min = peek_min_c_a_heap(min2->sample_heap);
  }	
  done_processing(est->hashtable, counter);
}

//end of stream reached. Compute estimate for entropy  
double Estimator_end_stream(Estimator_type* est)
{
  int max_count, r, m, i;
  int max_token;
  double p_max, sum_Xis, avg_Xis;
  
  max_count = sum_Xis = 0;
  m = est->count;
  
  if(est->count == 0 || est->two_distinct_tokens == 0) 
  { //empty stream or only one character in stream
    return 0;
  }
  
  SaveMax(est->freq, &max_token, &max_count);
	
  if(max_count > (int) (m/2))
  {
	p_max = (double) max_count/m;
	for(i=0; i < est->c; i++)
	{
	  if(est->samplers[i]->c_s0->key == max_token)
	  {
		r = est->samplers[i]->c_s1->count-
		    est->samplers[i]->val_c_s1+1;
	  }		
	  else
	  {
		r=est->samplers[i]->c_s0->count-
		  est->samplers[i]->val_c_s0+1;
	  }
	  
	  sum_Xis += (double) r * log10((double) m/r)/log10(2);
	  if(r > 1) //treat (r-1)log(m/(r-1)) as 0 if r=1, also ignore r=0 
	  {
	    sum_Xis -= (double) (r-1) * log10((double) m/(r-1))/log10(2);
	  }
	}
	avg_Xis = sum_Xis / est->c;
	return (1-p_max) * avg_Xis + p_max * log10(1/p_max)/log10(2); 
  }
  else
  {
	for(i=0; i < est->c; i++)
	{
	  r=est->samplers[i]->c_s0->count-
		est->samplers[i]->val_c_s0+1;
	  if(r!=0) //ignore empty stream
	    sum_Xis += r * log10((double) m/r)/log10(2);
	  if(r > 1) //treat (r-1)log(m/(r-1)) as 0 if r=1, also ignore r=0
		sum_Xis -= (r-1) * log10((double) m/(r-1))/log10(2);
	}
	return sum_Xis /est->c;  
  }	  
}