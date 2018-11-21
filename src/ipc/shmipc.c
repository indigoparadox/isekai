
#define IPC_C
#include "../ipc.h"

struct CONNECTION {
   VBOOL connected;
};

void ipc_setup() {
}

void ipc_shutdown() {
}

struct CONNECTION* ipc_alloc() {
   return mem_alloc( 1, struct CONNECTION );
}

void ipc_free( struct CONNECTION** n ) {
}

VBOOL ipc_connected( struct CONNECTION* n ) {
   return n->connected;
}

VBOOL ipc_listen( struct CONNECTION* n, uint16_t port ) {
}

VBOOL ipc_connect( struct CONNECTION* n, const bstring server, uint16_t port ) {
   n->connected = VTRUE;
}

VBOOL ipc_accept( struct CONNECTION* n_server, struct CONNECTION* n ) {
}

/*
SCAFFOLD_SIZE_SIGNED ipc_write( struct CONNECTION* n, const bstring buffer ) {
}

SCAFFOLD_SIZE_SIGNED ipc_read( struct CONNECTION* n, bstring buffer ) {
}
*/

void ipc_stop( struct CONNECTION* n ) {
}

IPC_END ipc_get_type( struct CONNECTION* n ) {
}

uint16_t ipc_get_port( struct CONNECTION* n ) {
   return 0;
}

VBOOL ipc_is_listening( struct CONNECTION* n ) {
}
