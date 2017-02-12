
#include "client.h"

void client_init( CLIENT* c ) {
    c->running = TRUE;
    c->buffer = bfromcstr( "" );
    c->sentinal = CLIENT_SENTINAL;
}

void client_cleanup( CLIENT* c ) {
    connection_cleanup( &(c->link) );
    bdestroy( c->buffer );
}

void client_connect( CLIENT* c, bstring server, int port ) {
    connection_connect( &(c->link), server , port );
}

void client_update( CLIENT* c ) {
}

void client_send( CLIENT* c, bstring buffer ) {
    bconchar( buffer, '\n' );
    connection_write_line( &(c->link), buffer );
}
