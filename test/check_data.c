
#include "check_data.h"

#include <stdlib.h>

static void free_blob( const struct _REF *ref ) {
    BLOB* blob = scaffold_container_of( ref, struct _BLOB, refcount );
    free( blob->data );
    free( blob );
}

BLOB* create_blob( uint32_t sent_s, uint16_t ptrn, size_t c, uint32_t sent_e ) {
   size_t i;
   BLOB* blob;

   blob = (BLOB*)calloc( 1, sizeof( BLOB ) );
   ref_init( &(blob->refcount), free_blob );

   blob->sentinal_start = sent_s;
   blob->data_len = c;
   blob->data = (uint16_t*)calloc( c, sizeof( uint16_t ) );
   for( i = 0 ; c > i ; i++ ) {
      blob->data[i] = ptrn;
   }
   blob->sentinal_end = sent_e;

   return blob;
}

/*
void free_blob( BLOB* blob ) {
   free( blob->data );
   free( blob );
}
*/
