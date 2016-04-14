#ifndef ENTROPYPUB_H
#define ENTROPYPUB_H

typedef struct Estimator_type Estimator_type;

extern Estimator_type* Estimator_Init(int c, int k);
extern void Estimator_Destroy(Estimator_type * est);
extern int Estimator_Size(Estimator_type * est);
extern void Estimator_Update(Estimator_type * est, int token);
extern double Estimator_end_stream(Estimator_type* est);

#endif