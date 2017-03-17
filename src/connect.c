
#define CONNECTION_C
#include "connect.h"

#include <string.h>
#include <stdlib.h>

#ifdef _GNU_SOURCE
#include <unistd.h>
#endif /* _WIN32 */

#ifdef USE_NETWORK
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>
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
      scaffold_print_error(
         &module, "Error while connecting on %d: %d\n", n->socket, errno );
      connection_cleanup( n );
      goto cleanup;
   }

   connected = TRUE;
#elif defined( USE_SYNCBUFF )
   connected = syncbuff_accept();
   if( TRUE == connected ) {
      n->socket = 2;
   }
#else
#error No IPC defined!
#endif /* USE_NETWORK */

   if( TRUE == connected ) {
      /* The client seems OK. */
      scaffold_print_info( &module, "New client: %d\n", n->socket );
   }

   /* TODO: Grab the remote hostname. */

#ifdef USE_NETWORK
cleanup:
#endif /* USE_NETWORK */
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
   result = listen( n->socket, 5 );
   scaffold_check_negative( result );
   listening = TRUE;
   scaffold_print_info( &module, "Now listening for connections...\n" );

cleanup:
#elif defined ( USE_SYNCBUFF )
   listening = syncbuff_listen();
   n->socket = 0;
#else
#error No IPC defined!
#endif /* USE_NETWORK */

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
   const char* server_c,
      * service_c;

   service = bformat( "%d", port );
   memset( &hints, 0, sizeof hints );
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   server_c = bdata( server );
   service_c = bdata( service );

   connect_result = getaddrinfo(
      bdata( server ), bdata( service ), &hints, &result
   );
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

SCAFFOLD_SIZE_SIGNED connection_write_line( CONNECTION* n, const bstring buffer, BOOL client ) {
   SCAFFOLD_SIZE_SIGNED sent = -1;
#ifdef USE_NETWORK
   const char* buffer_chars;
   SCAFFOLD_SIZE dest_socket;
   SCAFFOLD_SIZE buffer_len;
#elif defined( USE_SYNCBUFF )
   SCAFFOLD_SIZE syncbuff_previous_size;
#endif /* USE_NETWORK */

   scaffold_check_null( buffer );
   scaffold_check_null( n );

#ifdef USE_NETWORK
   buffer_chars = bdata( buffer );
   scaffold_assert( NULL != buffer_chars );

   dest_socket = n->socket;
   buffer_len = blength( buffer );
   scaffold_check_zero( dest_socket );

   sent = send( dest_socket, buffer_chars, buffer_len, MSG_NOSIGNAL );
#elif defined( USE_SYNCBUFF )
   syncbuff_previous_size =
      syncbuff_get_allocated( client ? SYNCBUFF_DEST_SERVER : SYNCBUFF_DEST_CLIENT );
   sent = syncbuff_write( buffer, client ? SYNCBUFF_DEST_SERVER : SYNCBUFF_DEST_CLIENT );
   scaffold_assert( 0 <= sent );
   if( syncbuff_previous_size < syncbuff_get_allocated( client ? SYNCBUFF_DEST_SERVER : SYNCBUFF_DEST_CLIENT ) ) {
      scaffold_print_debug(
         &module,
         "Syncbuff: Resized from %d to %d for: %s\n",
         syncbuff_previous_size,
         syncbuff_get_allocated( client ? SYNCBUFF_DEST_SERVER : SYNCBUFF_DEST_CLIENT ),
         client ? "Client" : "Server"
      );
   }
#else
#error No IPC defined!
#endif /* USE_NETWORK */
cleanup:
   return sent;
}

SCAFFOLD_SIZE_SIGNED connection_read_line( CONNECTION* n, bstring buffer, BOOL client ) {
	SCAFFOLD_SIZE_SIGNED total_read_count = 0;
#ifdef USE_NETWORK
	int bstr_res;
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
      bstr_res = bconchar( buffer, read_char );
      scaffold_check_nonzero( bstr_res );
   }
#elif defined( USE_SYNCBUFF )
   total_read_count = syncbuff_read( buffer, client ? SYNCBUFF_DEST_CLIENT : SYNCBUFF_DEST_SERVER );
   scaffold_assert( 0 <= total_read_count );
#else
#error No IPC defined!
#endif /* USE_NETWORK */
cleanup:
   return total_read_count;
}

void connection_assign_remote_name( CONNECTION* n, bstring buffer ) {
   int bstr_res;

   /* TODO: Figure out remote hostname. */
#if 0
   getpeername(
      n->socket,
      (struct sockaddr*)&(n->address),
      &(n->address_length)
   );
   bassignformat( buffer, "%s", inet_ntoa( n->address.sin_addr ) );
#endif
   bstr_res = bassignformat( buffer, "localhost" );
   scaffold_check_nonzero( bstr_res );

cleanup:
   return;
}

void connection_cleanup( CONNECTION* n ) {
   connection_cleanup_socket( n );
}
