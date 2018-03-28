
#define IPC_C
#include "../ipc.h"

typedef enum SYNCBUFF_STATE {
   SYNCBUFF_STATE_DISCONNECTED,
   SYNCBUFF_STATE_LISTENING,
   SYNCBUFF_STATE_CONNECTED
} SYNCBUFF_STATE;

struct CONNECTION {
   void* (*callback)( void* client );
   void* arg;
   IPC_END type;
   SYNCBUFF_STATE state;
   SCAFFOLD_SIZE client_count;
   BOOL local;
} CONNECTION;

#define SYNCBUFF_LINE_DEFAULT 255
#define SYNCBUFF_LINES_INITIAL 2

static SCAFFOLD_SIZE syncbuff_size[2];
static bstring* syncbuff_lines[2];
static SCAFFOLD_SIZE syncbuff_count[2];

SCAFFOLD_SIZE syncbuff_get_count( struct CONNECTION* n ) {
   return syncbuff_count[n->type];
}

SCAFFOLD_SIZE syncbuff_get_allocated( struct CONNECTION* n ) {
   return syncbuff_size[n->type];
}

static void syncbuff_alloc( IPC_END dest ) {
   SCAFFOLD_SIZE i;

   syncbuff_size[dest] = SYNCBUFF_LINES_INITIAL;
   syncbuff_count[dest] = 0;
   syncbuff_lines[dest] = mem_alloc( syncbuff_size[dest], bstring );

   for( i = 0 ; syncbuff_size[dest] > i ; i++ ) {
      syncbuff_lines[dest][i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }
}

static void syncbuff_realloc( IPC_END dest, SCAFFOLD_SIZE new_alloc ) {
   SCAFFOLD_SIZE i;

   scaffold_print_debug( &module,
      "Realloc: %d: %d to %d\n", dest, syncbuff_size[dest], new_alloc
   );

   syncbuff_size[dest] = new_alloc;
   syncbuff_lines[dest] = mem_realloc(
      syncbuff_lines[dest], syncbuff_size[dest], bstring
   );
   scaffold_assert( NULL != syncbuff_lines[dest] );

   for( i = syncbuff_count[dest] ; syncbuff_size[dest] > i ; i++ ) {
      syncbuff_lines[dest][i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }
}

void ipc_setup() {
   syncbuff_alloc( IPC_END_CLIENT );
   syncbuff_alloc( IPC_END_SERVER );
}

void ipc_shutdown() {
   /* TODO */
}

struct CONNECTION* ipc_alloc() {
   return mem_alloc( 1, struct CONNECTION );
}

void ipc_free( struct CONNECTION** n ) {
   mem_free( *n );
   n = NULL;
}

BOOL ipc_listen( struct CONNECTION* n, uint16_t port ) {
   BOOL retval = FALSE;

#ifdef SYNCBUFF_STRICT
   scaffold_assert( SYNCBUFF_STATE_LISTENING != n->state );
#else
   if( SYNCBUFF_STATE_LISTENING == n->state ) {
      retval = FALSE;
      goto cleanup;
   }
#endif /* SYNCBUFF_STRICT */

   n->type = IPC_END_SERVER;
   n->state = SYNCBUFF_STATE_LISTENING;
   retval = TRUE;

cleanup:
   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      n->state = SYNCBUFF_STATE_DISCONNECTED;
   }

   return retval;
}

BOOL ipc_connect( struct CONNECTION* n, const bstring server, uint16_t port ) {
   scaffold_assert( SYNCBUFF_STATE_DISCONNECTED == n->state );
   n->state = SYNCBUFF_STATE_CONNECTED;
   n->type = IPC_END_CLIENT;
   n->local = TRUE;
   return TRUE;
}

BOOL ipc_connected( struct CONNECTION* n ) {
   if(
      SYNCBUFF_STATE_CONNECTED == n->state ||
      SYNCBUFF_STATE_LISTENING == n->state
   ) {
      return TRUE;
   }
   return FALSE;
}

void ipc_stop( struct CONNECTION* n ) {
   scaffold_assert( SYNCBUFF_STATE_DISCONNECTED != n->state );
   n->state = SYNCBUFF_STATE_DISCONNECTED;
}

BOOL ipc_accept( struct CONNECTION* n_server, struct CONNECTION* n ) {
   scaffold_assert( SYNCBUFF_STATE_LISTENING == n_server->state );
   scaffold_assert( SYNCBUFF_STATE_DISCONNECTED == n->state );
   scaffold_assert( IPC_END_SERVER == n_server->type );
   if( 0 == n_server->client_count ) {
      n->state = SYNCBUFF_STATE_CONNECTED;
      n->type = IPC_END_CLIENT;
      n_server->client_count++;
      return TRUE;
   } else {
      return FALSE;
   }
}

SCAFFOLD_SIZE_SIGNED ipc_write( struct CONNECTION* n, const bstring buffer ) {
/* Push a line down onto the top. */
   int bstr_result;
   SCAFFOLD_SIZE_SIGNED i;
   SCAFFOLD_SIZE_SIGNED size_out = 0;
   IPC_END dest = IPC_END_SERVER;

   scaffold_assert( NULL != buffer );

   if( FALSE == n->local ) {
      /* We're sending stuff TO the client. */
      dest = IPC_END_CLIENT;
   }

   scaffold_print_debug( &module,  "Write: %s\n", bdata( buffer ) );

   if( syncbuff_count[dest] + 2 >= syncbuff_size[dest] ) {
      syncbuff_realloc( dest, syncbuff_size[dest] * 2 );
   }

   scaffold_print_debug( &module,  "Count is %d\n", syncbuff_count[dest] );

   for( i = syncbuff_count[dest] ; 0 <= i ; i-- ) {
      scaffold_print_debug( &module,
         "Move: %d: %d was: %s\n", dest, (i + 1), bdata( syncbuff_lines[dest][i + 1] )
      );
      bstr_result =
         bassignformat( syncbuff_lines[dest][i + 1], "%s", bdata( syncbuff_lines[dest][i] ) );
      scaffold_assert( NULL != syncbuff_lines[dest][i + 1] );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
      scaffold_print_debug( &module,
         "Move: %d: %d now: %s\n", dest, (i + 1), bdata( syncbuff_lines[dest][i + 1] )
      );
   }

   scaffold_print_debug( &module,
      "Write: %d: %d was: %s\n", dest, 0, bdata( syncbuff_lines[dest][0] )
   );
   bstr_result =
      bassignformat( syncbuff_lines[dest][0], "%s", bdata( buffer ) );
   scaffold_assert( NULL != syncbuff_lines[dest][0] );
   scaffold_assert( blength( buffer ) == blength( syncbuff_lines[dest][0] ) );
   if( 0 != bstr_result ) {
      goto cleanup;
   }
   scaffold_print_debug( &module,
      "Write: %d: %d now: %s\n", dest, 0, bdata( syncbuff_lines[dest][0] )
   );

   (syncbuff_count[dest])++;

   scaffold_assert( bdata( buffer ) != bdata( syncbuff_lines[dest][0] ) );

   size_out = blength( syncbuff_lines[dest][0] );

cleanup:
   return size_out;
}

/* Pull a line off the bottom. */
SCAFFOLD_SIZE_SIGNED ipc_read( struct CONNECTION* n, bstring buffer ) {
   int  bstr_result;
   SCAFFOLD_SIZE_SIGNED size_out = -1;
   IPC_END dest = IPC_END_SERVER;

   scaffold_assert( NULL != buffer );

   if( FALSE != n->local ) {
      dest = IPC_END_CLIENT;
   }

   bstr_result = btrunc( buffer, 0 );
   scaffold_assert( 0 == blength( buffer ) );
   if( 0 != bstr_result ) {
      scaffold_print_debug( &module,  "btrunc error on line: %d\n", __LINE__ );
      goto cleanup;
   }

   if( 0 < syncbuff_count[dest] ) {
      (syncbuff_count[dest])--;
      scaffold_assert( NULL != syncbuff_lines[dest][syncbuff_count[dest]] );
      bstr_result =
         bassignformat( buffer, "%s", bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );
      scaffold_assert( NULL != buffer );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
      bstr_result = btrunc( syncbuff_lines[dest][syncbuff_count[dest]], 0 );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
   }

   scaffold_assert( bdata( buffer ) != bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );

   size_out = blength( buffer );

cleanup:
   return size_out;
}

IPC_END ipc_get_type( struct CONNECTION* n ) {
   return n->type;
}

BOOL ipc_is_local_client( struct CONNECTION* n ) {
   return n->local;
}

BOOL ipc_is_listening( struct CONNECTION* n ) {
   if( SYNCBUFF_STATE_LISTENING == n->state ) {
      return TRUE;
   }
   return FALSE;
}

uint16_t ipc_get_port( struct CONNECTION* n ) {
   return 0;
}
