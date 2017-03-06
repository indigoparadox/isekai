
#include "mobile.h"

static void mobile_cleanup( const struct REF* ref ) {
   struct MOBILE* o = scaffold_container_of( ref, struct MOBILE, refcount );
   if( NULL != o->owner ) {
      client_clear_puppet( o->owner );
      o->owner = NULL;
   }
   bdestroy( o->serial );
   free( o );
}

void mobile_free( struct MOBILE* o ) {
   ref_dec( &(o->refcount) );
}

void mobile_init( struct MOBILE* o ) {
   ref_init( &(o->refcount), mobile_cleanup );
   o->serial = bfromcstralloc( MOBILE_RANDOM_SERIAL_LEN, "" );
}
