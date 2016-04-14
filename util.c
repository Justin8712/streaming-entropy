/***************************************************************************
 * This file released by Michael Fischer under  the terms of the GNU General
 * Public License (Version 3) as published by the Free Software Foundation. 
 * The original version may be found in the Downloads section of 
 * http://cs-www.cs.yale.edu/homes/fischer/
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "util.h"

//--------------------------------------------------------------------------
// malloc memory and abort on failure
//--------------------------------------------------------------------------
void* safe_malloc( size_t size )
{
  void* ret = malloc( size );
  if ( ret == NULL ) fatal( "safe_malloc: Out of memory" );
  return ret;
}

//--------------------------------------------------------------------------
// realloc memory and abort on failure
//--------------------------------------------------------------------------
void* safe_realloc( void *ptr, size_t size )
{
  void* ret = realloc( ptr, size );
  if ( ret == NULL ) fatal( "safe_realloc: Out of memory" );
  return ret;
}

/* ----------------------------------------------------------------------------
 * Report and exit gracefully from fatal error
 * This function is called like printf().
 * It prints an error comment to stderr and then exits with EXIT_FAILURE
 *   return code.

*/
void fatal ( char* format, ... )  // dots mean variable # args
{
  va_list vargs;                // optional arguments

  va_start( vargs, format );    // get varying part of arg list
  vfprintf( stderr, format, vargs ); // variable part as if a call to fprintf()
  fprintf( stderr, "\n" );      // print a newline, just in case
  exit( EXIT_FAILURE );         // report failure to invoking process
}
