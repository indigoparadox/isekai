#include "client.h"

//#include "parser.h"
#include "server.h"
#include "callbacks.h"
#include "irc.h"

/*
void* cb_client_cmp_ptr( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   CLIENT* c_ptr = (CLIENT*)arg;
   if( c == c_ptr ) {
      return c;
   }
   return NULL;
}
*/

static void client_cleanup( const struct _REF *ref ) {
   CONNECTION* n = scaffold_container_of( ref, struct _CONNECTION, refcount );
   CLIENT* c = scaffold_container_of( c, struct _CLIENT, link );
   hashmap_cleanup( &(c->channels) );
   /* TODO: Free chunkers? */
   hashmap_cleanup( &(c->chunkers) );
   vector_free( &(c->command_queue) );
   connection_cleanup( &(c->link) );
   bdestroy( c->buffer );
   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );
   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   //free( c );
}

void client_init( CLIENT* c ) {
   ref_init( &(c->link.refcount), client_cleanup );
   hashmap_init( &(c->channels) );
   vector_init( &(c->command_queue ) );
   c->buffer = bfromcstralloc( CLIENT_BUFFER_ALLOC, "" );
   c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->sentinal = CLIENT_SENTINAL;
   hashmap_init( &(c->chunkers) );
   //memset( &(c->chunker), '\0', sizeof( CLIENT_CHUNKER ) );
   /* if( NULL != m ) {
      c->jobs = m;
      c->jobs_socket = -1;
   } */
   c->running = TRUE;
}

BOOL client_free( CLIENT* c ) {
   return ref_dec( &(c->link.refcount) );
}

CHANNEL* client_get_channel_by_name( CLIENT* c, const bstring name ) {
   //return vector_iterate( &(c->channels), callback_search_channels, name );
   return hashmap_get( &(c->channels), name );
}

void client_connect( CLIENT* c, bstring server, int port ) {
   bstring buffer;

#ifdef DEBUG
   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
#endif /* DEBUG */

   connection_connect( &(c->link), server , port );
   scaffold_check_negative( scaffold_error );

   buffer = bfromcstr( "NICK " );
   bconcat( buffer, c->nick );
   client_send( c, buffer );
   bdestroy( buffer );

   buffer = bfromcstr( "USER " );
   bconcat( buffer, c->realname );
   client_send( c, buffer );
   bdestroy( buffer );

cleanup:

   return;
}

/* This runs on the local client. */
/* TODO: Process multiple lines? */
void client_update( CLIENT* c, GAMEDATA* d ) {
   ssize_t last_read_count = 0;
   IRC_COMMAND* cmd = NULL;

#ifdef DEBUG
   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
#endif /* DEBUG */

   btrunc( c->buffer, 0 );
   last_read_count = connection_read_line( &(c->link), c->buffer, TRUE );
   btrimws( c->buffer );

   if( 0 < last_read_count ) {
      /* TODO: Handle error reading. */

#ifdef DEBUG
      /* TODO: If for trace path? */
#ifdef DEBUG_RAW_LINES
      scaffold_print_debug(
         "Client %d: Line received from server: %s\n",
         c->link.socket, bdata( c->buffer )
      );
#endif /* DEBUG_RAW_LINES */
      assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
#endif /* DEBUG */

      //irc_dispatch( c, d, c->buffer );
   }

   /* Check for commands from the server. */
   cmd = callback_ingest_commands( NULL, c, NULL );
   if( NULL != cmd ) {
      vector_add( &(c->command_queue), cmd );
   }

   /* Execute one command per cycle if available. */
   if( 1 <= vector_count( &(c->command_queue) ) ) {
      cmd = vector_get( &(c->command_queue), 0 );
      vector_remove( &(c->command_queue), 0 );
      if( NULL != cmd->callback ) {
         cmd->callback( cmd->client, cmd->server, cmd->args );
      } else {
         scaffold_print_error( "Invalid command: %s\n", bdata( &(cmd->command) ) );
      }
      irc_command_free( cmd );
   }

   gamedata_update_client( d, c );

   return;
}

void client_stop( CLIENT* c ) {
   bstring buffer = NULL;

   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
   buffer = bfromcstr( "QUIT" );
   client_send( c, buffer );
   bdestroy( buffer );
}

void client_add_channel( CLIENT* c, CHANNEL* l ) {
   //vector_add( &(c->channels), l );
   hashmap_put( &(c->channels), l->name, l );
}

void client_join_channel( CLIENT* c, bstring name ) {
   bstring buffer = NULL;
   /* We won't record the channel in our list until the server confirms it. */
   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
   buffer = bfromcstr( "JOIN " );
   bconcat( buffer, name );
   client_send( c, buffer );
   bdestroy( buffer );
}

void client_leave_channel( CLIENT* c, bstring lname ) {
   bstring buffer = NULL;
   /* We won't record the channel in our list until the server confirms it. */
   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
   //scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
   buffer = bfromcstr( "PART " );
   bconcat( buffer, lname );
   client_send( c, buffer );
   bdestroy( buffer );

   /* TODO: Add callback from parser and only delete channel on confirm. */
   /* TODO: Cleanup channel. */
   //vector_delete_cb( &(c->channels), callback_free_channels, lname );
   hashmap_remove( &(c->channels), lname );
}

void client_send( CLIENT* c, bstring buffer ) {

   /* TODO: Make sure we're still connected. */

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, TRUE );

#ifdef DEBUG_RAW_LINES
   scaffold_print_debug( "Client sent to server: %s\n", bdata( buffer ) );
#endif /* DEBUG_RAW_LINES */
   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
}

void client_printf( CLIENT* c, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );

   buffer = bfromcstralloc( strlen( message ), "" );
   scaffold_check_null( buffer );

   va_start( varg, message );
   scaffold_snprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == scaffold_error ) {
      client_send( c, buffer );
   }

cleanup:
   bdestroy( buffer );
   return;
}
