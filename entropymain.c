/*entropymain.c*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "massdal.h"
#include "entropypub.h"
#include "naivepub.h"
#include "slowentropypub.h"
#include "prng.h"
#include "util.h"

#define INVALID_TOKEN INT_MIN
//constant should be defined in "math.h"
//#define M_E 2.71828183
#define MAX_WAIT 900000000

#define C_DEFAULT 0
#define K_DEFAULT 0
#define LENGTH_DEFAULT 100000
#define RANGE_DEFAULT 99999
#define EPS_DEFAULT 1.0
#define DELTA_DEFAULT 1.0
#define BYTES_DEFAULT 1

static int* CreateStream(int length, double zipfpar, int range);
void CheckArguments(int argc, char **argv); 
static double Fast_Handle_stream(int* stream, int c, int k, int length);
static double Fast_Handle_file(char* filename, int c, int k, int bytes);
static double Naive_Handle_stream(int* stream, int c, int k, int length);
static double Naive_Handle_file(char* filename, int c, int k, int bytes);
static double Slow_Handle_stream(int* stream, int c, int k, int length);
static double Slow_Handle_file(char* filename, int c, int k, int bytes);


int main(int argc, char **argv) 
{
  CheckArguments(argc,argv);
  return 0;
}

//compute entropy ofstream w/ c samplers and
// k counters for use by Misra-Gries alg
//uses fast implementation of algorithm
static double Fast_Handle_stream(int* stream, int c, int k, int length)
{
  double entropy;
  Estimator_type* est = Estimator_Init(c, k);  
  
  StartTheClock();
  for(int i = 0; i < length; i++)
  {
	Estimator_Update(est, stream[i]);
  }
  //reached end of stream
  entropy = Estimator_end_stream(est);
  printf("took %ld ms and used %d bytes\n", 
         StopTheClock(), Estimator_Size(est));
  Estimator_Destroy(est);
  return entropy;
}

//compute the entropy of the stream contained in file file_name
//with c samplers and k counters used by Misra-Gries alg
//uses fast implementation of algorithm
static double Fast_Handle_file(char* file_name, int c, int k, int bytes)
{
  char buf[5];
  double entropy;
  int token;
  Estimator_type* est = Estimator_Init(c, k);  
  
  FILE* file = fopen(file_name, "r");
  if(!file)
  {
    fprintf(stderr, "Can't open file %s\n", file_name);
	exit(1);
  }
  
  StartTheClock();
  while(fgets(buf, bytes+1, file)!=NULL)
  {
    token = 0;
	if(!feof(file)) //if not at end of stream, add all the bytes
	{
	  for(int i = 0; i <= bytes-1 ; i++)
	  {
	    token += buf[i] << (8*i);
	  }
	}
	else
	{ //handle case where number of bytes in 
	  //file not divisible by parameter bytes
	  //assumes no bytes in stream are "0"
	  for(int i = 0; i <= bytes-1 ; i++)
	  {
	    if(buf[i] == 0) break;
	    token += buf[i] << (8*i);
	  }
	}
	Estimator_Update(est, token);
  }
  //reached end of stream
  entropy = Estimator_end_stream(est);
  printf("took %ld ms and used %d bytes\n", 
         StopTheClock(), Estimator_Size(est));

  Estimator_Destroy(est);
  fclose(file);
  return entropy;
}

//compute entropy ofstream w/ c samplers and
// k counters for use by Misra-Gries alg
//uses naive implementation of algorithm
static double Naive_Handle_stream(int* stream, int c, int k, int length)
{
  double entropy;
  Naive_Estimator_type* est = Naive_Estimator_Init(c, k);  
  
  StartTheClock();
  for(int i = 0; i < length; i++)
  {
	Naive_Estimator_Update(est, stream[i]);
  }
  //reached end of stream
  entropy = Naive_Estimator_end_stream(est);
  printf("took %ld ms and used %d bytes\n", 
         StopTheClock(), Naive_Estimator_Size(est));
  Naive_Estimator_Destroy(est);
  return entropy;
}

//compute the entropy of the stream contained in file file_name
//with c samplers and k counters used by Misra-Gries alg
//uses naive implementation of algorithm
static double Naive_Handle_file(char* file_name, int c, int k, int bytes)
{
  char buf[2];
  double entropy;
  int token;
  
  Naive_Estimator_type* est = Naive_Estimator_Init(c, k);  
  
  FILE* file = fopen(file_name, "r");
  if(!file)
  {
    fprintf(stderr, "Can't open file %s\n", file_name);
	exit(1);
  }
  
  StartTheClock();
  while(fgets(buf, bytes+1, file)!=NULL)
  {
    token = 0;
	if(!feof(file)) //if not at end of stream, add all the bytes
	{
	  for(int i = 0; i <= bytes-1 ; i++)
	  {
	    token += buf[i] << (8*i);
	  }
	}
	else
	{ //handle case where number of bytes in 
	  //file not divisible by parameter bytes
	  //assumes no bytes in stream are "0"
	  for(int i = 0; i <= bytes-1 ; i++)
	  {
	    if(buf[i] == 0) break;
	    token += buf[i] << (8*i);
	  }
	}
	Naive_Estimator_Update(est, token);
  }
  //reached end of stream
  entropy = Naive_Estimator_end_stream(est);
  printf("took %ld ms and used %d bytes\n", 
         StopTheClock(), Naive_Estimator_Size(est));
  Naive_Estimator_Destroy(est);
  fclose(file);
  return entropy;
}

//compute entropy ofstream w/ c samplers and
// k counters for use by Misra-Gries alg
static double Slow_Handle_stream(int* stream, int c, int k, int length)
{
  double entropy;
  Slow_Estimator_type* est = Slow_Estimator_Init(c, k);  
  
  StartTheClock();
  for(int i = 0; i < length; i++)
  {
	Slow_Estimator_Update(est, stream[i]);
  }
  //reached end of stream
  entropy = Slow_Estimator_end_stream(est);
  printf("took %ld ms and used %d bytes\n", 
         StopTheClock(), Slow_Estimator_Size(est));
  Slow_Estimator_Destroy(est);
  return entropy;
}


//compute the entropy of the stream contained in file file_name
//with c samplers and k counters used by Misra-Gries alg
static double Slow_Handle_file(char* file_name, int c, int k, int bytes)
{
  char buf[2];
  double entropy;
  int token;

  Slow_Estimator_type* est = Slow_Estimator_Init(c, k);  
  
  FILE* file = fopen(file_name, "r");
  if(!file)
  {
    fprintf(stderr, "Can't open file %s\n", file_name);
	exit(1);
  }
  
  StartTheClock();
  while(fgets(buf, bytes+1, file)!=NULL)
  {
    token = 0;
	if(!feof(file)) //if not at end of stream, add all the bytes
	{
	  for(int i = 0; i <= bytes-1 ; i++)
	  { 
	    token += buf[i] << (8*i);
	  }
	}
	else
	{ //handle case where number of bytes in 
	  //file not divisible by parameter bytes
	  //assumes no bytes in stream are "0"
	  for(int i = 0; i <= bytes-1 ; i++)
	  {
	    if(buf[i] == 0) break;
	    token += buf[i] << (8*i);
	  }
	}
	Slow_Estimator_Update(est, token);
  }
  //reached end of stream
  entropy = Slow_Estimator_end_stream(est);
  printf("took %ld ms and used %d bytes\n", 
         StopTheClock(), Slow_Estimator_Size(est));
  Slow_Estimator_Destroy(est);
  fclose(file);
  return entropy;
}

/******************************************************************/

