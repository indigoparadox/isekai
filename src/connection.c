
#include "connection.h"

void connection_init( CONNECTION* n ) {

    memset( n, '\0', sizeof( CONNECTION ) );

    pthread_mutex_init( &(n->server_lock), NULL );

    n->socket = socket( AF_INET, SOCK_STREAM, 0 );
    scaffold_check_negative( n->socket );

    n->sentinal1 = SENTINAL;
    n->sentinal2 = SENTINAL;
    n->sentinal3 = SENTINAL;
    n->sentinal4 = SENTINAL;
    n->sentinal5 = SENTINAL;
    n->sentinal6 = SENTINAL;
    n->sentinal7 = SENTINAL;

cleanup:

    return;
}

static void* connection_listen_thread( void* arg ) {
    CONNECTION* n = (CONNECTION*)arg;
    CONNECTION* new_client = NULL;
    unsigned int new_client_addr_len = 0;
    pthread_t tmp_thread;
    void* (*tmp_callback)( void* );

    scaffold_print_info( "Now listening for connections..." );

    listen( n->socket, 5 );

    while( n->listening ) {
        /* This is a special case; don't init because we'll be using accept() */
        new_client = calloc( 1, sizeof( new_client ) );

        connection_init( new_client );

        /* The client's arg will be the server connection's arg, which  *
         * should nominally be the server object.                       */
        new_client->arg = n;

        /* Accept and verify the client. */
        new_client_addr_len = sizeof( n->address );
        new_client->socket = accept(
            n->socket, (struct sockaddr*)&(n->address), &new_client_addr_len
        );

        if( 0 > new_client->socket ) {
            scaffold_print_error( "Error while connecting: %d", new_client->socket );
            connection_cleanup( new_client );
            free( new_client );
            continue;
        }

        /* The client seems OK, so launch the handler. */
        scaffold_print_info( "New client: %d\n", new_client->socket );

        tmp_callback = n->callback;

        pthread_create(
            &tmp_thread, NULL, tmp_callback, new_client
        );

    }

    return NULL;
}

void connection_listen(
    CONNECTION* n, uint16_t port, void* (*callback)( void* client ), void* arg
) {
    int bind_result;

    /* Setup and bind the port, first. */
    n->address.sin_family = AF_INET;
    n->address.sin_port = htons( port );
    n->address.sin_addr.s_addr = INADDR_ANY;

    bind_result = bind(
        n->socket, (struct sockaddr*)&(n->address), sizeof( n->address )
    );
    scaffold_check_negative( bind_result );

    /* If we could bind the port, tserver_connectionhen launch the serving connection. */
    n->callback = callback;
    //n->arg = arg;
    n->listening = TRUE;
    pthread_create( &(n->thread), NULL, connection_listen_thread, n );

cleanup:

    return;
}

void connection_connect( CONNECTION* n, bstring server, uint16_t port ) {
    struct hostent* server_host;
    int connect_result;

    server_host = gethostbyname( bdata( server ) );
    scaffold_check_null( server_host );

    n->address.sin_family = AF_INET;
    bcopy(
        (char*)(server_host->h_addr),
        (char*)(n->address.sin_addr.s_addr),
        server_host->h_length
    );
    n->address.sin_port = htons( port );

    connect_result = connect( n->socket, (struct sockaddr*)&(n->address), sizeof( n->address ) );
    scaffold_check_negative( connect_result );

cleanup:

    return;
}

ssize_t connection_read_line( CONNECTION* n, bstring buffer ) {
    ssize_t last_read_count = 0,
        total_read_count = 0;
    char read_char = '\0';

    while( '\n' != read_char ) {
        last_read_count = recv( n->socket, &read_char, 1, MSG_WAITALL );

        if( 0 >= last_read_count ) {
            break;
        }

        /* No error and something was read, so add it to the string. */
        total_read_count++;
        bconchar( buffer, read_char );
    }

cleanup:

    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        scaffold_print_error( "Connection: Error while reading line: %d\n", errno );
    }

    return total_read_count;
}

void connection_lock( CONNECTION* n ) {
    pthread_mutex_lock( &(n->server_lock) );
}

void connection_unlock( CONNECTION* n ) {
    pthread_mutex_unlock( &(n->server_lock) );
}

void connection_cleanup( CONNECTION* n ) {
    if( 0 < n->socket ) {
        close( n->socket );
    }
    n->socket = 0;
}
