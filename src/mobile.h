
#ifndef MOBILE_H
#define MOBILE_H

#include "ref.h"
#include "client.h"

typedef struct _MOBILE {
   REF refcount;
   bstring serial;
   CLIENT* owner;
} MOBILE;

#define MOBILE_RANDOM_SERIAL_LEN 6

#define mobile_new( o ) \
    o = (MOBILE*)calloc( 1, sizeof( MOBILE ) ); \
    scaffold_check_null( o ); \
    mobile_init( o );

void mobile_free( MOBILE* o );
void mobile_init( MOBILE* o );

#endif /* MOBILE_H */
