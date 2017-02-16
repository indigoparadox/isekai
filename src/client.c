
#include "client.h"

#include "parser.h"

void client_init( CLIENT* c ) {
    c->running = TRUE;
    c->buffer = bfromcstralloc( CLIENT_BUFFER_ALLOC, "" );
    c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
    c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
    c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
    c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
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

void client_join_channel( CLIENT* c, bstring name ) {
    bstring buffer = NULL;
    buffer = bfromcstr( "JOIN " );
    bconcat( buffer, name );
    client_send( c, buffer );
    bdestroy( buffer );
}

void client_send( CLIENT* c, bstring buffer ) {

    /* TODO: Make sure we're still connected. */

    bconchar( buffer, '\r' );
    bconchar( buffer, '\n' );
    connection_write_line( &(c->link), buffer );

    scaffold_print_debug( "Sent: %s", bdata( buffer ) );
}

void client_printf( CLIENT* c, const char* message, ... ) {
    bstring buffer = NULL;
    va_list varg;

    buffer = bfromcstralloc( strlen( message ), "" );
    scaffold_check_null( buffer );

    va_start( varg, message );
    scaffold_snprintf( buffer, message, varg );
    va_end( varg );

    if( 0 == scaffold_error ) {
        client_send( c, buffer );
    }

cleanup:
    bdestroy( buffer );
    return;
}
