/***************************************************************************
 * backup_heap.c																
 * This file is based on the heap implementation released by Michael Fischer
 * under  the terms of the GNU General Public License (Version 3) as
 * published by the Free Software Foundation. The original version may be
 * found in the Downloads section of http://cs-www.cs.yale.edu/homes/fischer/
  
 * The specialized backup_heap data structure is needed in addition to the
 * fully abstracted heap data structure in "heap.c" so that each c_a in the
 * estimator's backup  heap can maintain a record of its position in the 
 * backup heap in its backup_pos field. The records in a backup_heap are of
 * type Sample_type*, not void*. New methods include delete_pos_bheap(),
 * which removes the element at a specified index in the heap, and
 * restore_bheap_property() which restores the min-heap property when the 
 * value of a record at a specified index changes
 ****************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "backup_heap.h"
#include "util.h"
#include "c_a_heap.h"

/* navigation macros */
#define father(a) ((a)/2)
#define left(a)   (2*(a))
#define right(a)  (2*(a)+1)
#define root      1
#define end(h)    (h->cursize+1) /* first available slot */

/* -------------------------------------------------------------------
 * heap constructor
 */
backup_heap* new_bheap( bcomp_fn comp, int initial_size )
{
  backup_heap* h = safe_malloc( sizeof *h );
  h->comp = comp;
  h->cursize = 0;
  h->maxsize = initial_size;
  h->node = safe_malloc( initial_size * sizeof(c_a*) );
  return h;
}

//return size of heap in bytes
int sizeof_bheap(backup_heap* h)
{
  return sizeof(struct backup_heap) + h->maxsize* sizeof(c_a*);
}
/* -------------------------------------------------------------------
 * heap destructor
 */
void free_bheap( backup_heap* h )
{
  free( h->node );
  free( h );
}

/* -------------------------------------------------------------------
 * test if heap is empty
 */
int is_empty_bheap( backup_heap* h )
{
  return h->cursize == 0;
}

int cur_size_bheap(backup_heap* h)
{
  return h->cursize;
}

/* -------------------------------------------------------------------
 * insert element at end of heap and fixup heap by walk towards root
 */
void insert_bheap( backup_heap* h, c_a* value )
{
  /* expand if necessary to accommodate a new node */
  if ( end(h) == h->maxsize ) {	/* no more available slots */
    h->maxsize *= 2;
    h->node = safe_realloc( h->node, h->maxsize * sizeof(c_a*) );
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
	h->node[hole]->backup_pos = hole;
    hole = scan;
  }

  /* fill hole with new element */
  value->backup_pos = hole;
  h->node[ hole ] = value;  
}

/* -------------------------------------------------------------------
 * remove root element and reinsert last element from heap into hole
 * Any c_a whose position in the heap changes will have the change
 * reflected in its backup_pos field
 */
c_a* delete_min_bheap( backup_heap* h )
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to delete from empty heap\n" );

  /* save return value, creating a hole at the root */
  c_a* minval = h->node[ root ];
  h->node[root]->backup_pos = -1;
  int hole = root;

  /* remove last element from tree */
  --h->cursize;
  c_a* value = h->node[ end(h) ];

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
	h->node[hole]->backup_pos = hole;
    hole = smaller;
  }

  /* fill hole with previously last element */
  h->node[ hole ] = value;
  value->backup_pos = hole;

  /* return min element put aside earlier */
  return minval;
}

/* -------------------------------------------------------------------
 * remove element at position index and reinsert last element from heap 
 * into hole. Any c_a whose position in the heap changes will have
 * the change reflected in its backup_pos field
 */
void delete_pos_bheap(backup_heap* h, int index)
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to delete from empty heap\n" );
  if( index >= end(h)) fatal("index in delete_pos_bheap too big\n");
   h->node[index]->backup_pos = -1;

  /* adjust size of heap*/
  --h->cursize;
  if(index < end(h))
  {
    c_a* value = h->node[ end(h) ];
  
    h->node[index]=value;
    restore_bheap_property(h, index);
  }
}


/* -------------------------------------------------------------------
 * Fix heap property after element at index has its value changed.
 * Any c_a whose position in the heap changes will have the change
 * reflected in its backup_pos field
 */
void restore_bheap_property( backup_heap* h, int index)
{
  if(index == -1) return;
  if( index >= end(h)) fatal("index in restore_bheap_property too big\n");
  int hole = index;
  c_a* value = h->node[index];
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
	  h->node[hole]->backup_pos = hole;
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
	h->node[ hole]->backup_pos = hole;
    hole = scan;
   }
 }
  /* fill hole with new element */
  value->backup_pos = hole;
  h->node[ hole ] = value; 
}

c_a * peek_min_bheap(backup_heap* h)
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to peek_min from empty backupheap\n" );

  /* save return value, creating a hole at the root */
  return h->node[ root ];
}

void print_bheap(backup_heap* h)
{
  for(int i = 1; i <= h->cursize; i++)
  {	  
	   fprintf(stderr, "%d ", h->node[i]->key);
	   print_c_a_heap(h->node[i]->sample_heap);
  }
 fprintf(stderr,"\n\n");
}

/*checks that heap property is maintained and all records' backup_pos
 * field is consistent with its position in the backup heap*/
void test_bheap(backup_heap* h)
{
  for(int i = 2; i <= h->cursize; i++)
  {
    if(h->node[i]->backup_pos != i)
	  {
	    fprintf(stderr, "Sample_type*'s pos in backup heap");
		fprintf(stderr, "is inconsistent with its backup_pos field\n");
		exit(1);
	  }
    if(h->comp(h->node[i], h->node[father(i)]) < 0)
	{
	  fprintf(stderr, "backup heap property not maintained\n");
	  exit(1);
	}
  }
}
