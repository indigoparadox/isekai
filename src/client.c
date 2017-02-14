
#include "client.h"

void client_init( CLIENT* c ) {
    c->running = TRUE;
    c->buffer = bfromcstralloc( 256, "" );
    c->nick = bfromcstralloc( 32, "" );
    c->realname = bfromcstralloc( 32, "" );
    c->remote = bfromcstralloc( 32, "" );
    c->username = bfromcstralloc( 32, "" );
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
    bstring buffer;

    connection_connect( &(c->link), server , port );
    scaffold_check_negative( scaffold_error );

    buffer = bfromcstr( "NICK " );
    bconcat( buffer, c->nick );
    client_send( c, buffer );
    bdestroy( buffer );

    buffer = bfromcstr( "USER " );
    bconcat( buffer, c->realname );
    client_send( c, buffer );
    bdestroy( buffer );

cleanup:

    return;
}

void client_update( CLIENT* c ) {
}

void client_send( CLIENT* c, bstring buffer ) {

    /* TODO: Make sure we're still connected. */

    bconchar( buffer, '\r' );
    bconchar( buffer, '\n' );
    connection_write_line( &(c->link), buffer );

    scaffold_print_debug( "Sent: %s", bdata( buffer ) );
}
