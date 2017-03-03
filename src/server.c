#include "server.h"

#include <stdlib.h>

//#include "parser.h"
#include "heatshrink/heatshrink_encoder.h"
#include "b64/b64.h"
#include "callbacks.h"
#include "hashmap.h"
#include "irc.h"

static char nick_random_chars[] =
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static void server_cleanup( const struct _REF* ref ) {
   SERVER* s = scaffold_container_of( ref, SERVER, self.link.refcount );
#ifdef DEBUG
   size_t deleted = 0;
#endif /* DEBUG */

   if( TRUE == client_free( &(s->self) ) ) {
      bdestroy( s->version );
      bdestroy( s->servername );

      /* Remove clients. */
#ifdef DEBUG
      deleted =
#endif /* DEBUG */
         hashmap_remove_cb( &(s->clients), callback_free_clients, NULL );
      scaffold_print_debug(
         "Removed %d clients from server. %d remaining.\n",
         deleted, hashmap_count( &(s->clients) )
      );
      hashmap_cleanup( &(s->clients) );

#ifdef DEBUG
      deleted =
#endif /* DEBUG */
         hashmap_remove_cb( &(s->self.channels), callback_free_channels, NULL );
      scaffold_print_debug(
         "Removed %d channels from server. %d remaining.\n",
         deleted, hashmap_count( &(s->self.channels) )
      );
   }
}

BOOL server_free( SERVER* s ) {
   if( NULL == s ) {
      return FALSE;
   }
   return ref_dec( &(s->self.link.refcount) );
}

void server_init( SERVER* s, const bstring myhost ) {
   int bstr_result;
   client_init( &(s->self) );
   s->self.link.refcount.free = server_cleanup;
   hashmap_init( &(s->clients) );
   s->servername =  blk2bstr( bsStaticBlkParms( "ProCIRCd" ) );
   s->version = blk2bstr(  bsStaticBlkParms( "0.1" ) );

   /* Setup the jobs mailbox. */
   s->self.sentinal = SERVER_SENTINAL;
   bstr_result = bassign( s->self.remote, myhost );
   scaffold_check_nonzero( bstr_result );
cleanup:
   return;
}

inline void server_stop( SERVER* s ) {
   s->self.running = FALSE;
}

void server_client_send( CLIENT* c, bstring buffer ) {

   /* TODO: Make sure we're still connected? */
   assert( 0 != c->sentinal );

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, FALSE );

#ifdef DEBUG_NETWORK
   scaffold_print_debug( "Server sent to client %d: %s\n", c->link.socket, bdata( buffer ) );
#endif /* DEBUG_NETWORK */
   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );
}

void server_client_printf( SERVER* s, CLIENT* c, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   buffer = bfromcstralloc( strlen( message ), "" );
   scaffold_check_null( buffer );

   va_start( varg, message );
   scaffold_snprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == scaffold_error ) {
      server_client_send( c, buffer );
   }

   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );

cleanup:
   bdestroy( buffer );
   return;
}

void server_channel_send( SERVER* s, CHANNEL* l, CLIENT* c_skip, bstring buffer ) {
   VECTOR* l_clients = NULL;

   l_clients =
      hashmap_iterate_v( &(l->clients), callback_search_clients_r, c_skip );
   scaffold_check_null( l_clients );

   vector_iterate( l_clients, callback_send_clients, buffer );

cleanup:
   if( NULL != l_clients ) {
      vector_remove_cb( l_clients, callback_free_clients, NULL );
      vector_free( l_clients );
      free( l_clients );
   }
   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );
}

void server_channel_printf( SERVER* s, CHANNEL* l, CLIENT* c_skip, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   buffer = bfromcstralloc( strlen( message ), "" );
   scaffold_check_null( buffer );

   va_start( varg, message );
   scaffold_snprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == scaffold_error ) {
      // FIXME: server_channel_send( s, l, c_skip, buffer );
   }

   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );

cleanup:
   bdestroy( buffer );
   return;
}

void server_add_client( SERVER* s, CLIENT* c ) {
   size_t i;
   if( 0 >= blength( c->nick ) ) {
      /* Generate a temporary random nick not already existing. */
      do {
         btrunc( c->nick, 0 );
         for( i = 0; SERVER_RANDOM_NICK_LEN > i ; i++ ) {
            bconchar( c->nick, nick_random_chars[rand() % (int)(sizeof( nick_random_chars )-1)] );
         }
      } while( NULL != hashmap_get( &(s->clients), c->nick ) );
   }
   assert( NULL == hashmap_get( &(s->clients), c->nick ) );
   hashmap_put( &(s->clients), c->nick, c );
   scaffold_print_debug( "Client added to server with nick: %s\n", bdata( c->nick ) );
   //c->jobs_socket = mailbox_accept( s->self.jobs, -1 );
}

CHANNEL* server_add_channel( SERVER* s, bstring l_name, CLIENT* c_first ) {
   CHANNEL* l = NULL;
   bstring map_serial = NULL;
#ifdef DEBUG
   size_t old_count;
#endif /* DEBUG */

   map_serial = bfromcstralloc( 1024, "" );
   scaffold_check_null( map_serial );

   /* Get the channel, or create it if it does not exist. */
   l = server_get_channel_by_name( s, l_name );

   if( NULL == l ) {
      channel_new( l, l_name );
      gamedata_init_server( &(l->gamedata), l_name );
      client_add_channel( &(s->self), l );
      scaffold_print_info( "Server: Channel created: %s\n", bdata( l->name ) );
   } else {
      scaffold_print_info( "Server: Channel found on server: %s\n", bdata( l->name ) );
   }

   /* Make sure the user is not already in the channel. If they are, then  *
    * just shut up and explode.                                            */
   if( NULL != channel_get_client_by_name( l, c_first->nick ) ) {
      scaffold_print_debug(
         "Server: %s already in channel %s; ignoring.\n",
         bdata( c_first->nick ), bdata( l->name )
      );
      l = NULL;
      goto cleanup;
   }

   assert( 0 < c_first->link.refcount.count );
   assert( 0 < l->refcount.count );

#ifdef DEBUG
   old_count = c_first->link.refcount.count;
   channel_add_client( l, c_first );
   assert( c_first->link.refcount.count > old_count );
   old_count = l->refcount.count;
   client_add_channel( c_first, l );
   assert( l->refcount.count > old_count );
#endif /* DEBUG */

cleanup:
   bdestroy( map_serial );
   if( NULL != l ) {
      assert( 0 < hashmap_count( &(l->clients) ) );
      assert( 0 < hashmap_count( &(c_first->channels) ) );
   }
   return l;
}

