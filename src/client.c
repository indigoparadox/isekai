
#include "client.h"

void client_init( CLIENT* c ) {
    connection_init_socket( &(c->link) );
    c->running = TRUE;
    c->buffer = bfromcstr( "" );
    c->sentinal = CLIENT_SENTINAL;
}
