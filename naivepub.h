#ifndef NAIVEPUB_H
#define NAIVEPUB_H

typedef struct Naive_Estimator_type Naive_Estimator_type;

extern Naive_Estimator_type* Naive_Estimator_Init(int c, int k);
extern void Naive_Estimator_Destroy(Naive_Estimator_type * est);
extern int Naive_Estimator_Size(Naive_Estimator_type * est);
extern void Naive_Estimator_Update(Naive_Estimator_type * est, int token);
extern double Naive_Estimator_end_stream(Naive_Estimator_type* est);

#endif