void CheckArguments(int argc, char **argv) {
  int fflag, nflag, sflag, zflag;
  int mflag, eflag, dflag, cflag, kflag, lflag;
  int length, c, k, range, next, bytes;
  char* filename;
  double delta, eps, zipfparam;
  
  //set defaults
  fflag = nflag = sflag = zflag = 0;
  mflag = eflag = dflag = cflag = kflag = lflag = 0;
  
  length = LENGTH_DEFAULT;
  eps = EPS_DEFAULT;
  delta = DELTA_DEFAULT;
  c = C_DEFAULT;
  k = K_DEFAULT;
  range = RANGE_DEFAULT;
  bytes = BYTES_DEFAULT;
  filename = "";
  zipfparam = 1.1;
	  
  opterr = 0;	  
  while ((next = getopt (argc, argv, "fnsz:m:e:d:c:k:l:b:")) != -1)
  {     
	switch (next)
	{
	  case 'f':
	    //fprintf(stderr, "f\n");
		fflag = 1;
	    break;
	  case 'n':
	    //fprintf(stderr, "n\n");
	    nflag = 1;
		break;
	  case 's':
	   //fprintf(stderr, "s\n");
		sflag = 1;
	  break;
	  case 'z':
	    //fprintf(stderr, "z\n");
	    zflag = 1;
		//if zipfparam specified
		zipfparam = strtod(optarg, (char **) NULL);
		if(errno == ERANGE) {
		  fprintf(stderr, "overflow or underflow in zipfparameter\n");
		  exit(1);
		}
		if(zipfparam < 0){
		  fprintf(stderr, "zipfparam < 0\n");
		  exit(1);
		}
		break;   
	  case 'm':
	    mflag = 1;
	    filename = optarg;
		//fprintf(stderr, "m filename %s\n", filename);
		break;
	  case 'e':
	    //fprintf(stderr, "e\n");
	    eflag = 1;
		eps= strtod(optarg, (char **) NULL);
		if(errno == ERANGE) {
		  fprintf(stderr, "overflow or underflow in specifying epsilon\n");
		  exit(1);
		}
		if(eps <= 0 || eps > 1){
		  fprintf(stderr, "eps <= 0 or > 1\n");
		  exit(1);
		}
		break;  
	  case 'd':
	    //fprintf(stderr, "d\n");
	    dflag = 1;
		delta= strtod(optarg, (char **) NULL);
		if(errno == ERANGE) {
		  fprintf(stderr, "overflow or underflow in specifying delta\n");
		  exit(1);
		}
		if(delta <= 0 || delta > 1){
		  fprintf(stderr, "delta < 0 or > 1\n");
		  exit(1);
		}
		break;  
	  case 'c':
	    //fprintf(stderr, "c\n");
	    cflag = 1; 
		c = (int)strtol(optarg, (char **)NULL, 10);
	    if(c <= 0){
		  fprintf(stderr, "error occurred in reading c=num samplers\n");
		  exit(1);
		}
		break;
	  case 'k':
	    //fprintf(stderr, "k\n");
		kflag = 1; 
		k = (int)strtol(optarg, (char **)NULL, 10);
	    if(k <= 0){
		  fprintf(stderr, "error occurred in reading k=num counters for Misra-Gries\n");
		  exit(1);
		}
		break;
	 case 'l':
	   //fprintf(stderr, "l\n");
	   lflag = 1; 
	   length = (int)strtol(optarg, (char **)NULL, 10);
	   if(length <= 0){
	     fprintf(stderr, "error occurred in reading length of stream ");
		 fprintf(stderr, "or negative length given\n");
		 exit(1);
	   }
	   break;
	case 'r':
	   //fprintf(stderr, "r\n");
	   range = (int)strtol(optarg, (char **)NULL, 10);
	   if(range <= 0){
		 fprintf(stderr, "error occurred in reading range of stream ");
		 fprintf(stderr, "or negative range given\n");
		 exit(1);
	   }
	   break;  
	 case 'b':
	   bytes = (int)strtol(optarg, (char **)NULL, 10);
	   if(bytes <= 0 || bytes > 4){
		 fprintf(stderr, "Can't read %d bytes.", bytes);
		 fprintf(stderr, " Must be integer between 1 and 4 inclusive\n");
		 exit(1);
	   }
	   break; 
	 case '?':
	   if (isprint (optopt))
		fprintf (stderr, "Unknown option or required argument not provided `-%c'.\n", optopt);
		else
		 fprintf (stderr, "Unknown option character `\\x%x'.\n",
                        optopt);
		exit(1);
		break;
	 default:
	   abort ();
    }
  }
  
  //default to fast version if no version specified
  if(!fflag && !nflag && !sflag)
    fflag = 1;
 //default to synthetic stream if -z and -m not specified
 if(!zflag && !mflag)
   zflag = 1;

  //do some error checking
  if( mflag && zflag){
    fprintf(stderr, "can't choose to read from file (-m) ");
	fprintf(stderr, "and create synthetic stream (-z) at same time\n");
	exit(1);
  }
  if(fflag + nflag + sflag > 1){
    fprintf(stderr, "two or more of versions (-f, -n, -s) specified\n");
	exit(1);
  }
  
  if(mflag) //figure out length if we're reading from file
  {
	//determine filesize:
	FILE* the_file = fopen(filename, "r");
    if(the_file == NULL)
    {
        fprintf(stderr, "File pointer is null\n.");
        exit(1);
    }
	fseek(the_file, 0, SEEK_END);
	length = ftell(the_file);
	fclose(the_file);
  }
  //set values of c and k if not specified on command line
  if(!cflag)
	c = ceil(16 * 1/(eps*eps) * log(2/delta) * log(length * M_E));
  if(!kflag)
	k = ceil(7/eps);
  
  double answer;
  //phew, all errors should have been detected and all variables have correct values
  if(fflag) //use fast version
  { 
    if(mflag) //read from file
	{
      answer = Fast_Handle_file(filename, c, k, bytes);
      printf("Estimated entropy is: %lf\n", answer);
	  return;
	}
	else //create synthetic stream
	{
	  int* stream=CreateStream(length, zipfparam, range);
      answer = Fast_Handle_stream(stream, c, k, length);
  
      printf("Estimated entropy is: %f\n", answer);
      free(stream);
    }
  }
  if(nflag)
  {
	if(mflag) //read from file
	{
      answer = Naive_Handle_file(filename, c, k, bytes);
      printf("Estimated entropy is: %lf\n", answer);
	  return;
	}
	else //create synthetic stream
	{
	  int* stream=CreateStream(length, zipfparam, range);
      answer = Naive_Handle_stream(stream, c, k, length);
  
      printf("Estimated entropy is: %f\n", answer);
      free(stream);
    }
  }
  if(sflag)
  {
	if(mflag) //read from file
	{
      answer = Slow_Handle_file(filename, c, k, bytes);
      printf("Estimated entropy is: %lf\n", answer);
	  return;
	}
	else //create synthetic stream
	{
	  int* stream=CreateStream(length, zipfparam, range);
      answer = Slow_Handle_stream(stream, c, k, length);
  
      printf("Estimated entropy is: %f\n", answer);
      free(stream);
    }
  }
}

/******************************************************************/

int * CreateStream(int length, double zipfpar, int range)
{
  float zet;
  int i; 
  int * stream, * exact;
  prng_type * prng;
  double entropy, p;
  
  exact = (int*)calloc(range+2, sizeof(int));
  stream=(int *) calloc(length+1,sizeof(int));
      
  prng=prng_Init(44545,2);


  zet=zeta(length,zipfpar);

  for (i=1;i<=length;i++) 
  {
	stream[i]=(int) floor(fastzipf(zipfpar,range,zet,prng));
	exact[ stream[i] ]++;
  }
  
  entropy = 0;
  for(i=0; i<=range+1; i++)
  {
    p = (double) exact[i]/(double)length;
	if(p > 0)
	{
	  entropy+= p * -1 * log(p)/log(2);
    }
  }

  printf("exact entropy is %f\n", entropy);

  prng_Destroy(prng);

  return(stream);

}

