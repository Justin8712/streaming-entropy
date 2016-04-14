/*slowentropy.c
 *"Slow" implemenation of the entropy-estimation algorithm presented in
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
#include "prng.h"
#include "massdal.h"
#include "slowentropypriv.h"

#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))
#define INVALID_TOKEN INT_MIN
//#define M_E 2.71828183 //should be defined in "math.h"

static Sample_type * Sample_Init(int seed)
{
  Sample_type * sm;
  
  sm=(Sample_type *) malloc(sizeof(Sample_type));
  if(sm)
  {
    sm->s0=sm->r0=sm->s1=sm->r1=0;
	sm->t0=sm->t1=INT_MAX; 
	//set t0 and t1 to INT_MAX so any random int in [m^3] will be smaller
  }
  return sm;
}

//initialize estimator with c samplers and k counters (used by Misra-Gries alg)
Slow_Estimator_type * Slow_Estimator_Init(int c, int k)
{
  Slow_Estimator_type* est = (Slow_Estimator_type*) malloc(sizeof(Slow_Estimator_type));
  est->c=c;
  est->k=k;
  est->count = 0;
  est->freq=Freq_Init((float)1.0/k);
  est->samplers = (Sample_type**) malloc(sizeof(Sample_type*) * c);
  
  est->prng=prng_Init(drand48(), 2); 
  // initialize the random number generator
	
  int i;
  for(i = 0; i < c; i++)
  {
    est->samplers[i]=Sample_Init(i);
  }
  return est;
}

static void Sample_Destroy(Sample_type * sm)
{
  if(!sm) return;
  free(sm);
}

void Slow_Estimator_Destroy(Slow_Estimator_type * est)
{
  prng_Destroy(est->prng);
  free(est->freq);
  int i;
  for(i=0; i < est->c; i++)
  {
    Sample_Destroy(est->samplers[i]);
  }
  free(est->samplers);
  free(est);
}

// return the size of the estimator in bytes
int Slow_Estimator_Size(Slow_Estimator_type * est)
{
 //include size of random number generator?
  int samplers, admin, freq;
  if (!est) return 0;
  admin=sizeof(Slow_Estimator_type);
  samplers = est->c*sizeof(Sample_type);
  freq = Freq_Size(est->freq);
  //note Freq_Size just a placeholder function at the moment
  return(admin + samplers + freq);
}

//update a sampler 
//(called for each sampler each time an item from the stream is read)
static void Sample_Update(Sample_type * sm, prng_type* prng, int token)
{
  long rand = prng_int(prng) & MOD;
  if(token == sm->s0)
  {
    if(rand < sm->t0)
	{
	  sm->t0=rand;
	  sm->r0=1;
	}
	else sm->r0++;
  }
  else
  {
    if(token == sm->s1)
	{
	  sm->r1 ++;
	}
  
    if(rand<sm->t0)
    {
      sm->s1=sm->s0;
	  sm->t1=sm->t0;
	  sm->r1=sm->r0; 
	
  	  sm->s0=token;
	  sm->t0=rand;
	  sm->r0=1;
    }
    else 
    {
      if(rand < sm->t1)
      {
	    sm->s1=token;
	    sm->t1=rand;
	    sm->r1=1;
	  }
    }
  }
}

//process a new token read from the stream
void Slow_Estimator_Update(Slow_Estimator_type * est, int token)
{
  est->count++;
  Freq_Update(est->freq, token);//end of Misra-Gries part of algorithm
  
  //update all c versions of Algorithm Maintain_Samples
  for(int i=0; i < est->c; i++)
  {
	Sample_Update(est->samplers[i], est->prng, token);
  }
}

//end of stream reached. Compute estimate for entropy  
double Slow_Estimator_end_stream(Slow_Estimator_type* est)
{
  int max_count, max_token, r, m;
  double p_max, sum_Xis, avg_Xis;
  
  max_count = sum_Xis = 0;
  max_token= INVALID_TOKEN;
  m = est->count;
  
  int i;	
  //find maximum value retained by Misra-Gries algorithm
  SaveMax(est->freq, &max_token, &max_count);
	
  if(max_count > (int) (m/2))//truncate m/2 in comparison
  {
	p_max = (double) max_count/m;
	for(i=0; i < est->c; i++)
	{
	  if(est->samplers[i]->s0 == max_token)
		r = est->samplers[i]->r1;
	  else
		r=est->samplers[i]->r0;
	  if(r!=0)//treat rlog(m/r) as 0 if r=0 (there was only 1 token in stream)
	  {
	    sum_Xis += (double) r * log10((double) m/r)/log10(2);
	  }
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
	  r=est->samplers[i]->r0;
	  if(r!=0) //ignore empty stream
	    sum_Xis += r * log10((double) m/r)/log10(2);
	  if(r > 1) //treat (r-1)log(m/(r-1)) as 0 if r=1, also ignore r=0
		sum_Xis -= (r-1) * log10((double) m/(r-1))/log10(2);
	}
	return sum_Xis /est->c;  
  }	  
}