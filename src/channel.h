#ifndef CHANNEL_H
#define CHANNEL_H

#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "client.h"
#include "gamedata.h"

struct CHANNEL {
   struct REF refcount;
   struct GAMEDATA gamedata;
   bstring name;
   bstring topic;
   struct HASHMAP clients;
};

struct CHANNEL_CLIENT {
   struct CHANNEL* l;
   struct CLIENT* c;
};

#define channel_new( l, name ) \
    scaffold_check_null( name ); \
    l = (struct CHANNEL*)calloc( 1, sizeof( struct CHANNEL ) ); \
    scaffold_check_null( l ); \
    channel_init( l, name );

void channel_init( struct CHANNEL* l, const bstring name );
void channel_free( struct CHANNEL* l );
struct CLIENT* channel_client_present( struct CHANNEL* l, struct CLIENT* c );
void channel_add_client( struct CHANNEL* l, struct CLIENT* c );
void channel_remove_client( struct CHANNEL* l, struct CLIENT* c );
struct CLIENT* channel_get_client_by_name( struct CHANNEL* l, bstring nick );
struct bstrList* channel_list_clients( struct CHANNEL* l );

#endif /* CHANNEL_H */
