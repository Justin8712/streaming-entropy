/***************************************************************************
 * c_a_heap.c																
 * This file is based on the heap implementation released by Michael Fischer
 * under  the terms of the GNU General Public License (Version 3) as
 * published by the Free Software Foundation. The original version may be
 * found in the Downloads section of http://cs-www.cs.yale.edu/homes/fischer/
  
 * The specialized c_a_heap data structure is needed in addition to the
 * fully abstracted heap data structure in "heap.c" so that each Sample_type*
 * in a c_a's heap of samplers can maintain a record of its position in its
 * primary sample's heap. New methods include delete_pos_bheap(), which removes
 * the element at a specified index in the heap, and restore_bheap_property() 
 * which restores the min-heap property when the value of a record at a specified
 * index changes*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "c_a_heap.h"
#include "util.h"

/* navigation macros */
#define father(a) ((a)/2)
#define left(a)   (2*(a))
#define right(a)  (2*(a)+1)
#define root      1
#define end(h)    (h->cursize+1) /* first available slot */

/* -------------------------------------------------------------------
 * heap constructor
 */
c_a_heap* new_c_a_heap( c_a_comp_fn comp, int initial_size )
{
  c_a_heap* h = safe_malloc( sizeof *h );
  h->comp = comp;
  h->cursize = 0;
  h->maxsize = initial_size;
  h->node = safe_malloc( initial_size * sizeof(Sample_type*) );
  return h;
}

//return size of heap in bytes
int sizeof_c_a_heap(c_a_heap* h)
{
  return sizeof(struct c_a_heap) + h->maxsize* sizeof(Sample_type*);
}
/* -------------------------------------------------------------------
 * heap destructor
 */
void free_c_a_heap( c_a_heap* h )
{
  free( h->node );
  free( h );
}

/* -------------------------------------------------------------------
 * test if heap is empty
 */
int is_empty_c_a_heap( c_a_heap* h )
{
  return h->cursize == 0;
}

int cur_size_c_a_heap(c_a_heap* h)
{
  return h->cursize;
}

/* -------------------------------------------------------------------
 * insert element at end of heap and fixup heap by walk towards root
 * any Sample_type whose position in the heap changes will have the change
 * reflected in its c_s0_pos field
 */
void insert_c_a_heap( c_a_heap* h, Sample_type* value )
{
  /* expand if necessary to accommodate a new node */
  if ( end(h) == h->maxsize ) {	/* no more available slots */
    h->maxsize *= 2;
    h->node = safe_realloc( h->node, h->maxsize * sizeof(Sample_type*) );
  }

  /* create a hole at end of heap */
  int hole = end(h);
  h->cursize++;

  /* move hole upwards to place where new value goes */
  while (hole != root) {
    /* test if value belongs in hole */
    int scan = father(hole);    
    if (h->comp( h->node[ scan ], value ) <= 0) /* yes */
      break;
    /* no, move hole up one level */
    h->node[ hole ] = h->node[ scan ];
	h->node[ hole]->c_s0_pos = hole;
    hole = scan;
  }

  /* fill hole with new element */
  value->c_s0_pos = hole;
  h->node[ hole ] = value;   
}

/* -------------------------------------------------------------------
 * remove root element and reinsert last element from heap into hole
 * any Sample_type whose position in the heap changes will have the change
 * reflected in its c_a_pos field
 * SHOULD TECHNICALLY FREE UP SPACE FROM HEAP WHEN ELEMENTS ARE REMOVED
 * BUT FOR SIMPLICITY WE DO NOT DO THIS NOW
 */
Sample_type* delete_min_c_a_heap( c_a_heap* h )
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to delete from empty heap\n" );

  /* save return value, creating a hole at the root */
  Sample_type* minval = h->node[ root ];
  int hole = root;
   h->node[root]->c_s0_pos = -1; 
  /* remove last element from tree */
  --h->cursize;
  Sample_type* value = h->node[ end(h) ];

  /* move hole downwards, always filling hole with smaller of two sons.
     stop when hole is at place where removed element goes */
  for (;;) {
    int lson = left( hole );
    int rson = right( hole );

    /* find smaller son, if any */
    int smaller = lson;
    if (lson >= end(h)) break; /* hole is a leaf */
    if (rson < end(h) &&
	    h->comp( h->node[ lson ], h->node[ rson ] ) > 0)
      smaller = rson;		/* rson exists and is smaller than lson */

    /* does value go in hole? */
    if (h->comp( value, h->node[ smaller ] ) <= 0) /* yes */
      break;

    /* no, move hole down one level */
    h->node[ hole ] = h->node[ smaller ];
	h->node[hole]->c_s0_pos = hole;
    hole = smaller;
  }

  /* fill hole with previously last element */
  h->node[ hole ] = value;
  value->c_s0_pos = hole;

  /* return min element put aside earlier */
  return minval;
}

