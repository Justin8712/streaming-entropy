/*Uses "Fast" implemenation of the entropy-estimation algorithm presented in
 *"A Near-Optimal Algorithm for Computing the Entropy of a Stream.
 *
 *Runs estimation algorithm on all possible combinations of the parameters 
 *in the length array and zipf array, outputting results in two files:
 *timevlength and timevzipf. These two files contain the same information,
 *but the results are ordered differently so it's easier to isolate the
 *desired independent variable in any graphs a user creates*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "massdal.h"
#include "entropypub.h"
#include "slowentropypub.h"
#include "naivepub.h"
#include "util.h"

#define INVALID_TOKEN INT_MIN
//should be defined in "math.h"
//#define M_E 2.71828183
#define MAX_WAIT 900000000
#define MB 1048576 
#define L_MAX 6
#define Z_MAX 7
//tokens in stream created by CreateStream will be in range [1, N+1]
#define RANGE_DEFAULT 99999
#define EPS_DEFAULT 1.0
#define DELTA_DEFAULT 1.0

typedef struct entry{
  double exact_ent;
  double estimated_ent;
  int time;
  int space;
} entry;

int length[L_MAX]= {100000, 500000, MB, 3*MB, 5*MB, 10*MB}; 
double zipf[Z_MAX]={1.001, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0};

static int * CreateStream(int the_length, entry* the_entry, double zipfpar, int range);
static void CheckArguments(int argc, char **argv, int*, int*, int*, double*, double*, int*); 
static void Fast_Handle_stream(int* stream, int c, int k, int range, entry* the_entry);
static void Slow_Handle_stream(int* stream, int c, int k, int range, entry* the_entry);
static void Naive_Handle_stream(int* stream, int c, int k, int range, entry* the_entry);

int main(int argc, char **argv) 
{
  double eps = EPS_DEFAULT;
  double delta = DELTA_DEFAULT;
  int range = RANGE_DEFAULT;
  int fflag, sflag, nflag;
  
  CheckArguments(argc, argv, &fflag, &nflag, &sflag, &eps, &delta, &range);

  FILE* time_v_length = fopen("timevlength", "w");
  if(time_v_length == NULL)
  {
    fprintf(stderr, "cant open timevlength!");
	exit(1);
  }
  FILE* time_v_zipf = fopen("timevzipf", "w");
  if(time_v_zipf == NULL)
  {
    fprintf(stderr, "cant open timevzipf!");
	exit(1);
  }
  
  //print column titles
  fprintf(time_v_length, "zipf\tlength\ttime\tspace\texact_entropy\testimated_entropy\n");
  fprintf(time_v_zipf, "zipf\tlength\ttime\tspace\texact_entropy\testimated_entropy\n");
  
  entry** values = (entry**) malloc(L_MAX * sizeof(entry*));
  for(int i = 0; i < L_MAX; i++)
  {
    values[i] = (entry*) malloc(Z_MAX * sizeof(struct entry));
  }
  
  int c, k;
  int* stream;
  entry* cur_entry;

  if(fflag)
  {
    for(int z_index = 0; z_index<Z_MAX; z_index++)
    {
      for(int l_index = 0; l_index<L_MAX; l_index++)
	  {
	    fprintf(stderr, "starting new stream\n");
	    c = ceil(16 * 1/(eps*eps) * log(2/delta) * log(length[l_index] * M_E));
        k = ceil(7/eps);
	    cur_entry = &(values[l_index][z_index]);
	    stream = CreateStream(length[l_index], cur_entry, zipf[z_index], range);
	    Fast_Handle_stream(stream, c, k, length[l_index], cur_entry);
	    free(stream);
	  
	    //time_v_length results should be in groups of fixed zipfpars
	    fprintf(time_v_length, "%5f\t%d\t%d\t%d\t%5f\t%5f\n", zipf[z_index], length[l_index],
	                          cur_entry->time, cur_entry->space, cur_entry->exact_ent,
							  cur_entry->estimated_ent);
	  }
    }
  }
  if(nflag)
  {
    for(int z_index = 0; z_index<Z_MAX; z_index++)
    {
      for(int l_index = 0; l_index<L_MAX; l_index++)
	  {
	    fprintf(stderr, "starting new stream\n");
	    c = ceil(16 * 1/(eps*eps) * log(2/delta) * log(length[l_index] * M_E));
        k = ceil(7/eps);
	    cur_entry = &(values[l_index][z_index]);
	    stream = CreateStream(length[l_index], cur_entry, zipf[z_index], range);
	    Naive_Handle_stream(stream, c, k, length[l_index], cur_entry);
	    free(stream);
	  
	    //time_v_length results should be in groups of fixed zipfpars
	    fprintf(time_v_length, "%5f\t%d\t%d\t%d\t%5f\t%5f\n", zipf[z_index], length[l_index],
	                          cur_entry->time, cur_entry->space, cur_entry->exact_ent,
							  cur_entry->estimated_ent);
	  }
    }
  }
  if(sflag)
  {
    for(int z_index = 0; z_index<Z_MAX; z_index++)
    {
      for(int l_index = 0; l_index<L_MAX; l_index++)
	  {
	    fprintf(stderr, "starting new stream\n");
	    c = ceil(16 * 1/(eps*eps) * log(2/delta) * log(length[l_index] * M_E));
        k = ceil(7/eps);
	    cur_entry = &(values[l_index][z_index]);
	    stream = CreateStream(length[l_index], cur_entry, zipf[z_index], range);
	    Slow_Handle_stream(stream, c, k, length[l_index], cur_entry);
	    free(stream);
	  
	    //time_v_length results should be in groups of fixed zipfpars
	    fprintf(time_v_length, "%5f\t%d\t%d\t%d\t%5f\t%5f\n", zipf[z_index], length[l_index],
	                          cur_entry->time, cur_entry->space, cur_entry->exact_ent,
							  cur_entry->estimated_ent);
	  }
    }
  }
  //finally output time_v_zipf
  for(int l_index = 0; l_index<L_MAX; l_index++)
  {
      for(int z_index = 0; z_index<Z_MAX; z_index++)
	{
	   cur_entry = &(values[l_index][z_index]);
	  	  
	  //time_v_zipf results should be in groups of fixed zipfpars
	  fprintf(time_v_zipf, "%5f\t%d\t%d\t%d\t%5f\t%5f\n", zipf[z_index], length[l_index], 
							  cur_entry->time, cur_entry->space, cur_entry->exact_ent,
							  cur_entry->estimated_ent);
	}
  }
  fclose(time_v_length);
  fclose(time_v_zipf);

  return 0;
}

//compute entropy ofstream w/ c samplers and
// k counters for use by Misra-Gries alg
//uses fast implementation of algorithm
static void Fast_Handle_stream(int* stream, int c, int k, int length, entry* the_entry)
{
  Estimator_type* est = Estimator_Init(c, k);  
  
  StartTheClock();
  for(int i = 0; i < length; i++)
  {
	Estimator_Update(est, stream[i]);
  }
  the_entry->time = StopTheClock();
  the_entry->space = Estimator_Size(est);
  //reached end of stream
  the_entry->estimated_ent= Estimator_end_stream(est);
  Estimator_Destroy(est);
}

//compute entropy ofstream w/ c samplers and
// k counters for use by Misra-Gries alg
//uses naive implementation of algorithm
static void Naive_Handle_stream(int* stream, int c, int k, int length, entry* the_entry)
{
  Naive_Estimator_type* est = Naive_Estimator_Init(c, k);  
  
  StartTheClock();
  for(int i = 0; i < length; i++)
  {
	Naive_Estimator_Update(est, stream[i]);
  }
  the_entry->time = StopTheClock();
  the_entry->space = Naive_Estimator_Size(est);
  //reached end of stream
  the_entry->estimated_ent= Naive_Estimator_end_stream(est);
  Naive_Estimator_Destroy(est);
}

//compute entropy ofstream using slow version of algorithm
//w/ c samplers and k counters for use by Misra-Gries alg
static void Slow_Handle_stream(int* stream, int c, int k, int length, entry* the_entry)
{
  Slow_Estimator_type* est = Slow_Estimator_Init(c, k);  
  
  StartTheClock();
  for(int i = 0; i < length; i++)
  {
	Slow_Estimator_Update(est, stream[i]);
  }
  the_entry->time = StopTheClock();
  the_entry->space = Slow_Estimator_Size(est);
  //reached end of stream
  the_entry->estimated_ent= Slow_Estimator_end_stream(est);
  Slow_Estimator_Destroy(est);
}

/******************************************************************/
//Creates and returns stream of ints in range [1, N+1] according to
//zipfian distribution based on zipfpar. Also computes exact entropy
//of stream and saves it in the_entry->exact_ent
int* CreateStream(int the_length, entry* the_entry, double zipfpar, int range)
{
  float zet;
  int i; 
  int* stream;
  prng_type * prng;
  double entropy, p;
  int* exact = (int*)calloc(range+2, sizeof(int));
  
  stream=(int *) calloc(the_length+1,sizeof(int));
      
  prng=prng_Init(44545,2);


  zet=zeta(the_length,zipfpar);

  for (i=1;i<=the_length;i++) 
  {
	stream[i]=(int) floor(fastzipf(zipfpar,range,zet,prng));
	//fprintf(stderr, "stream[%d] is %d, N is %d\n", i, stream[i], N); 
	exact[ stream[i] ]++;
  }
  
  entropy = 0;
  for(i=0; i<=range+1; i++)
  {
    p = (double) exact[i]/(double)the_length;
	if(p > 0)
	{
	  entropy+= p * -1 * log(p)/log(2);
    }
  }
  
  the_entry->exact_ent= entropy; 
  free(exact);

  prng_Destroy(prng);

  return(stream);

}

