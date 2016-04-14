/*
 *  symtab.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "symtab.h"
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
static int c_a_heap_cmp(Sample_type* p, Sample_type* q);

// -----------------------------------------------------
// Create symbol table
symtab* new_symtab(int k)
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
void free_symtab( symtab* table )
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
int lookup( symtab* table, int key )
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
//removed from hashtable. removes min from b->sample_heap
//and restores heap property to backup heap
void decrement_prim_samplers(symtab* table, c_a* b, 
						backup_heap* h, Sample_type* min)
{
  b->num_prim_samplers--;
  if(b->num_prim_samplers == 0)
  { //need to remove b from backup heap
	delete_pos_bheap(h, b->backup_pos);
    if(b->num_backup_samplers == b->processing &&
	   b->processing == 0)
    { //time to remove sampler from hash table

      int bn = hash(table, b->key);
      c_a* c = table->bucket[ bn ];
  
      if(b == c)
      { //b is first element in bucket
        table->bucket[bn] = b->next;
	    if(b->next != NULL) b->next->previous=NULL;
      }
	  else
	  { //b is not first element in bucket
	    b->previous->next = b->next;
	    if(b->next != NULL) b->next->previous=b->previous;
	  }
	  free_c_a(b);
	  return;
    }
  }
  delete_pos_c_a_heap(b->sample_heap, min->c_s0_pos);
  restore_bheap_property(h, b->backup_pos);
}

//decrements number of backup samplers sampling b
//if b is not being processed and no one is sampling b
//then b is removed from the hashtable
void decrement_backup_samplers(symtab* table, c_a* b)
{
  b->num_backup_samplers--;
  if(b->num_prim_samplers == b->num_backup_samplers &&
     b->num_backup_samplers == b->processing &&
	 b->processing == 0)
  { //time to remove sampler from hash table

    int bn = hash(table, b->key);
    c_a* c = table->bucket[ bn ];
  
    if(b == c)
    { //b is first element in bucket
      table->bucket[bn] = b->next;
	  if(b->next != NULL) b->next->previous=NULL;
    }
	else
	{ //b is not first element in bucket
	  b->previous->next = b->next;
	  if(b->next != NULL) b->next->previous=b->previous;
	}
	free_c_a(b);
  }
}

//sets b's processing field to 0, if no sampler has b as a primary or
//backup sample, b is removed from hashtable to save space
void done_processing(symtab* table, c_a* b)
{
  b->processing = 0;
  if(b->num_prim_samplers == b->num_backup_samplers &&
     b->num_backup_samplers == 0)
  { //time to remove sampler from hash table

    int bn = hash(table, b->key);
    c_a* c = table->bucket[ bn ];
  
    if(b == c)
    { //b is first element in bucket
      table->bucket[bn] = b->next;
	  if(b->next != NULL) b->next->previous=NULL;
    }
	else
	{ //b is not first element in bucket
	  b->previous->next = b->next;
	  if(b->next != NULL) b->next->previous=b->previous;
	}
	free_c_a(b);
  }
}

//precondition: min's wait times set properly
//postcondition: min is in proper place in backup_heap and in b's sample_heap
void increment_prim_samplers(c_a* b, backup_heap* h, Sample_type* min)
{
  b->num_prim_samplers++;
  insert_c_a_heap(b->sample_heap, min);
  if(b->num_prim_samplers == 1)
  {
    insert_bheap(h, b);
  }
  else
  {
    restore_bheap_property(h, b->backup_pos);
  }
}

void increment_backup_samplers(c_a* b)
{
  b->num_backup_samplers++;
}


//increment count of key
//if key is not in table, insert it and 
//set processing and count to 1, num_prim/backup_samplers to 0
//returns pointer to the key
//DOES NOT RESTORE HEAP PROPERTY IN BACKUP HEAP
c_a* increment_count(symtab* table, int key)
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
}

int sizeof_symtab(symtab* tab)
{
  int size = sizeof(struct symtab) + tab->size * sizeof(c_a**);
  c_a* iter;
  for(int i = 0; i < tab->size; i++)
  {
	iter = tab->bucket[i];
	while(iter != NULL)
	{
	  size += sizeof(struct c_a) + sizeof_c_a_heap(iter->sample_heap);
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
   value->num_prim_samplers = value->num_backup_samplers = 0;
   value->sample_heap = new_c_a_heap(c_a_heap_cmp, 10);
   value->backup_pos = -1;
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
  free_c_a_heap(c->sample_heap);
  free(c);
}

static int c_a_heap_cmp(Sample_type* p, Sample_type* q)
{
  int a = p->backup_minus_delay;
  int b = q->backup_minus_delay;
  return (a==b) ? 0 : (a<b) ? -1 : 1;
}

// -----------------------------------------------------
// Compute hash value in range [0..nBuckets-1] from string s
static int hash( symtab* tab, int item)
{
   return hash31(tab->a,tab->b,item) % tab->size;
}