/* -------------------------------------------------------------------
 * remove element at position index and reinsert last element from heap 
 * into hole. Any Sample_type whose position in the heap changes will have
 * the change reflected in its c_s0_pos field
 * SHOULD TECHNICALLY FREE UP SPACE FROM HEAP WHEN ELEMENTS ARE REMOVED
 * BUT FOR SIMPLICITY WE DO NOT DO THIS NOW
 */
void delete_pos_c_a_heap(c_a_heap* h, int index)
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to delete from empty heap\n" );
  if( index >= end(h)) fatal("index in delete_pos_bheap too big\n");

  h->node[index]->c_s0_pos = -1;

  /* remove last element from tree */
  --h->cursize;
  Sample_type* value = h->node[ end(h) ];
  //value->c_s0_pos = index;
  if(index < end(h))
  {
    h->node[index]=value;
    restore_c_a_heap_property(h, index);
  }
}

/* -------------------------------------------------------------------
 * Fix heap property after element at index has its value changed.
 * Any Sample_type whose position in the heap changes will have the change
 * reflected in its c_s0_pos field
 */
void restore_c_a_heap_property( c_a_heap* h, int index)
{
  if(index == -1) 
  {
	fprintf(stderr, "restore_c_a_heapprop called w/. index -1!!\n");
	exit(1);
  }
  
  if( index >= end(h)) fatal("index in delete_pos_bheap too big\n");
  
  int hole = index;
  Sample_type* value = h->node[index];
  int lson = left( hole );
  int rson = right( hole );
  if((lson < end(h) && h->comp(value, h->node[lson]) > 0) || 
     (rson < end(h) && h->comp(value, h->node[rson]) > 0) )
  {
    /* move hole downwards, always filling hole with smaller of two sons.
     stop when hole is at place where removed element goes */
    for (;;) {
      lson = left( hole );
      rson = right( hole );

      /* find smaller son, if any */
      int smaller = lson;
      if (lson >= end(h)) break; /* hole is a leaf */
      if (rson < end(h) &&
	      h->comp( h->node[ lson ], h->node[ rson ] ) > 0)
        smaller = rson;		/* rson exists and is smaller than lson */

      /* does value go in hole? */
      if (h->comp( value, h->node[ smaller ] ) <= 0) /* yes */
        break;

      /* no, move hole down one level */
      h->node[ hole ] = h->node[ smaller ];
	  h->node[hole]->c_s0_pos = hole;
      hole = smaller;
    }
 }
 else
 {
    /* move hole upwards to place where new value goes */
   while (hole != root) {
     /* test if value belongs in hole */
     int scan = father(hole);    
     if (h->comp( h->node[ scan ], value ) <= 0) /* yes */
       break;
     /* no, move hole up one level */
     h->node[ hole ] = h->node[ scan ];
	 h->node[ hole]->c_s0_pos = hole;
     hole = scan;
   }
 }

  /* fill hole with new element */
  value->c_s0_pos = hole;
  h->node[ hole ] = value;
} 

Sample_type* peek_min_c_a_heap(c_a_heap* h)
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to peek_min from empty c_a_heap\n" );

  /* save return value, creating a hole at the root */
  return h->node[ root ];
}

/*print all the elements' backup_minus_delay field in h*/
void print_c_a_heap(c_a_heap* h)
{
  for(int i = 1; i <= h->cursize; i++)
  {
    fprintf(stderr, "%d ", h->node[i]->backup_minus_delay);
  }
 fprintf(stderr,"done printing\n\n");
}

/*print all the elements' backup_minus_delay field in h, noting when
 *we print the element at a specific index. For use in debugging*/
void print_c_a_heapspec(c_a_heap* h, int index)
{
  for(int i = 1; i <= index; i++)
  {
    fprintf(stderr, "%d ", h->node[i]->backup_minus_delay);
  }
  fprintf(stderr, "\nmoving past index\n");
    for(int i = index+1; i <= h->cursize; i++)
  {
    fprintf(stderr, "%d ", h->node[i]->backup_minus_delay);
  }
 fprintf(stderr,"done printing\n\n");
}

/*tests to make sure the heap property is satisfied and that each
 *Sample_type*'s c_s0_pos field is consistent with its actual position
 *in the heap*/
void test_heap(c_a_heap* h)
{
  for(int i = 2; i <= h->cursize; i++)
  {
    if(h->node[i]->c_s0_pos != i)
	  {
	    fprintf(stderr, "an element's c_s0_pos does not match its position in heap.");
		fprintf(stderr, "\nHeap looks like \n");
		print_c_a_heap(h);
		exit(1);
	  }
    if(h->comp(h->node[i], h->node[father(i)]) < 0)
	{
	  fprintf(stderr, "c_a_heap property not maintained\nHeap looks like: ");
	  print_c_a_heap(h);
	  exit(1);
	}
	//fprintf(stderr, "ok  ");
  }
}

