#ifndef CONNECTION_H
#define CONNECTION_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"
#include "ref.h"

#ifdef USE_NETWORK
#include <netinet/in.h>
#endif /* USE_NETWORK */

#define CONNECTION_BUFFER_LEN 80

typedef struct _connection {
   REF refcount;
   int socket;
   BOOL listening;
   void* (*callback)( void* client );
   void* arg;
} CONNECTION;

void connection_register_incoming( CONNECTION* n_server, CONNECTION* n );
void connection_listen( CONNECTION* n, uint16_t port );
void connection_connect( CONNECTION* c, bstring server, uint16_t port );
void connection_write_line( CONNECTION* n, bstring buffer, BOOL client );
ssize_t connection_read_line( CONNECTION* c, bstring buffer, BOOL client );
void connection_lock( CONNECTION* c );
void connection_unlock( CONNECTION* c );
void connection_assign_remote_name( CONNECTION* n, bstring buffer );
void connection_cleanup( CONNECTION* c );

#endif /* CONNECTION_H */
