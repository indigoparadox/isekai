#include "connection.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef USE_NETWORK
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#endif /* USE_NETWORK */

#ifndef USE_NETWORK

#include "vector.h"

typedef struct {
    int socket_src;
    int socket_dest;
    bstring contents;
} CONNECTION_ENVELOPE;

static const struct tagbstring str_connect = bsStatic( "%%CONNECT%%" );
static VECTOR envelopes = { 0 };
static uint16_t last_socket = 0;
static int32_t server_socket = -1;

static int fake_listen() {
    vector_init( &envelopes );
    server_socket = last_socket++;
    return server_socket;
}

static int fake_accept( int socket_dest ) {
    CONNECTION_ENVELOPE* mailbox = NULL;
    int socket_out = -1;

    if( 0 > socket_dest ) {
        goto cleanup;
    }

    mailbox = vector_get( &envelopes, 0 );
    if( NULL != mailbox && 0 == bstrcmp(
        &str_connect,
        mailbox->contents
    ) && socket_dest == mailbox->socket_dest ) {
        socket_out = mailbox->socket_src;
        bdestroy( mailbox->contents );
        free( mailbox );
        vector_delete( &envelopes, 0 );
    }

cleanup:
    return socket_out;
}

static void fake_send( int socket_src, int socket_dest, bstring message ) {
    CONNECTION_ENVELOPE* outgoing = NULL;
    BOOL ok = FALSE;

    outgoing = calloc( 1, sizeof( CONNECTION_ENVELOPE ) );
    scaffold_check_null( outgoing );
    outgoing->contents = bstrcpy( message );
    scaffold_check_null( outgoing->contents );
    outgoing->socket_src = socket_src;
    outgoing->socket_dest = socket_dest;

    ok = TRUE;
    vector_add( &envelopes, outgoing );

cleanup:
    if( TRUE != ok ) {
        free( outgoing );
    }
    return;
}

static void fake_connect( int socket_src ) {
    bstring buffer = NULL;
    buffer = bstrcpy( &str_connect );

    fake_send( socket_src, server_socket, buffer );

    bdestroy( buffer );
}

static int fake_read( int socket_dest, bstring buffer ) {
    CONNECTION_ENVELOPE* mailbox = NULL;
    int length_out = 0;

    mailbox = vector_get( &envelopes, 0 );
    if( NULL != mailbox && socket_dest == mailbox->socket_dest ) {
        scaffold_check_null( mailbox->contents );
        bassign( buffer, mailbox->contents );
        length_out = blength( buffer );
        bdestroy( mailbox->contents );
        free( mailbox );
        vector_delete( &envelopes, 0 );
    } else {
        bassigncstr( buffer, "" );
    }

cleanup:
    return length_out;
}

#endif /* USE_NETWORK */


static void connection_cleanup_socket( CONNECTION* n ) {
   if( 0 < n->socket ) {
      close( n->socket );
   }
   n->socket = 0;
}

CONNECTION* connection_register_incoming( CONNECTION* n_server ) {
   static CONNECTION* new_client = NULL;
   CONNECTION* return_client = NULL;
#ifdef USE_NETWORK
   unsigned int address_length;
   struct sockaddr_in address;
#endif /* USE_NETWORK */

   /* This is a special case; don't init because we'll be using accept()   *
    * We will only init this if it's NULL so that we're not constantly     *
    * allocing and deallocing memory.                                      */
   if( NULL == new_client ) {
      new_client = calloc( 1, sizeof( new_client ) );
   }

#ifdef USE_NETWORK
   /* Accept and verify the client. */
   address_length = sizeof( address );
   new_client->socket = accept(
                           n_server->socket, (struct sockaddr*)&address,
                           &address_length
                        );

   /* No connection incoming, this time! */
   if( 0 > new_client->socket && (EWOULDBLOCK == errno || EAGAIN == errno) ) {
      goto cleanup;
   }

   fcntl( new_client->socket, F_SETFL, O_NONBLOCK );

   if( 0 > new_client->socket ) {
      scaffold_print_error( "Error while connecting on %d: %d\n", new_client->socket,
                            errno );
      connection_cleanup( new_client );
      free( new_client );
      goto cleanup;
   }
#else
   new_client->socket = fake_accept( n_server->socket );

   /* No connection incoming, this time! */
   if( 0 > new_client->socket ) {
      goto cleanup;
   }
#endif /* USE_NETWORK */

   /* The client seems OK. */
   scaffold_print_info( "New client: %d\n", new_client->socket );
   return_client = new_client;
   new_client = NULL;

   /* TODO: Grab the remote hostname. */

cleanup:

   return return_client;
}