/*
CLIENT* server_get_client( SERVER* s, int index ) {
   return (CLIENT*)vector_get( &(s->clients), index );
}
*/

CLIENT* server_get_client( SERVER* s, const bstring nick ) {
   //return vector_iterate( &(s->clients), callback_search_clients, (bstring)nick );
   return hashmap_get( &(s->clients), nick );
}

/*
CLIENT* server_get_client_by_ptr( SERVER* s, CLIENT* c ) {
   return vector_iterate( &(s->clients), client_cmp_ptr, c );
}
*/

CHANNEL* server_get_channel_by_name( SERVER* s, const bstring nick ) {
   return client_get_channel_by_name( &(s->self), (bstring)nick );
}

void server_drop_client( SERVER* s, bstring nick ) {
#ifdef DEBUG
   size_t deleted;
   size_t old_count = 0, new_count = 0;
#endif /* DEBUG */

#ifdef DEBUG
   old_count = hashmap_count( &(s->clients) );

   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(s->clients), callback_free_clients, nick );
   scaffold_print_debug(
      "Server: Removed %d clients. %d remaining.\n",
      deleted, hashmap_count( &(s->clients) )
   );

#ifdef DEBUG
   new_count = hashmap_count( &(s->clients) );
   assert( new_count == old_count - deleted );

   old_count = hashmap_count( &(s->self.channels) );
#endif /* DEBUG */

/*
   deleted = hashmap_remove_cb( &(s->self.channels), callback_free_channels, NULL );
   scaffold_print_debug(
      "Removed %d channels from server. %d remaining.\n",
      deleted, hashmap_count( &(s->self.channels) )
   );
*/

#ifdef DEBUG
   //new_count = hashmap_count( &(s->self.channels) );
   //assert( new_count == old_count - deleted );
#endif /* DEBUG */
}

void server_listen( SERVER* s, int port ) {
   s->self.link.arg = s;
   connection_listen( &(s->self.link), port );
   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      scaffold_print_error( "Server: Unable to bind to specified port. Exiting.\n" );
   }
}

void server_poll_new_clients( SERVER* s ) {
   static CLIENT* c = NULL;
#ifdef DEBUG
   size_t old_client_count = 0;

   old_client_count = hashmap_count( &(s->clients) );
   scaffold_trace_path = SCAFFOLD_TRACE_SERVER;
#endif /* DEBUG */

   /* Get a new standby client ready. */
   if( NULL == c ) {
      client_new( c );
   }

   /* Check for new clients. */
   connection_register_incoming( &(s->self.link), &(c->link) );
   if( 0 >= c->link.socket ) {
      goto cleanup;
   }

   /* Add some details to c before stowing it. */
   connection_assign_remote_name( &(c->link), c->remote );
   //c->jobs_socket = mailbox_connect( c->jobs, -1, -1 );

   server_add_client( s, c );

#ifdef DEBUG
   assert( old_client_count < hashmap_count( &(s->clients) ) );
#endif /* DEBUG */

   /* Ditch this client for now. */
   ref_dec( &(c->link.refcount) );
   c = NULL;

cleanup:
   return;
}

void server_service_clients( SERVER* s ) {
   IRC_COMMAND* cmd = NULL;

#ifdef DEBUG
   scaffold_trace_path = SCAFFOLD_TRACE_SERVER;
#endif /* DEBUG */

   scaffold_check_null( s );

   /* Check for commands from existing clients. */
   cmd = hashmap_iterate( &(s->clients), callback_ingest_commands, s );
   if( NULL != cmd ) {
      vector_add( &(s->self.command_queue), cmd );
   }

   /* Execute one command per cycle if available. */
   if( 1 <= vector_count( &(s->self.command_queue) ) ) {
      cmd = vector_get( &(s->self.command_queue), 0 );
      vector_remove( &(s->self.command_queue), 0 );
      if( NULL != cmd->callback ) {
         cmd->callback( cmd->client, cmd->server, cmd->args );
      } else {
         scaffold_print_error( "Server: Invalid command: %s\n", bdata( &(cmd->command) ) );
      }
      irc_command_free( cmd );
   }

   /* Send files in progress. */
   hashmap_iterate( &(s->clients), callback_process_chunkers, s );

cleanup:
   return;
}

/* Scaffold errors if unsuccessful:
 *
 * NULLPO = No nick given.
 * NOT_NULLPO = Nick already taken.
 * NONZERO = Allocation error.
 */
void server_set_client_nick( SERVER* s, CLIENT* c, const bstring nick ) {
   int bstr_result = 0;
   CLIENT* c_test = NULL;

   scaffold_check_null( nick );

   c_test = server_get_client( s, nick );
   scaffold_check_not_null( c_test );

   bstr_result = bassign( c->nick, nick );
   scaffold_check_nonzero( bstr_result );

cleanup:
   return;
}
