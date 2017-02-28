#ifndef CHANNEL_H
#define CHANNEL_H

#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "client.h"
#include "gamedata.h"

typedef struct _CHANNEL {
   GAMEDATA gamedata;
   bstring name;
   bstring topic;
   VECTOR clients;
   REF refcount;
} CHANNEL;

#define channel_new( l, name ) \
    scaffold_check_null( name ); \
    l = (CHANNEL*)calloc( 1, sizeof( CHANNEL ) ); \
    scaffold_check_null( l ); \
    channel_init( l, name );

void* cb_channel_get_name( VECTOR* v, size_t idx, void* iter, void* arg );
BOOL cb_channel_del_clients( VECTOR* v, size_t idx, void* iter, void* arg );

void channel_init( CHANNEL* l, const bstring name );
void channel_free( CHANNEL* l );
CLIENT* channel_client_present( CHANNEL* l, CLIENT* c );
void channel_add_client( CHANNEL* l, CLIENT* c );
void channel_remove_client( CHANNEL* l, CLIENT* c );
CLIENT* channel_get_client_by_name( CHANNEL* l, bstring nick );
struct bstrList* channel_list_clients( CHANNEL* l );

#endif /* CHANNEL_H */
