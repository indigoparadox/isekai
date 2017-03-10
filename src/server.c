#include "server.h"

#include <stdlib.h>

#include "heatshrink/heatshrink_encoder.h"
#include "b64.h"
#include "callbacks.h"
#include "hashmap.h"
#include "proto.h"

static void server_cleanup( const struct REF* ref ) {
   SERVER* s = scaffold_container_of( ref, SERVER, self.link.refcount );
#ifdef DEBUG
   SCAFFOLD_SIZE deleted = 0;
#endif /* DEBUG */

   if( TRUE == client_free( &(s->self) ) ) {
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
   s->self.link.refcount.gc_free = server_cleanup;
   hashmap_init( &(s->clients) );

   /* Setup the jobs mailbox. */
   s->self.sentinal = SERVER_SENTINAL;
   bstr_result = bassign( s->self.remote, myhost );
   scaffold_check_nonzero( bstr_result );
cleanup:
   return;
}

void server_stop( SERVER* s ) {
   s->self.running = FALSE;
}

void server_client_send( struct CLIENT* c, bstring buffer ) {

   /* TODO: Make sure we're still connected? */
   scaffold_assert( 0 != c->sentinal );

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, FALSE );

#ifdef DEBUG_NETWORK
   scaffold_print_debug( "Server sent to client %d: %s\n", c->link.socket, bdata( buffer ) );
#endif /* DEBUG_NETWORK */
   scaffold_assert_server();
}

void server_client_printf( struct CLIENT* c, const char* message, ... ) {
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

   scaffold_assert_server();

cleanup:
   bdestroy( buffer );
   return;
}

void server_channel_send( SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, bstring buffer ) {
   struct VECTOR* l_clients = NULL;

   l_clients =
      hashmap_iterate_v( &(l->clients), callback_search_clients_r, c_skip->nick );
   scaffold_check_null( l_clients );

   vector_iterate( l_clients, callback_send_clients, buffer );

cleanup:
   if( NULL != l_clients ) {
      vector_remove_cb( l_clients, callback_free_clients, NULL );
      vector_free( l_clients );
      free( l_clients );
   }
   scaffold_assert_server();
}

void server_channel_printf( SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   buffer = bfromcstralloc( strlen( message ), "" );
   scaffold_check_null( buffer );

   va_start( varg, message );
   scaffold_snprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == scaffold_error ) {
      server_channel_send( s, l, c_skip, buffer );
   }

   scaffold_assert_server();

cleanup:
   bdestroy( buffer );
   return;
}

void server_add_client( SERVER* s, struct CLIENT* c ) {
   if( 0 >= blength( c->nick ) ) {
      /* Generate a temporary random nick not already existing. */
      do {
         scaffold_random_string( c->nick, SERVER_RANDOM_NICK_LEN );
      } while( NULL != hashmap_get( &(s->clients), c->nick ) );
   }
   scaffold_assert( NULL == hashmap_get( &(s->clients), c->nick ) );
   hashmap_put( &(s->clients), c->nick, c );
   scaffold_print_debug(
      "Client %d added to server with nick: %s\n",
      c->link.socket, bdata( c->nick )
   );
}

struct CHANNEL* server_add_channel( SERVER* s, bstring l_name, struct CLIENT* c_first ) {
   struct CHANNEL* l = NULL;
#ifdef DEBUG
   SCAFFOLD_SIZE old_count;
#endif /* DEBUG */

   /* Get the channel, or create it if it does not exist. */
   l = server_get_channel_by_name( s, l_name );

