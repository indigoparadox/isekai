
#include "connection.h"

void connection_init_socket( CONNECTION* n ) {
    n->socket = socket( AF_INET, SOCK_STREAM, 0 );
    scaffold_check_negative( n->socket );

cleanup:

    return;
}

CONNECTION* connection_register_incoming( CONNECTION* n_server ) {
    static CONNECTION* new_client = NULL;
    CONNECTION* return_client = NULL;
    unsigned int new_client_addr_len = 0;

    /* This is a special case; don't init because we'll be using accept()   *
     * We will only init this if it's NULL so that we're not constantly     *
     * allocing and deallocing memory.                                      */
    if( NULL == new_client ) {
        new_client = calloc( 1, sizeof( new_client ) );
    }

    /* Accept and verify the client. */
    new_client_addr_len = sizeof( n_server->address );
    new_client->socket = accept(
        n_server->socket, (struct sockaddr*)&(n_server->address),
        &new_client_addr_len
    );

    /* No connection incoming, this time! */
    if( 0 > new_client->socket && (EWOULDBLOCK == errno || EAGAIN == errno) ) {
        goto cleanup;
    }

    fcntl( new_client->socket, F_SETFL, O_NONBLOCK );

    if( 0 > new_client->socket ) {
        scaffold_print_error( "Error while connecting on %d: %d\n", new_client->socket, errno );
        connection_cleanup( new_client );
        free( new_client );
        goto cleanup;
    }

    /* The client seems OK, so launch the handler. */
    scaffold_print_info( "New client: %d\n", new_client->socket );
    return_client = new_client;
    new_client = NULL;

cleanup:

    return return_client;
}

void connection_listen( CONNECTION* n, uint16_t port ) {
    int bind_result;

    fcntl( n->socket, F_SETFL, O_NONBLOCK );

    /* Setup and bind the port, first. */
    n->address.sin_family = AF_INET;
    n->address.sin_port = htons( port );
    n->address.sin_addr.s_addr = INADDR_ANY;

    bind_result = bind(
        n->socket, (struct sockaddr*)&(n->address), sizeof( n->address )
    );
    scaffold_check_negative( bind_result );

    /* If we could bind the port, then launch the serving connection. */
    n->listening = TRUE;
    listen( n->socket, 5 );
    scaffold_print_info( "Now listening for connections..." );

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
        last_read_count = recv( n->socket, &read_char, 1, 0 );

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
}

void connection_unlock( CONNECTION* n ) {
}

void connection_cleanup( CONNECTION* n ) {
    if( 0 < n->socket ) {
        close( n->socket );
    }
    n->socket = 0;
}
