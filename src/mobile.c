
#include "mobile.h"

static void mobile_cleanup( const struct _REF* ref ) {
   MOBILE* o = scaffold_container_of( ref, MOBILE, refcount );
   if( NULL != o->owner ) {
      client_clear_puppet( o->owner );
      o->owner = NULL;
   }
   bdestroy( o->serial );
}

void mobile_free( MOBILE* o ) {
   ref_dec( &(o->refcount) );
}

void mobile_init( MOBILE* o, bstring serial ) {
   ref_init( &(o->refcount), mobile_cleanup );
   o->serial = bstrcpy( serial );
}

