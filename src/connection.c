
#include "connection.h"

void connection_init( CONNECTION* c ) {

    memset( c, '\0', sizeof( CONNECTION ) );

    c->socket = socket( AF_INET, SOCK_STREAM, 0 );
    scaffold_check_negative( c->socket );

    pthread_mutex_init( &(c->server_lock), NULL );

cleanup:

    return;
}

static void* connection_listen_thread( void* arg ) {
    CONNECTION* s = (CONNECTION*)arg;
    CONNECTION* new_client = NULL;
    unsigned int new_client_addr_len = 0;

    scaffold_print_info( "Now listening for connections..." );

    listen( s->socket, 5 );

    while( s->listening ) {
        /* This is a special case; don't init because we'll be using accept() */
        new_client = calloc( 1, sizeof( new_client ) );

        /* Accept and verify the client. */
        new_client_addr_len = sizeof( s->address );
        new_client->socket = accept(
            s->socket, (struct sockaddr*)&(s->address), &new_client_addr_len
        );

        if( 0 > new_client->socket ) {
            scaffold_print_error( "Error while connecting: %d", new_client->socket );
            connection_free( new_client );
            continue;
        }

        /* The client seems OK, so launch the handler. */
        scaffold_print_error( "New client: %d\n", new_client->socket );

        /* The client's arg will be the server connection's arg, which  *
         * should nominally be the server object.                       */
        new_client->arg = s->arg;

        pthread_create(
            &(new_client->thread), NULL, s->callback, new_client
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

    /* If we could bind the port, tserver_connectionhen launch the serving connection. */
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

    connect_result = connect( c->socket, (struct sockaddr*)&(c->address), sizeof( c->address ) );
    scaffold_check_negative( connect_result );

cleanup:

    return;
}

ssize_t connection_read_line( CONNECTION* c, bstring buffer ) {
    ssize_t last_read_count = 0,
        total_read_count = 0;
    char read_char = '\0';

    while( '\n' != read_char ) {
        last_read_count = read( c->socket, &read_char, 1 );

        //scaffold_print_debug( "Number: %d\n", last_read_count );

        if( 0 > last_read_count && EINTR == errno ) {
            continue;
        }
        scaffold_print_error( "errno: %d\n", errno );
        scaffold_check_negative( last_read_count );

        if( 0 == last_read_count ) {
            break;
        }

        /* No error and something was read, so add it to the string. */
        total_read_count++;
        bconchar( buffer, read_char );
    }

cleanup:

    return total_read_count;
}

void connection_lock( CONNECTION* c ) {
    pthread_mutex_lock( &(c->server_lock) );
}

void connection_unlock( CONNECTION* c ) {
    pthread_mutex_unlock( &(c->server_lock) );
}
