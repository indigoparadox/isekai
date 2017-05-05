
#ifndef RNG_H
#define RNG_H

#include "scaffold.h"

void rng_init();
BOOL rng_bytes( BYTE* ptr, SCAFFOLD_SIZE length );
BIG_SERIAL rng_serial();
SCAFFOLD_SIZE rng_max( SCAFFOLD_SIZE max );

#ifdef RNG_C
SCAFFOLD_MODULE( "rng.c" );
#endif /* RNG_C */

#endif /* RNG_H */
