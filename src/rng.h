
#ifndef RNG_H
#define RNG_H

#include "scaffold.h"

#define rng_gen_serial( object, vector, min, max ) \
   do { \
      object->serial = min + rng_max( max ); \
   } while( 0 == object->serial || NULL != vector_get( vector, object->serial ) );

void rng_init();
BOOL rng_bytes( BYTE* ptr, SCAFFOLD_SIZE length );
SCAFFOLD_SIZE rng_max( SCAFFOLD_SIZE max );

#ifdef RNG_C
SCAFFOLD_MODULE( "rng.c" );
#endif /* RNG_C */

#endif /* RNG_H */
