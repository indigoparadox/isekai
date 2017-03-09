
#include "check_data.h"

struct tagbstring str_key_hit = bsStatic( "This key should be a hit." );
struct tagbstring str_key_miss = bsStatic( "This key should be a miss." );
struct tagbstring str_key_also1 = bsStatic( "This key isn't relevant." );
struct tagbstring str_key_also2 = bsStatic( "This key is not relevant." );
struct tagbstring str_key_also3 = bsStatic( "This key ain't relevant." );

#include <stdlib.h>

static void free_blob( const struct REF *ref ) {
    BLOB* blob = scaffold_container_of( ref, struct _BLOB, refcount );
    free( blob->data );
    free( blob );
}

BLOB* create_blob( uint32_t sent_s, uint16_t ptrn, SCAFFOLD_SIZE c, uint32_t sent_e ) {
   SCAFFOLD_SIZE i;
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
