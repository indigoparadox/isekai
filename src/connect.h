#ifndef CONNECTION_H
#define CONNECTION_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"
#include "ref.h"

#define CONNECTION_BUFFER_LEN 80

typedef struct _CONNECTION {
   struct REF refcount;
   int socket;
   BOOL listening;
   void* (*callback)( void* client );
   void* arg;
} CONNECTION;

void connection_init();
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
