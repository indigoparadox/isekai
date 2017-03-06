
#ifndef MOBILE_H
#define MOBILE_H

#include "ref.h"
#include "client.h"

struct MOBILE {
   struct REF refcount;
   bstring serial;
   struct CLIENT* owner;
};

#define MOBILE_RANDOM_SERIAL_LEN 6

#define mobile_new( o ) \
    o = (struct MOBILE*)calloc( 1, sizeof( struct MOBILE ) ); \
    scaffold_check_null( o ); \
    mobile_init( o );

void mobile_free( struct MOBILE* o );
void mobile_init( struct MOBILE* o );

#endif /* MOBILE_H */
