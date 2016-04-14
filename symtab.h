
/*
 *  symtab.h
 */
#include "c_a_heap.h"
#include "backup_heap.h"

#ifndef SYMTAB_H
#define SYMTAB_H

/* opaque type */
typedef struct symtab symtab;

/* prototypes */
symtab* new_symtab( int k );
void free_symtab( symtab* table );
int lookup( symtab* table, int key );
c_a* increment_count(symtab*, int);
void increment_prim_samplers(c_a*, backup_heap*, Sample_type*);
void decrement_backup_samplers(symtab* table, c_a* b);
void increment_backup_samplers(c_a* b);
void done_processing(symtab* table, c_a* b);
int sizeof_symtab(symtab* tab);
int max_row(symtab* table);
int total_elements_tracked(symtab* tab);
void decrement_prim_samplers(symtab* , c_a*, 
                 backup_heap*, Sample_type*);

#endif
