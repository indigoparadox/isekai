
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

    scaffold_print_info( "Now listening for connections..." );

    listen( c->socket, 5 );

    while( c->listening ) {
        new_client = calloc( 1, sizeof( new_client ) );
        connection_init( new_client );

        /* Accept and verify the client. */
        new_client_addr_len = sizeof( c->address );
        new_client->socket = accept(
            c->socket, (struct sockaddr*)&(c->address), &new_client_addr_len
        );

        if( 0 > new_client->socket ) {
            scaffold_print_error( "Error while connecting: %d", new_client->socket );
            connection_free( new_client );
            continue;
        }

        /* The client seems OK, so launch the handler. */
        scaffold_print_info( "New client: %d", new_client->socket );

        pthread_create(
            &(new_client->thread), NULL, c->callback, c->arg
        );

    }

    return NULL;
}

void connection_listen(
    CONNECTION* c, uint16_t port, void* (*callback)( void* client ), void* arg
) {
    int bind_result;

    /* Setup and bind the port, first. */
    c->address.sin_family = AF_INET;
    c->address.sin_port = htons( port );
    c->address.sin_addr.s_addr = INADDR_ANY;

    bind_result = bind(
        c->socket, (struct sockaddr*)&(c->address), sizeof( c->address )
    );
    scaffold_check_negative( bind_result );

    /* If we could bind the port, then launch the serving connection. */
    c->callback = callback;
    c->arg = arg;
    c->listening = TRUE;
    pthread_create( &(c->thread), NULL, connection_listen_thread, c );

cleanup:

    return;
}

void connection_connect( CONNECTION* c, bstring server, uint16_t port ) {
    struct hostent* server_host;
    int connect_result;

    server_host = gethostbyname( bdata( server ) );
    scaffold_check_null( server_host );

    c->address.sin_family = AF_INET;
    bcopy(
        (char*)(server_host->h_addr),
        (char*)(c->address.sin_addr.s_addr),
        server_host->h_length
    );
    c->address.sin_port = htons( port );

    connect_result = connect( c->socket, &(c->address), sizeof( c->address ) );
    scaffold_check_negative( connect_result );

cleanup:

    return;
}
