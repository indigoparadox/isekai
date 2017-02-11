
#include "server.h"

void server_init( SERVER* s ) {
    client_init( &(s->self) );
    vector_init( &(s->clients) );
    s->self.sentinal = SERVER_SENTINAL;
}

void server_add_connection( SERVER* s, CONNECTION* n ) {
    connection_lock( &(s->self.link) );
    vector_add( &(s->clients), n );
    connection_unlock( &(s->self.link) );
}

CONNECTION* server_get_connection( SERVER* s, int index ) {
    CONNECTION* n;
    connection_lock( &(s->self.link) );
    n = vector_get( &(s->clients), index );
    connection_unlock( &(s->self.link) );
    return n;
}

void server_listen( SERVER* s ) {
    s->self.link.arg = s;
    connection_listen( &(s->self.link), 33080 );
    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        scaffold_print_error( "Unable to bind to specified port. Exiting.\n" );
        server_stop( s );
    }
}

void server_service_clients( SERVER* s ) {
    ssize_t last_read_count = 0;
    CONNECTION* n = NULL;
    int i = 0;

    n = connection_register_incoming( &(s->self.link) );
    if( NULL != n ) {
        server_add_connection( s, n );
    }

    for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
        bassigncstr( s->self.buffer, "" );

        n = vector_get( &(s->clients), i );

        last_read_count = connection_read_line( n, s->self.buffer );

        if( 0 >= last_read_count ) {
            /* TODO */
            continue;
        }

        scaffold_print_debug(
            "Server: Line received from %d: %s\n", n->socket, bdata( s->self.buffer )
        );

        parser_dispatch( s, s->self.buffer );
    }
}

void server_stop( SERVER* s ) {
    s->self.running = FALSE;
}
