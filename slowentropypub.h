#ifndef SLOWENTROPYPUB_H
#define SLOWENTROPYPUB_H

#include "prng.h"
#include "frequent.h"

typedef struct Slow_Estimator_type Slow_Estimator_type;

extern void Slow_Estimator_Destroy(Slow_Estimator_type* est);
extern int Slow_Estimator_Size(Slow_Estimator_type* est);
extern Slow_Estimator_type * Slow_Estimator_Init(int c, int k);
extern void Slow_Estimator_Update(Slow_Estimator_type * est, int token);
extern double Slow_Estimator_end_stream(Slow_Estimator_type* est);

#endif