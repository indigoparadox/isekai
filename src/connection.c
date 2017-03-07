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

#ifdef USE_SYNCBUFF
#include "ipc/syncbuff.h"
#endif /* USE_SYNCBUFF */

static void connection_cleanup_socket( CONNECTION* n ) {
#ifdef USE_NETWORK
   if( 0 < n->socket ) {
      close( n->socket );
   }
#endif /* USE_NETWORK */
   n->socket = 0;
}

BOOL connection_register_incoming( CONNECTION* n_server, CONNECTION* n ) {
   BOOL connected = FALSE;
#ifdef USE_NETWORK
   unsigned int address_length;
   struct sockaddr_in address;

   /* Accept and verify the client. */
   address_length = sizeof( address );
   n->socket = accept(
      n_server->socket, (struct sockaddr*)&address,
      &address_length
   );

   /* No connection incoming, this time! */
   if( 0 > n->socket && (EWOULDBLOCK == errno || EAGAIN == errno) ) {
      goto cleanup;
   }

   fcntl( n->socket, F_SETFL, O_NONBLOCK );

   if( 0 > n->socket ) {
      scaffold_print_error( "Error while connecting on %d: %d\n", n->socket,
                            errno );
      connection_cleanup( n );
      goto cleanup;
   }

   connected = TRUE;
#elif defined( USE_SYNCBUFF )
   connected = syncbuff_accept();
   n->socket = 2;
#else
#error No IPC defined!
#endif /* USE_NETWORK */

   if( TRUE == connected ) {
      /* The client seems OK. */
      scaffold_print_info( "New client: %d\n", n->socket );
   }

   /* TODO: Grab the remote hostname. */

cleanup:
   return connected;
}

BOOL connection_listen( CONNECTION* n, uint16_t port ) {
   BOOL listening = FALSE;
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
   scaffold_print_info( "Now listening for connections...\n" );
   result = listen( n->socket, 5 );
   scaffold_check_negative( result );
   listening = TRUE;
#elif defined ( USE_SYNCBUFF )
   listening = syncbuff_listen();
   n->socket = 0;
#else
#error No IPC defined!
#endif /* USE_NETWORK */

cleanup:

   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      connection_cleanup_socket( n );
   } else {
      n->listening = TRUE;
   }

   return listening;
}

BOOL connection_connect( CONNECTION* n, const bstring server, uint16_t port ) {
   BOOL connected = FALSE;
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
   connected = TRUE;
cleanup:

   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      connection_cleanup_socket( n );
   }

   bdestroy( service );
   freeaddrinfo( result );
#elif defined( USE_SYNCBUFF )
   connected = syncbuff_connect();
   n->socket = 1;
#else
#error No IPC defined!
#endif /* USE_NETWORK */

   return connected;
}

ssize_t connection_write_line( CONNECTION* n, const bstring buffer, BOOL client ) {
   const char* buffer_chars;
   ssize_t sent = -1;
#ifdef USE_NETWORK
   size_t dest_socket;
   size_t buffer_len;
#endif /* USE_NETWORK */

   scaffold_check_null( buffer );
   scaffold_check_null( n );

   buffer_chars = bdata( buffer );
   assert( NULL != buffer_chars );

#ifdef USE_NETWORK
   dest_socket = n->socket;
   buffer_len = blength( buffer );
   assert( 0 != dest_socket );

   sent = send( dest_socket, buffer_chars, buffer_len, MSG_NOSIGNAL );
#elif defined( USE_SYNCBUFF )
   sent = syncbuff_write( buffer, client ? SYNCBUFF_DEST_SERVER : SYNCBUFF_DEST_CLIENT );
#else
#error No IPC defined!
#endif /* USE_NETWORK */
cleanup:
   return sent;
}

ssize_t connection_read_line( CONNECTION* n, bstring buffer, BOOL client ) {
   ssize_t total_read_count = 0;
#ifdef USE_NETWORK
   ssize_t last_read_count = 0;
   char read_char = '\0';
#endif /* USE_NETWORK */

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
#elif defined( USE_SYNCBUFF )
   total_read_count = syncbuff_read( buffer, client ? SYNCBUFF_DEST_CLIENT : SYNCBUFF_DEST_SERVER );
#else
#error No IPC defined!
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
