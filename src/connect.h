#ifndef CONNECTION_H
#define CONNECTION_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"
#include "ref.h"

#define CONNECTION_BUFFER_LEN 80

#ifdef USE_MBED_TLS
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#endif /* USE_MBED_TLS */

typedef struct _CONNECTION {
   struct REF refcount;
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
} CONNECTION;

void connection_init( CONNECTION* n );
void connection_setup();
BOOL connection_connected( CONNECTION* n );
BOOL connection_register_incoming( CONNECTION* n_server, CONNECTION* n );
BOOL connection_listen( CONNECTION* n, uint16_t port );
BOOL connection_connect( CONNECTION* c, const bstring server, uint16_t port );
SCAFFOLD_SIZE_SIGNED connection_write_line( CONNECTION* n, const bstring buffer, BOOL client );
SCAFFOLD_SIZE_SIGNED connection_read_line( CONNECTION* c, bstring buffer, BOOL client );
void connection_lock( CONNECTION* c );
void connection_unlock( CONNECTION* c );
void connection_assign_remote_name( CONNECTION* n, const bstring buffer );
void connection_cleanup( CONNECTION* c );

#ifdef CONNECTION_C
SCAFFOLD_MODULE( "connect.c" );
#endif /* CONNECTION_C */

#endif /* CONNECTION_H */
