
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

void channel_init( CHANNEL* l, const bstring name );
BOOL channel_client_present( CHANNEL* l, CLIENT* c );
void channel_add_client( CHANNEL* l, CLIENT* c );
void channel_remove_client( CHANNEL* l, CLIENT* c );
CLIENT* channel_get_client_by_name( CHANNEL* l, bstring nick );
struct bstrList* channel_list_clients( CHANNEL* l );
void channel_send( CHANNEL* l, bstring message );
void channel_printf( CHANNEL* l, CLIENT* c_skip, const char* message, ... );
void channel_lock_clients( CHANNEL* l, BOOL lock );

#endif /* CHANNEL_H */
