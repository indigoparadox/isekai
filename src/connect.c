
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

#ifdef _GNU_SOURCE
#include <unistd.h>
#endif /* _WIN32 */

#ifdef USE_NETWORK
#endif /* USE_NETWORK */

#ifdef USE_SYNCBUFF
#include "ipc/syncbuff.h"
#endif /* USE_SYNCBUFF */

/** \brief Perform any system-wide initialization required by connections.
 */
void connection_setup() {
#ifdef USE_MBED_TLS
   int mbed_tls_ret;
   unsigned char* pers = NULL;
#endif /* USE_MBED_TLS */
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

cleanup:
   return;
}

void connection_init( struct CONNECTION* n ) {
#ifdef USE_MBED_TLS
   mbedtls_net_init( &(n->ssl_net) );
   mbedtls_ssl_init( &(n->ssl) );
   mbedtls_ssl_config_init( &(n->ssl_conf) );
   /* mbedtls_x509_crt_init( &cacert );
   mbedtls_ctr_drbg_init( &ctr_drbg ); */

   /*
   mbedtls_entropy_init( &entropy );
   if( (mbed_tls_ret = mbedtls_ctr_drbg_seed(
      &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers,
      strlen( pers ) ) ) != 0
   ) {
      scaffold_print_error(
         &module, "mbedtls_ctr_drbg_seed returned: %d\n", mbed_tls_ret
      );
      goto cleanup;
   }
   */
#endif /* USE_MBED_TLS */
}

#if 0
BOOL connection_connected( CONNECTION* n ) {
   return ipc_connected( struct CONNECTION* n );
}

BOOL connection_register_incoming( CONNECTION* n_server, CONNECTION* n ) {
   return ipc_accept( n_server, n );
}

BOOL connection_listen( CONNECTION* n, uint16_t port ) {
   return ipc_listen( n, port );
}

BOOL connection_connect( CONNECTION* n, const bstring server, uint16_t port ) {
   return ipc_connect( n, server, port );
}

SCAFFOLD_SIZE_SIGNED connection_write_line( CONNECTION* n, const bstring buffer, BOOL client ) {
   return ipc_write( n, buffer, client );
}

SCAFFOLD_SIZE_SIGNED connection_read_line( CONNECTION* n, bstring buffer, BOOL client ) {
   return ipc_read( n, buffer, client );
}
#endif

void connection_assign_remote_name( struct CONNECTION* n, bstring buffer ) {
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

//void connection_cleanup( struct CONNECTION* n ) {
   //connection_cleanup_socket( n );
//}
