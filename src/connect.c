

#define CONNECTION_C
#include "connect.h"


#ifdef USE_NETWORK
#endif /* USE_NETWORK */

#ifdef USE_SYNCBUFF
#include "ipc/syncbuff.h"
#endif /* USE_SYNCBUFF */

/** \brief Perform any system-wide initialization required by connections.
 */
void connection_setup() {
}

void connection_init( struct CONNECTION* n ) {
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
