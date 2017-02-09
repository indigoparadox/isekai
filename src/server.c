
#include "server.h"

void server_init( SERVER* s ) {
    connection_init( &(s->server_connection) );
    vector_init( &(s->connections) );
}

static void* server_accept_thread( void* arg ) {
    CONNECTION* c = (CONNECTION*)arg;
    SERVER* s = c->arg;

    vector_add( &(s->connections), c );

    /* Game logic starts here. */

cleanup:

    return NULL;
}

void server_listen( SERVER* s ) {
    connection_listen( &(s->server_connection), 33080, server_accept_thread, s );
}
