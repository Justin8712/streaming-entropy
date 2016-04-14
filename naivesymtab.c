/*
 *  symtab.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "naivesymtab.h"
#include "util.h"
#include "prng.h"

//no keys in hashtable should have count of 0
//if a key is not found in hashtable, indicate with 0
#define NOT_FOUND 0

// Define incomplete type from symtab.h
struct symtab {
  c_a** bucket;
  int size;
  long long a,b;
};

// private functions
static void free_c_a( c_a* c );
static void free_chain( c_a* c );
static int hash( symtab* tab, int item);
static c_a* init_c_a( int key);

// -----------------------------------------------------
// Create symbol table
symtab* new_naivesymtab(int k)
{
  symtab* table = safe_malloc( sizeof *table );
  prng_type* prng = prng_Init(12345, 2);
  prng_int(prng);
  prng_int(prng);
  table->a = (long long) (prng_int(prng)% MOD);
  table->b = (long long) (prng_int(prng)% MOD);
  prng_Destroy(prng);
  
  table->size = k;
  table->bucket = (c_a**) safe_malloc(k * sizeof(c_a*));
  for (int i=0; i<k; i++) {
    table->bucket[ i ] = NULL;
  }
  return table;
}

// -----------------------------------------------------
// Free symbol table
void free_naivesymtab( symtab* table )
{
  for (int i=0; i<table->size; i++) {
    free_chain( table->bucket[i] );
  }
  free(table->bucket);
  free( table );
}


// -----------------------------------------------------
// Lookup key in symbol table
// If found, return the count of the counter for key
// If not found, return special value NOT_FOUND
int naive_lookup( symtab* table, int key )
{
  int bn = hash( table, key );
  c_a* c = table->bucket[ bn ];
  while (c != NULL) {
    if(c->key == key) return c->count;
    c = c->next;
  }
  return NOT_FOUND;
}

//decrements number of primary samplers of b. If b
//is not being processed and not being sampled, b is 
//removed from hashtable.
void naive_decrement_prim_samplers(symtab* table, c_a* b)
{
  b->num_prim_samplers--;
  if(b->num_prim_samplers == 0)
  { 
    if(b->processing == 0)
    { //time to remove sampler from hash table

      int bn = hash(table, b->key);
      c_a* c = table->bucket[ bn ];
  
      if(b == c)
      {
        table->bucket[bn] = b->next;
	    if(b->next != NULL) b->next->previous=NULL;
      }
	  else
	  {
	    b->previous->next = b->next;
	    if(b->next != NULL) b->next->previous=b->previous;
	  }
	  free_c_a(b);
	  return;
    }
  }
}

//decrements number of backup samplers sampling b
//if b is not being processed and no one is sampling b
//then b is removed from the hashtable
void naive_done_processing(symtab* table, c_a* b)
{
  b->processing = 0;
  if(b->num_prim_samplers == 0)
  { //time to remove sampler from hash table

    int bn = hash(table, b->key);
    c_a* c = table->bucket[ bn ];
  
    if(b == c)
    {
      table->bucket[bn] = b->next;
	  if(b->next != NULL) b->next->previous=NULL;
    }
	else
	{
	  b->previous->next = b->next;
	  if(b->next != NULL) b->next->previous=b->previous;
	}
	free_c_a(b);
  }
}

//increments number of prim samplers of b
void naive_increment_prim_samplers(c_a* b)
{
  b->num_prim_samplers++;
}

//increment count of key
//if key is not in table, insert it and 
//set processing and count to 1, num_prim_samplers to 0
//returns pointer to the key
c_a* naive_increment_count(symtab* table, int key)
{
  int bn = hash(table, key);
  c_a* c = table->bucket[ bn ];
  while (c != NULL) {
    if(c->key == key)
	{
	  c->count++;
	  c->processing = 1;
	  return c;
	}
    else c = c->next;
  }
  // key not found; create new cell
  c_a* n = init_c_a(key);
  n->next = table->bucket[bn];
  if(table->bucket[bn] != NULL)
	table->bucket[bn]->previous = n;
  table->bucket[ bn ] = n;
  return n;
}  

//return the length of the longest bucket
//used for testing effectiveness of hash function
/*
int max_row(symtab* tab)
{
  int j = 0;
  int max = 0;
  c_a* iter;
  for(int i = 0; i < tab->size; i++)
  {
    j = 0;
	iter = tab->bucket[i];
	while(iter != NULL)
	{
	  j++;
	  iter = iter->next;
	}
    if(max < j) max = j;
  }
  return max;
}
*/
/*
int total_elements_tracked(symtab* tab)
{
  int count = 0;
  c_a* iter;
  for(int i = 0; i < tab->size; i++)
  {
	iter = tab->bucket[i];
	while(iter != NULL)
	{
	  count++;
	  iter = iter->next;
	}
  }
  return count;
}*/

int sizeof_naivesymtab(symtab* tab)
{
  int size = sizeof(struct symtab) + tab->size * sizeof(c_a**);
  c_a* iter;
  for(int i = 0; i < tab->size; i++)
  {
	iter = tab->bucket[i];
	while(iter != NULL)
	{
	  size += sizeof(struct c_a);
	  iter = iter->next;
	}
  }
  return size;
}

// =====================================================
// Local functions
// -----------------------------------------------------
// Create a new cell


static c_a* init_c_a( int key)
{
   c_a* value = (c_a*) safe_malloc(sizeof(c_a));
   value->key = key;
   value->next = value->previous = NULL;
   value->count = value->processing= 1;
   value->num_prim_samplers = 0;
   return value;
}


// -----------------------------------------------------
// Free a linked chain of cells
static void free_chain( c_a* c )
{
  c_a* next;
  while (c != NULL) {
    next = c->next;
    free_c_a( c );
    c = next;
  }
}

static void free_c_a( c_a* c )
{
  free(c);
}

// -----------------------------------------------------
// Compute hash value in range [0..nBuckets-1] from string s
static int hash( symtab* tab, int item)
{
   return hash31(tab->a,tab->b,item) % tab->size;
}
