
#define IPC_C
#include "../ipc.h"

#include "../connect.h"

/* This block is really finicky in terms of order. */
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

#ifdef USE_MBED_TLS
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
#endif /* USE_MBED_TLS */

struct CONNECTION {
   BOOL listening;
   void* (*callback)( void* client );
   void* arg;
#ifdef USE_MBED_TLS
   mbedtls_net_context ssl_net;
   mbedtls_ssl_context ssl;
   mbedtls_ssl_config ssl_conf;
#else
   int socket;
#endif /* USE_MBED_TLS */
};

struct CONNECTION* ipc_alloc() {
   return mem_alloc( 1, struct CONNECTION );
}

void ipc_free( struct CONNECTION** n ) {
   mem_free( *n );
   n = NULL;
}

BOOL ipc_connected( struct CONNECTION* n ) {
#ifdef USE_MBED_TLS
#error TODO
#else
   if( 0 < n->socket ) {
      return TRUE;
   } else {
      return FALSE;
   }
#endif /* USE_MBED_TLS */
}

BOOL ipc_connect( struct CONNECTION* n, const bstring server, uint16_t port ) {
   BOOL connected = FALSE;
   int connect_result;
#ifdef _WIN32
   u_long mode = 1;
#endif /* _WIN32 */

#ifdef USE_MBED_TLS
   int mbed_tls_ret;
   bstring b_port = NULL;

   b_port = bformat( "%d", port );

   if( (mbed_tls_ret = mbedtls_net_connect(
      &(n->ssl_net), bdata( server ), bdata( b_port ), MBEDTLS_NET_PROTO_TCP
   )) != 0 ) {
      scaffold_print_error( &module, "mbedtls_net_connect returned: %d", mbed_tls_ret );
      goto cleanup;
   }

cleanup:
   bdestroy( b_port );
#elif defined( USE_LEGACY_NETWORK )
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
      //connection_cleanup_socket( n );
   }

   bdestroy( service );
   freeaddrinfo( result );
#endif /* USE_MBED_TLS || USE_LEGACY_NETWORK || USE_NETWORK */

   return connected;
}

SCAFFOLD_SIZE_SIGNED ipc_write( struct CONNECTION* n, const bstring buffer, BOOL client ) {
   SCAFFOLD_SIZE_SIGNED sent = -1;
   const char* buffer_chars;
   SCAFFOLD_SIZE dest_socket;
   SCAFFOLD_SIZE buffer_len,
      total_sent = 0;

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

cleanup:
   return sent;
}

BOOL ipc_accept( struct CONNECTION* n_server, struct CONNECTION* n ) {
   BOOL connected = FALSE;
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
      ipc_stop( n );
      goto cleanup;
   }

   connected = TRUE;

cleanup:
   return connected;
}

BOOL ipc_listen( struct CONNECTION* n, uint16_t port ) {
   int connect_result;
#ifdef _WIN32
   u_long mode = 1;
#endif /* _WIN32 */

#ifdef USE_MBED_TLS
   bstring b_port = NULL;

   b_port = bformat( "%d", port );
   scaffold_check_null( b_port );

   mbedtls_net_bind( &(n->ssl_net), "0.0.0.0", bdata( b_port ), MBEDTLS_NET_PROTO_TCP )
#else
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
#endif /* USE_MBED_TLS */
   scaffold_check_negative( connect_result );

   /* If we could bind the port, then launch the serving connection. */
   connect_result = listen( n->socket, 5 );
   scaffold_check_negative( connect_result );
   n->listening = TRUE;
   scaffold_print_debug( &module, "Now listening for connections...\n" );

cleanup:
   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      // FIXME connection_cleanup_socket( n );
   }
}

SCAFFOLD_SIZE_SIGNED ipc_read( struct CONNECTION* n, bstring buffer, BOOL client ) {
	SCAFFOLD_SIZE_SIGNED total_read_count = 0;
	int bstr_res;
   SCAFFOLD_SIZE_SIGNED last_read_count = 0;
   char read_char = '\0';

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

cleanup:
   return total_read_count;
}

void ipc_stop( struct CONNECTION* n ) {
   if( FALSE != ipc_connected( n ) ) {
#ifdef USE_MBED_TLS
      mbedtls_net_free( &(n->ssl_net) );
#else
      close( n->socket );
      n->socket = 0;
#endif /* USE_MBED_TLS */
   }
}
