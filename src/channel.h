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
} CHANNEL;

#define channel_new( l, name ) \
    scaffold_check_null( name ); \
    l = (CHANNEL*)calloc( 1, sizeof( CHANNEL ) ); \
    scaffold_check_null( l ); \
    channel_init( l, name );

void channel_init( CHANNEL* l, const bstring name );
void channel_cleanup( CHANNEL* l );
BOOL channel_client_present( CHANNEL* l, CLIENT* c );
void channel_add_client( CHANNEL* l, CLIENT* c );
void channel_remove_client( CHANNEL* l, CLIENT* c );
CLIENT* channel_get_client_by_name( CHANNEL* l, bstring nick );
struct bstrList* channel_list_clients( CHANNEL* l );
void channel_lock_clients( CHANNEL* l, BOOL lock );

#endif /* CHANNEL_H */
