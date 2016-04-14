
/*
 *  symtab.h
 */

#ifndef NAIVESYMTAB_H
#define NAIVESYMTAB_H

typedef struct c_a{
  int count, num_prim_samplers, processing;
  int key;
    
  //next and previous elements in c_a's bucket in hashtable
  struct c_a* next;
  struct c_a* previous;
} c_a;  

/* opaque type */
typedef struct symtab symtab;

/* prototypes */
symtab* new_naivesymtab( int k );
void free_naivesymtab( symtab* table );
int naive_lookup( symtab* table, int key );
c_a* naive_increment_count(symtab*, int key);
void naive_increment_prim_samplers(c_a* b);
void naive_done_processing(symtab* table, c_a* b);
int sizeof_naivesymtab(symtab* tab);
void naive_decrement_prim_samplers(symtab* , c_a*);

#endif
