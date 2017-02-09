
#include "connection.h"

void connection_init( CONNECTION* c ) {

    memset( c, '\0', sizeof( CONNECTION ) );

    c->socket = socket( AF_INET, SOCK_STREAM, 0 );
    scaffold_check_negative( c->socket );

cleanup:

    return;
}

static void* connection_listen_thread( void* arg ) {
    CONNECTION* c = (CONNECTION*)arg;
    CONNECTION* new_client = NULL;
    unsigned int new_client_addr_len = 0;

    listen( c->socket, 5 );

    while( c->listening ) {
        new_client = calloc( 1, sizeof( new_client ) );
        connection_init( new_client );

        new_client_addr_len = sizeof( c->address );
        new_client->socket = accept(
            c->socket, (struct sockaddr*)&(c->address), &new_client_addr_len
        );

        if( 0 > new_client->socket ) {
            scaffold_print_error( "Error while connecting: %d", new_client->socket );
            connection_free( new_client );
        }

        pthread_create(
            &(new_client->thread), NULL, new_client->callback, new_client
        );

    }

    return NULL;
}

void connection_listen(
    CONNECTION* c, int port, void* (*callback)( void* client ), void* arg
) {
    int bind_result;

    c->address.sin_family = AF_INET;
    c->address.sin_port = port;
    c->address.sin_addr.s_addr = INADDR_ANY;

    bind_result = bind(
        c->socket, (struct sockaddr*)&(c->address), sizeof( c->address )
    );
    scaffold_check_negative( bind_result );

    c->callback = callback;
    c->arg = arg;
    c->listening = TRUE;
    pthread_create( &(c->thread), NULL, connection_listen_thread, c );

cleanup:

    return;
}
