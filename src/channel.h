
#ifndef CHANNEL_H
#define CHANNEL_H

#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "client.h"

typedef struct {
    bstring name;
    VECTOR clients;
} CHANNEL;

void channel_init( CHANNEL* l, const bstring name );
BOOL channel_client_present( CHANNEL* l, CLIENT* c );
void channel_add_client( CHANNEL* l, CLIENT* c );
void channel_remove_client( CHANNEL* l, CLIENT* c );
struct bstrList* channel_list_clients( CHANNEL* l );
void channel_send( CHANNEL* l, bstring message );
void channel_lock( CHANNEL* l );
void channel_unlock( CHANNEL* l );

#endif /* CHANNEL_H */
