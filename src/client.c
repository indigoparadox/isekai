
#include "client.h"

void client_init( CLIENT* c ) {
    c->running = TRUE;
    scaffold_blank_string( c->buffer );
    scaffold_blank_string( c->nick );
    scaffold_blank_string( c->realname );
    scaffold_blank_string( c->remote );
    scaffold_blank_string( c->username );
    c->sentinal = CLIENT_SENTINAL;
}

void client_cleanup( CLIENT* c ) {
    connection_cleanup( &(c->link) );
    bdestroy( c->buffer );
    bdestroy( c->nick );
    bdestroy( c->realname );
    bdestroy( c->remote );
    bdestroy( c->username );
}

void client_connect( CLIENT* c, bstring server, int port ) {
    connection_connect( &(c->link), server , port );
}

void client_update( CLIENT* c ) {
}

void client_send( CLIENT* c, bstring buffer ) {
    bconchar( buffer, '\r' );
    bconchar( buffer, '\n' );
    connection_write_line( &(c->link), buffer );

    scaffold_print_debug( "Sent: %s", bdata( buffer ) );
}
