
#include "server.h"

void server_init( SERVER* s ) {
    client_init( &(s->self) );
    vector_init( &(s->clients) );
    s->self.sentinal = SERVER_SENTINAL;
}

void server_cleanup( SERVER* s ) {
    /* TODO: Remove clients. */
    client_cleanup( &(s->self) );
}

void server_add_client( SERVER* s, CLIENT* n ) {
    connection_lock( &(s->self.link) );
    vector_add( &(s->clients), n );
    connection_unlock( &(s->self.link) );
}

CLIENT* server_get_client( SERVER* s, int index ) {
    CLIENT* c;
    connection_lock( &(s->self.link) );
    c = vector_get( &(s->clients), index );
    connection_unlock( &(s->self.link) );
    return c;
}

void server_listen( SERVER* s, int port ) {
    s->self.link.arg = s;
    connection_listen( &(s->self.link), port );
    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        scaffold_print_error( "Unable to bind to specified port. Exiting.\n" );
    }
}

void server_service_clients( SERVER* s ) {
    ssize_t last_read_count = 0;
    CLIENT* c = NULL;
    CONNECTION* n_client = NULL;
    int i = 0;

    /* Check for new clients. */
    n_client = connection_register_incoming( &(s->self.link) );
    if( NULL != n_client ) {
        c = calloc( 1, sizeof( CLIENT ) );
        memcpy( &(c->link), n_client, sizeof( CONNECTION ) );
        free( n_client ); /* Don't clean up, because data is still valid. */
        server_add_client( s, c );
    }

    /* Check for commands from existing clients. */
    for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
        bassigncstr( s->self.buffer, "" );

        c = vector_get( &(s->clients), i );

        last_read_count = connection_read_line( &(c->link), s->self.buffer );

        if( 0 >= last_read_count ) {
            /* TODO */
            continue;
        }

        scaffold_print_debug(
            "Server: Line received from %d: %s\n",
            c->link.socket, bdata( s->self.buffer )
        );

        parser_dispatch( s, c, s->self.buffer );
    }
}

void server_stop( SERVER* s ) {
    s->self.running = FALSE;
}
