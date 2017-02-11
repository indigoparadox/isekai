
#include "server.h"

void server_init( SERVER* s ) {
    connection_init_socket( &(s->server_connection) );
    vector_init( &(s->connections) );
    s->running = TRUE;
    s->buffer = bfromcstr( "" );
    s->sentinal = SERVER_SENTINAL;
}

void server_add_connection( SERVER* s, CONNECTION* n ) {
    connection_lock( &(s->server_connection) );
    vector_add( &(s->connections), n );
    connection_unlock( &(s->server_connection) );
}

CONNECTION* server_get_connection( SERVER* s, int index ) {
    CONNECTION* n;
    connection_lock( &(s->server_connection) );
    n = vector_get( &(s->connections), index );
    connection_unlock( &(s->server_connection) );
    return n;
}

void server_listen( SERVER* s ) {
    s->server_connection.arg = s;
    connection_listen( &(s->server_connection), 33080 );
    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        scaffold_print_error( "Unable to bind to specified port. Exiting.\n" );
        server_stop( s );
    }
}

void server_service_clients( SERVER* s ) {
    ssize_t last_read_count = 0;
    CONNECTION* n = NULL;
    int i = 0;

    n = connection_register_incoming( &(s->server_connection) );
    if( NULL != n ) {
        server_add_connection( s, n );
    }

    for( i = 0 ; vector_count( &(s->connections) ) > i ; i++ ) {
        bassigncstr( s->buffer, "" );

        n = vector_get( &(s->connections), i );

        last_read_count = connection_read_line( n, s->buffer );

        if( 0 >= last_read_count ) {
            /* TODO */
            continue;
        }

        scaffold_print_debug(
            "Server: Line received from %d: %s\n", n->socket, bdata( s->buffer )
        );

        parser_dispatch( s, s->buffer );
    }
}

void server_stop( SERVER* s ) {
    s->running = FALSE;
}
