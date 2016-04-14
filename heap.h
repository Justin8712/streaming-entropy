/***************************************************************************
 *   Copyright (C) 2006, 2007 by Michael Fischer                           *
 *   fischer-michael@cs.yale.edu                                           *
 *                                                                         *
 *   For use in Yale course CPSC 223b, Spring 2007                         *
 ***************************************************************************/

#ifndef HEAP_H
#define HEAP_H

typedef int (*comp_fn) (void*, void*);
typedef void (*heap_fn)( const void*, void*);
typedef struct heap {
  comp_fn comp;			/* comparison function */
  int cursize;			/* current size of heap */
  int maxsize;			/* maximum size of heap */
  void** node;			/* array of heap nodes */
} heap;

/* prototypes */
heap* new_heap( comp_fn, int inital_size );
void free_heap( heap* );
int is_empty_heap( heap* );
void insert_heap( heap*, void* );
void* delete_min( heap* );
void map_heap( heap_fn, heap*, void* );
void* peek_min(heap* h);
int sizeof_heap(heap* h);
int cur_size(heap* h);
#endif