void connection_listen( CONNECTION* n, uint16_t port ) {
#ifdef USE_NETWORK
   int result;
   struct sockaddr_in address;

   n->socket = socket( AF_INET, SOCK_STREAM, 0 );
   scaffold_check_negative( n->socket );

   fcntl( n->socket, F_SETFL, O_NONBLOCK );

   /* Setup and bind the port, first. */
   address.sin_family = AF_INET;
   address.sin_port = htons( port );
   address.sin_addr.s_addr = INADDR_ANY;

   result = bind(
               n->socket, (struct sockaddr*)&address, sizeof( address )
            );
   scaffold_check_negative( result );

   /* If we could bind the port, then launch the serving connection. */
   scaffold_print_info( "Now listening for connections..." );
   result = listen( n->socket, 5 );
   scaffold_check_negative( result );
#else
    n->socket = fake_listen();
    scaffold_check_negative( n->socket );
#endif /* USE_NETWORK */

cleanup:

   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      connection_cleanup_socket( n );
   } else {
      n->listening = TRUE;
   }

   return;
}

void connection_connect( CONNECTION* n, bstring server, uint16_t port ) {
#ifdef USE_NETWORK
   int connect_result;
   struct addrinfo hints,
         * result;
   bstring service;

   service = bformat( "%d", port );
   memset( &hints, 0, sizeof hints );
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   connect_result = getaddrinfo( bdata( server ), bdata( service ), &hints,
                                 &result );
   scaffold_check_nonzero( connect_result );

   n->socket =
      socket( result->ai_family, result->ai_socktype, result->ai_protocol );

   connect_result = connect( n->socket, result->ai_addr, result->ai_addrlen);
   scaffold_check_negative( connect_result );

   fcntl( n->socket, F_SETFL, O_NONBLOCK );

cleanup:

   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      connection_cleanup_socket( n );
   }

   bdestroy( service );
   freeaddrinfo( result );
#else
    n->socket = last_socket++;
    fake_connect( n->socket );
#endif /* USE_NETWORK */

   return;
}

void connection_write_line( CONNECTION* n, bstring buffer, BOOL client ) {
   scaffold_check_null( buffer );
   scaffold_check_null( n );
#ifdef USE_NETWORK
   send( n->socket, bdata( buffer ), blength( buffer ), 0 );
#else
   if( TRUE == client ) {
      fake_send( n->socket, server_socket, buffer );
   } else {
      fake_send( server_socket, n->socket, buffer );
   }
#endif /* USE_NETWORK */
cleanup:
   return;
}

ssize_t connection_read_line( CONNECTION* n, bstring buffer, BOOL client ) {
   ssize_t last_read_count = 0,
           total_read_count = 0;
   char read_char = '\0';

   scaffold_check_null( buffer );
   scaffold_check_null( n );

#ifdef USE_NETWORK
   while( '\n' != read_char ) {
      last_read_count = recv( n->socket, &read_char, 1, 0 );

      if( 0 >= last_read_count ) {
         break;
      }

      /* No error and something was read, so add it to the string. */
      total_read_count++;
      bconchar( buffer, read_char );
   }
#else
   if( client ) {
      total_read_count = fake_read( n->socket, buffer );
   } else {
      total_read_count = fake_read( server_socket, buffer );
   }
#endif /* USE_NETWORK */
cleanup:
   return total_read_count;
}

void connection_assign_remote_name( CONNECTION* n, bstring buffer ) {
   /* TODO: Figure out remote hostname. */
#if 0
   getpeername(
      n->socket,
      (struct sockaddr*)&(n->address),
      &(n->address_length)
   );
   bassignformat( buffer, "%s", inet_ntoa( n->address.sin_addr ) );
#endif
   bassignformat( buffer, "localhost" );
}

void connection_cleanup( CONNECTION* n ) {
   connection_cleanup_socket( n );
}
