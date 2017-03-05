
#include "mobile.h"

static void mobile_cleanup( const struct _REF* ref ) {
   MOBILE* m = scaffold_container_of( ref, MOBILE, refcount );
}

void mobile_free( MOBILE* o ) {
   ref_dec( &(o->refcount) );
}

void mobile_init( MOBILE* o ) {
   ref_init( &(o->refcount), mobile_cleanup );
}