//reads command line arguments to set one of fflag, nflag, and sflag to 1
//as specified and the other two as 0 (default: fflag set to 1). Also 
//sets eps and delta if specified. Does error checking on command line args
static void CheckArguments(int argc, char **argv, int* fflag, int* nflag,
					int* sflag, double* eps, double* delta, int* range) {
  
  int next;
  //set defaults
  *fflag = *nflag = *sflag = 0;
  *eps = *delta = .1;
	  
  while ((next = getopt (argc, argv, "fnse:d:")) != -1)
  {     
	switch (next)
	{
	  case 'f':
	    //fprintf(stderr, "f\n");
		*fflag = 1;
	    break;
	  case 'n':
	    //fprintf(stderr, "n\n");
	    *nflag = 1;
		break;
	  case 's':
	   //fprintf(stderr, "s\n");
		*sflag = 1;
	    break;
	  case 'e':
	    //fprintf(stderr, "e\n");
	    *eps= strtod(optarg, (char **) NULL);
		if(errno == ERANGE) {
		  fprintf(stderr, "overflow or underflow in specifying epsilon\n");
		  exit(1);
		}
		if(*eps <= 0 || *eps > 1){
		  fprintf(stderr, "eps <= 0 or > 1\n");
		  exit(1);
		}
		break;  
	  case 'd':
	    //fprintf(stderr, "d\n");
		*delta= strtod(optarg, (char **) NULL);
		if(errno == ERANGE) {
		  fprintf(stderr, "overflow or underflow in specifying delta\n");
		  exit(1);
		}
		if(*delta <= 0 || *delta > 1){
		  fprintf(stderr, "delta < 0 or > 1\n");
		  exit(1);
		}
		break;  
	  case 'r':
	   *range = (int)strtol(optarg, (char **)NULL, 10);
	   if(*range <= 0){
		 fprintf(stderr, "error occurred in reading range of stream ");
		 fprintf(stderr, "or negative range given\n");
		 exit(1);
	   }
	   break; 
   	  case '?':
	   if (isprint (optopt))
		fprintf (stderr, "Unknown option `-%c'.\n", optopt);
		else
		 fprintf (stderr, "Unknown option character `\\x%x'.\n",
                        optopt);
		exit(1);
		break;
	 default:
	   abort();
    }
  }
  
  //default to fast version if no version specified
  if(!*fflag && !*nflag && !*sflag)
    *fflag = 1;
	
  if(*fflag + *nflag + *sflag > 1){
    fprintf(stderr, "two or more of versions (-f, -n, -s) specified\n");
	exit(1);
  }
}


