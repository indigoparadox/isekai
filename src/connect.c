
/* Need to include winsock stuff before windows.h in scaffold. */
#if defined( _WIN32 ) && defined( USE_NETWORK )
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "Mswsock.lib" )
#pragma comment( lib, "AdvApi32.lib" )
#endif /* _WIN32 && USE_NETWORK */

#define CONNECTION_C
#include "connect.h"

#include <string.h>
#include <stdlib.h>

#ifdef _GNU_SOURCE
#include <unistd.h>
#endif /* _WIN32 */

#ifdef USE_NETWORK
#ifdef C99
#endif /* C99 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <netdb.h>
#endif /* _WIN32 */
#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* _WIN32 */
#endif /* USE_NETWORK */

#ifdef USE_SYNCBUFF
#include "ipc/syncbuff.h"
#endif /* USE_SYNCBUFF */

/** \brief Perform any system-wide initialization required by connections.
 */
void connection_init() {
#if defined( _WIN32 ) && defined( USE_NETWORK )
   int result = 0;
   WSADATA wsa_data = { 0 };

   result = WSAStartup( MAKEWORD(2,2), &wsa_data );
   if( 0 != result ) {
      scaffold_print_error(
         &module,
         "WSAStartup failed with error: %d\n", result
      );
      scaffold_error = SCAFFOLD_ERROR_CONNECTION_CLOSED;
   }
#endif /* _WIN32 && USE_NETWORK */
}

static void connection_cleanup_socket( CONNECTION* n ) {
#ifdef USE_NETWORK
   if( 0 < n->socket ) {
      close( n->socket );
      n->socket = 0;
   }
#endif /* USE_NETWORK */
   n->socket = 0;
}

BOOL connection_register_incoming( CONNECTION* n_server, CONNECTION* n ) {
   BOOL connected = FALSE;
#ifdef USE_NETWORK
   unsigned int address_length;
   struct sockaddr_in address;
   #ifdef _WIN32
   u_long mode = 1;
   #endif /* _WIN32 */

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

   /* TODO: Check for error. */
#ifdef _WIN32
   ioctlsocket( n->socket, FIONBIO, &mode );
#else
   fcntl( n->socket, F_SETFL, O_NONBLOCK );
#endif /* _WIN32 */

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

#if defined( USE_NETWORK ) || defined( USE_LEGACY_NETWORK )
   int connect_result;
#ifdef _WIN32
   u_long mode = 1;
#endif /* _WIN32 */
#endif /* USE_NETWORK || USE_LEGACY_NETWORK */

#ifdef USE_NETWORK
   struct sockaddr_in address;

   n->socket = socket( AF_INET, SOCK_STREAM, 0 );
   scaffold_check_negative( n->socket );

   /* TODO: Check for error. */
#ifdef _WIN32
   ioctlsocket( n->socket, FIONBIO, &mode );
#else
   fcntl( n->socket, F_SETFL, O_NONBLOCK );
#endif /* _WIN32 */

   /* Setup and bind the port, first. */
   address.sin_family = AF_INET;
   address.sin_port = htons( port );
   address.sin_addr.s_addr = INADDR_ANY;

   connect_result = bind(
      n->socket, (struct sockaddr*)&address, sizeof( address )
   );
   scaffold_check_negative( connect_result );

   /* If we could bind the port, then launch the serving connection. */
   connect_result = listen( n->socket, 5 );
   scaffold_check_negative( connect_result );
   listening = TRUE;
   scaffold_print_debug( &module, "Now listening for connections...\n" );

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

#if defined( USE_NETWORK ) || defined( USE_LEGACY_NETWORK )
   int connect_result;
#ifdef _WIN32
   u_long mode = 1;
#endif /* _WIN32 */
#endif /* USE_NETWORK || USE_LEGACY_NETWORK */

#ifdef USE_LEGACY_NETWORK
   unsigned long ul_addr;
   struct sockaddr_in dest = { 0 };

   ul_addr = inet_addr( bdata( server ) );

   n->socket = socket(
      AF_INET,
      SOCK_STREAM,
      0
   );

   dest.sin_family = AF_INET;
   dest.sin_addr.s_addr = ul_addr;
   dest.sin_port = htons( port );

   connect_result = connect(
      n->socket,
      (const struct sockaddr*)&dest,
      sizeof( struct sockaddr_in )
   );

   scaffold_check_negative( connect_result );

   /* TODO: Check for error. */
#ifdef _WIN32
   ioctlsocket( n->socket, FIONBIO, &mode );
#else
   fcntl( n->socket, F_SETFL, O_NONBLOCK );
#endif /* _WIN32 */

cleanup:

   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      connection_cleanup_socket( n );
   }

#elif defined( USE_NETWORK )
   bstring service;
   struct addrinfo hints = { 0 },
         * result;

   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   service = bformat( "%d", port );

   connect_result = getaddrinfo(
      bdata( server ), bdata( service ), &hints, &result
   );
   scaffold_check_nonzero( connect_result );

   n->socket = socket(
      result->ai_family,
      result->ai_socktype,
      result->ai_protocol
   );

   connect_result = connect(
      n->socket,
      result->ai_addr,
      result->ai_addrlen
   );

   scaffold_check_negative( connect_result );

   /* TODO: Check for error. */
#ifdef _WIN32
   ioctlsocket( n->socket, FIONBIO, &mode );
#else
   fcntl( n->socket, F_SETFL, O_NONBLOCK );
#endif /* _WIN32 */

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
   SCAFFOLD_SIZE buffer_len,
      total_sent = 0;
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
   scaffold_check_zero( dest_socket, "Invalid destination socket." );

   do {
      sent = send(
         dest_socket,
         &(buffer_chars[total_sent]),
         buffer_len - total_sent,
#ifdef _WIN32
         0
#else
         MSG_NOSIGNAL
#endif /* _WIN32 */
      );
      total_sent += sent;
   } while( total_sent < buffer_len );

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
   SCAFFOLD_SIZE_SIGNED last_read_count = 0;
   char read_char = '\0';
#endif /* USE_NETWORK */

   scaffold_check_null( buffer );
   scaffold_check_null( n );

   scaffold_error = 0;

#ifdef USE_NETWORK
   while( '\n' != read_char ) {
      last_read_count = recv( n->socket, &read_char, 1, 0 );

      if( 0 == last_read_count ) {
         scaffold_error = SCAFFOLD_ERROR_CONNECTION_CLOSED;
         scaffold_print_info(
            &module, "Remote connection (%d) has been closed.\n", n->socket
         );
         close( n->socket );
         n->socket = 0;
         goto cleanup;
      }

      if( 0 > last_read_count ) {
         goto cleanup;
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
