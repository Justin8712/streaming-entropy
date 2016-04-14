/*c_a_heap.h*/
#include "backup_heap.h"

#ifndef C_A_HEAP_H
#define C_A_HEAP_H

typedef struct Sample_type{
  int val_c_s0, val_c_s1; //values of c_s0/s1 when s0 and s1 were sampled
  double t0, t1;
  c_a* c_s0;
  c_a* c_s1;
  int c_s0_pos; //position in c_s0's heap of samplers
  int prim; //next time to take primary sample
  int backup_minus_delay;//next time to take backup sample minus c_s0's delay
} Sample_type;

typedef int (*c_a_comp_fn) (Sample_type*, Sample_type*);
typedef struct c_a_heap {
  c_a_comp_fn comp;			/* comparison function */
  int cursize;			/* current size of heap */
  int maxsize;			/* maximum size of heap */
  Sample_type** node;   /* array of heap nodes */
} c_a_heap;

/* prototypes */
c_a_heap* new_c_a_heap( c_a_comp_fn, int inital_size );
void free_c_a_heap( c_a_heap* );
int is_empty_c_a_heap( c_a_heap* );
void insert_c_a_heap( c_a_heap*, Sample_type* );
Sample_type* delete_min_c_a_heap( c_a_heap* );
Sample_type* peek_min_c_a_heap(c_a_heap* h);
void delete_pos_c_a_heap(c_a_heap* h, int index);
void restore_c_a_heap_property( c_a_heap* h, int index);
int sizeof_c_a_heap(c_a_heap* h);
int cur_size_c_a_heap(c_a_heap* h);
void print_c_a_heap(c_a_heap* h);
void test_heap(c_a_heap* h);
void print_c_a_heapspec(c_a_heap* h, int index);
#endif
