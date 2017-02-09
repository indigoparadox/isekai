
#include "server.h"

void server_init( SERVER* s ) {
    connection_init( &(s->server_connection) );
    vector_init( &(s->connections) );
    s->running = TRUE;
}

static void* server_accept_thread( void* arg ) {
    CONNECTION* c = (CONNECTION*)arg;
    SERVER* s = (SERVER*)c->arg;
    bstring buffer;
    uint32_t last_read_count = 0;

    //connection_lock( &(s->server_connection) );
    vector_add( &(s->connections), c );
    //connection_unlock( &(s->server_connection) );
    buffer = bfromcstr( "" );

    /* Game logic starts here. */
    while( TRUE ) {
        bassigncstr( buffer, "" );
        last_read_count = connection_read_line( c, buffer );

        scaffold_print_error( "Line: %s", bdata( buffer ) );

        if( biseqcstrcaseless( buffer, "stop\n" ) ) {
            scaffold_print_error( "Stopping!" );
            server_stop( s );
        }

        connection_lock( &(s->server_connection) );
        if( !s->running ) {
            break;
        }
        connection_unlock( &(s->server_connection) );
    }

cleanup:

    return NULL;
}

void server_listen( SERVER* s ) {
    connection_listen( &(s->server_connection), 33080, server_accept_thread, s );
}

void server_stop( SERVER* s ) {
    connection_lock( &(s->server_connection) );
    s->running = FALSE;
    connection_unlock( &(s->server_connection) );
}
