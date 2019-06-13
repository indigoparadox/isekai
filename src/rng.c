
#define RNG_C
#include "rng.h"

#ifdef USE_CLOCK
#include <time.h>
#endif /* USE_CLOCK */

#ifndef __palmos__
#include <stdlib.h>
#endif /* __palmos__ */

void rng_init() {
#ifdef USE_CLOCK
   time_t tm = 0;

   srand( (unsigned)time( &tm ) );
#endif /* USE_CLOCK */
}

/** \brief Provides length number of pseudo-random bytes from a good entropy
 *         source.
 */
bool rng_bytes( BYTE* ptr, SCAFFOLD_SIZE length ) {
   bool ok = true;
   #ifdef WIN32
   static HCRYPTPROV prov = 0;

   if( 0 == prov ) {
      if( !CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, 0 ) ) {
         lg_error( __FILE__, "Unable to open crypto context.\n" );
         //scaffold_error = SCAFFOLD_ERROR_RANDOM;
         ok = false;
         goto cleanup;
      }
   }
   if( !CryptGenRandom( prov, length, ptr ) ) {
      lg_error( __FILE__, "Unable to generate random bytes.\n" );
      //scaffold_error = SCAFFOLD_ERROR_RANDOM;
      ok = false;
      goto cleanup;
   }

cleanup:
   #elif defined( USE_FILE )
   FILE* randhand = fopen( "/dev/urandom", "rb" );

   if( randhand == NULL ) {
      lg_error( __FILE__, "Unable to open entropy source.\n" );
      //scaffold_error = SCAFFOLD_ERROR_RANDOM;
      ok = false;
      goto cleanup;
   }

   if( 0 == fread( ptr, length, 1, randhand ) ) {
      lg_error( __FILE__, "Unable to read random bytes.\n" );
      //scaffold_error = SCAFFOLD_ERROR_RANDOM;
      ok = false;
      goto cleanup;
   }

cleanup:
   if( NULL != randhand ) {
      fclose( randhand );
   }
   #endif
   if( false != ok ) {
      //scaffold_error = SCAFFOLD_ERROR_NONE;
      lgc_error = LGC_ERROR_NONE;
   }
   return ok;
}

SCAFFOLD_SIZE rng_max( SCAFFOLD_SIZE max ) {
   return rand() % max;
}
