/***************************************************************************
 * heap.c																
 * This file is based on the heap implementation released by Michael Fischer
 * under  the terms of the GNU General Public License (Version 3) as
 * published by the Free Software Foundation. The original version may be
 * found in the Downloads section of http://cs-www.cs.yale.edu/homes/fischer/
 
 * The only modification made to the original version from Professor Fischer
 * is the inclusion of the fuctions peek_min() and sizeof_heap()
 ***************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "heap.h"
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
heap* new_heap( comp_fn comp, int initial_size )
{
  heap* h = safe_malloc( sizeof *h );
  h->comp = comp;
  h->cursize = 0;
  h->maxsize = initial_size;
  h->node = safe_malloc( initial_size * sizeof(void*) );
  return h;
}

//return size of heap in bytes
//sizeof_heap not included in original version of code
int sizeof_heap(heap* h)
{
  return sizeof(struct heap) + h->maxsize* sizeof(void*);
}
/* -------------------------------------------------------------------
 * heap destructor
 */
void free_heap( heap* h )
{
  free( h->node );
  free( h );
}

/* -------------------------------------------------------------------
 * test if heap is empty
 */
int is_empty_heap( heap* h )
{
  return h->cursize == 0;
}

int cur_size(heap* h)
{
  return h->cursize;
}

/* -------------------------------------------------------------------
 * insert element at end of heap and fixup heap by walk towards root
 */
void insert_heap( heap* h, void* value )
{
  /* expand if necessary to accommodate a new node */
  if ( end(h) == h->maxsize ) {	/* no more available slots */
    h->maxsize *= 2;
    h->node = safe_realloc( h->node, h->maxsize * sizeof(void*) );
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
    hole = scan;
  }

  /* fill hole with new element */
  h->node[ hole ] = value;   
}

/* -------------------------------------------------------------------
 * remove root element and reinsert last element from heap into hole
 */
void* delete_min( heap* h )
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to delete from empty heap\n" );

  /* save return value, creating a hole at the root */
  void* minval = h->node[ root ];
  int hole = root;

  /* remove last element from tree */
  --h->cursize;
  void* value = h->node[ end(h) ];

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
    hole = smaller;
  }

  /* fill hole with previously last element */
  h->node[ hole ] = value;

  /* return min element put aside earlier */
  return minval;
}

/*look at the minimum element in the heap*/
/*peek_min() not included in original version of code*/
void* peek_min(heap* h)
{
  /* safety check */
  if ( h->cursize <= 0 ) fatal( "Attempt to peek_min from empty heap\n" );

  return h->node[ root ];
}


/* -------------------------------------------------------------------
 * map fn() onto each heap node
 */
void map_heap( heap_fn fn, heap* h, void* clientData )
{
  for (int k=root; k<end(h); k++) {
    fn( h->node[ k ], clientData );
  }
}
