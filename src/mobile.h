
#ifndef MOBILE_H
#define MOBILE_H

#include "ref.h"
#include "client.h"

typedef struct _MOBILE {
   REF refcount;
   bstring serial;
   CLIENT* owner;
} MOBILE;

void mobile_free( MOBILE* o );
void mobile_init( MOBILE* o, bstring serial );

#endif /* MOBILE_H */
