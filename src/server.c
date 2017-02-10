
#include "server.h"

void server_init( SERVER* s ) {
    connection_init( &(s->server_connection) );
    vector_init( &(s->connections) );
    s->running = TRUE;

    s->sentinal = SENTINAL;
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

static void* server_accept_thread( void* arg ) {
    CONNECTION* n = (CONNECTION*)arg;
    SERVER* s = (SERVER*)(n->arg);
    bstring buffer;
    ssize_t last_read_count;

    assert( SENTINAL == s->sentinal );

    buffer = bfromcstr( "" );
    server_add_connection( s, n );

    /* Game logic starts here. */
    while( TRUE ) {
        bassigncstr( buffer, "" );
        last_read_count = connection_read_line( n, buffer );
        if( 0 >= last_read_count ) {
            continue;
        }

        scaffold_print_debug( "Server: Line received: %s\n", bdata( buffer ) );

        if( biseqcstrcaseless( buffer, "stop\n" ) ) {
            scaffold_print_debug( "Stop command received. Server stopping!\n" );
            server_stop( s );
        }

        usleep( 2000 );

        /* Make sure we're still live. */
        if( !s->running ) {
            break;
        }
    }

cleanup:

    return NULL;
}

void server_listen( SERVER* s ) {
    assert( SENTINAL == s->sentinal );
    s->server_connection.arg = s;
    connection_listen( &(s->server_connection), 33080, server_accept_thread, s );
    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        scaffold_print_error( "Unable to bind to specified port. Exiting.\n" );
        server_stop( s );
    }
}

void server_stop( SERVER* s ) {
    assert( SENTINAL == s->sentinal );
    s->running = FALSE;
}
