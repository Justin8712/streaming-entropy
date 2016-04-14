/**************************************************************************/
/*  backup_heap.h														  */
/**************************************************************************/
#ifndef BACKUPHEAP_H
#define BACKUPHEAP_H

//each sampled token a has a counter c_a
typedef struct c_a{
  int backup_pos; //position in backup heap
  int count, num_prim_samplers, num_backup_samplers, processing;
  int key;
  
  //heap of Sample_type's that have c_a's key as their primary sample
  struct c_a_heap* sample_heap;
  
  //next and previous elements in c_a's bucket in hashtable
  struct c_a* next;
  struct c_a* previous;
} c_a;  

typedef int (*bcomp_fn) (c_a*, c_a*);
typedef struct backup_heap {
  bcomp_fn comp;			/* comparison function */
  int cursize;			/* current size of heap */
  int maxsize;			/* maximum size of heap */
  c_a** node;			/* array of heap nodes */
} backup_heap;

/* prototypes */
backup_heap* new_bheap( bcomp_fn, int inital_size );
void free_bheap( backup_heap* );
int is_empty_bheap( backup_heap* );
void insert_bheap( backup_heap*, c_a* );
c_a* delete_min_bheap( backup_heap* );
c_a* peek_min_bheap(backup_heap* h);
void restore_bheap_property( backup_heap* h, int index);
void delete_pos_bheap(backup_heap* h, int index);
int sizeof_bheap(backup_heap* h);
int cur_size_bheap(backup_heap* h);
void print_bheap(backup_heap* h);
void test_bheap(backup_heap* h);
#endif
