
#include "connection.h"

static void connection_cleanup_socket( CONNECTION* n ) {
    if( 0 < n->socket ) {
        close( n->socket );
    }
    n->socket = 0;
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
    int result;

    n->socket = socket( AF_INET, SOCK_STREAM, 0 );
    scaffold_check_negative( n->socket );

    fcntl( n->socket, F_SETFL, O_NONBLOCK );

    /* Setup and bind the port, first. */
    n->address.sin_family = AF_INET;
    n->address.sin_port = htons( port );
    n->address.sin_addr.s_addr = INADDR_ANY;

    result = bind(
        n->socket, (struct sockaddr*)&(n->address), sizeof( n->address )
    );
    scaffold_check_negative( result );

    /* If we could bind the port, then launch the serving connection. */
    scaffold_print_info( "Now listening for connections..." );
    result = listen( n->socket, 5 );
    scaffold_check_negative( result );

cleanup:

    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        connection_cleanup_socket( n );
    } else {
        n->listening = TRUE;
    }

    return;
}

void connection_connect( CONNECTION* n, bstring server, uint16_t port ) {
    int connect_result;
    struct addrinfo hints,
        * result;
    bstring service;

    service = bformat( "%d", port );
    memset( &hints, 0, sizeof hints );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    connect_result = getaddrinfo( bdata( server ), bdata( service ), &hints, &result );
    scaffold_check_nonzero( connect_result );

    n->socket =
        socket( result->ai_family, result->ai_socktype, result->ai_protocol );

    connect_result = connect( n->socket, result->ai_addr, result->ai_addrlen);
    scaffold_check_negative( connect_result );

cleanup:

    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        connection_cleanup_socket( n );
    }

    bdestroy( service );
    freeaddrinfo( result );

    return;
}

void connection_write_line( CONNECTION* n, bstring buffer ) {
    send( n->socket, bdata( buffer ), blength( buffer ), 0 );
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

    return total_read_count;
}

void connection_lock( CONNECTION* n ) {
}

void connection_unlock( CONNECTION* n ) {
}

void connection_cleanup( CONNECTION* n ) {
    connection_cleanup_socket( n );
}
