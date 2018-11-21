
#ifndef RNG_H
#define RNG_H

#include "scaffold.h"

#define rng_gen_serial( object, vector, min, max ) \
   do { \
      object->serial = min + rng_max( max ); \
   } while( 0 == object->serial || NULL != vector_get( vector, object->serial ) );

void rng_init();
VBOOL rng_bytes( BYTE* ptr, SCAFFOLD_SIZE length );
SCAFFOLD_SIZE rng_max( SCAFFOLD_SIZE max );

#endif /* RNG_H */