   if( NULL == l ) {
      channel_new( l, l_name, FALSE );
      //gamedata_init_server( &(l->gamedata), l_name );
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

   scaffold_assert( 0 < c_first->link.refcount.count );
   scaffold_assert( 0 < l->refcount.count );

   channel_load_tilemap( l );
   scaffold_check_nonzero( scaffold_error );

#ifdef DEBUG
   old_count = c_first->link.refcount.count;
   server_channel_add_client( l, c_first );
   scaffold_assert( c_first->link.refcount.count > old_count );
   old_count = l->refcount.count;
   client_add_channel( c_first, l );
   scaffold_assert( l->refcount.count > old_count );
#endif /* DEBUG */

cleanup:
   if( NULL != l ) {
      scaffold_assert( 0 < hashmap_count( &(l->clients) ) );
      scaffold_assert( 0 < hashmap_count( &(c_first->channels) ) );
   }
   return l;
}

void server_channel_add_client( struct CHANNEL* l, struct CLIENT* c ) {

   scaffold_check_null( c );

   if( NULL != channel_get_client_by_name( l, c->nick ) ) {
      goto cleanup;
   }

   channel_add_client( l, c );

   hashmap_iterate( &(l->clients), callback_send_mobs_to_channel, l );

cleanup:
   return;
}

struct CLIENT* server_get_client( SERVER* s, const bstring nick ) {
   return hashmap_get( &(s->clients), nick );
}

struct CHANNEL* server_get_channel_by_name( SERVER* s, const bstring nick ) {
   return client_get_channel_by_name( &(s->self), (bstring)nick );
}

void server_drop_client( SERVER* s, bstring nick ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;
   SCAFFOLD_SIZE old_count = 0, new_count = 0;

   old_count = hashmap_count( &(s->clients) );
#endif /* DEBUG */

   /* Perform the deletion. */
   /* TODO: Remove the client from all of the other lists that have upped its *
    *       ref count.                                                        */
#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(s->clients), callback_free_clients, nick );
   scaffold_print_debug(
      "Server: Removed %d clients. %d remaining.\n",
      deleted, hashmap_count( &(s->clients) )
   );

#ifdef DEBUG
   new_count = hashmap_count( &(s->clients) );
   scaffold_assert( new_count == old_count - deleted );

   old_count = hashmap_count( &(s->self.channels) );
#endif /* DEBUG */

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(s->self.channels), callback_free_empty_channels, NULL );
   scaffold_print_debug(
      "Removed %d channels from server. %d remaining.\n",
      deleted, hashmap_count( &(s->self.channels) )
   );
}

void server_listen( SERVER* s, int port ) {
   s->self.link.arg = s;
   connection_listen( &(s->self.link), port );
   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      scaffold_print_error( "Server: Unable to bind to specified port. Exiting.\n" );
   }
}

void server_poll_new_clients( SERVER* s ) {
   static struct CLIENT* c = NULL;
#ifdef DEBUG
   SCAFFOLD_SIZE old_client_count = 0;

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
   } else {

      /* Add some details to c before stowing it. */
      connection_assign_remote_name( &(c->link), c->remote );

      server_add_client( s, c );

#ifdef DEBUG
      scaffold_assert( old_client_count < hashmap_count( &(s->clients) ) );
#endif /* DEBUG */

      /* The only association this client should start with is the server's   *
       * client hashmap, so get rid of its initial ref.                       */
      ref_dec( &(c->link.refcount) );
      c = NULL;
   }

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
   if( 0 < hashmap_count( &(s->clients) ) ) {
      cmd = hashmap_iterate( &(s->clients), callback_ingest_commands, s );
   }
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
   hashmap_iterate( &(s->clients), callback_proc_chunkers, s );

cleanup:
   return;
}

/* Scaffold errors if unsuccessful:
 *
 * NULLPO = No nick given.
 * NOT_NULLPO = Nick already taken.
 * NONZERO = Allocation error.
 */
void server_set_client_nick( SERVER* s, struct CLIENT* c, const bstring nick ) {
   int bstr_result = 0;
   struct CLIENT* c_test = NULL;

   scaffold_check_null( nick );

   c_test = server_get_client( s, nick );
   scaffold_check_not_null( c_test );

   bstr_result = bassign( c->nick, nick );
   scaffold_check_nonzero( bstr_result );

cleanup:
   return;
}
