
#define SERVER_C
#include "server.h"

#include "callback.h"
#include "hashmap.h"
#include "proto.h"
#include "channel.h"

static void server_cleanup( struct SERVER* s ) {
   /* Infinite circle. server_free > client_free > ref_dec > server_free */
   client_free_from_server( &(s->self) );

   /* Remove clients. */
   server_free_clients( s );

   scaffold_assert( 0 == s->self.refcount.count );
}

void server_free_clients( struct SERVER* s ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted = 0;

   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(s->clients), callback_free_clients, NULL );
#ifdef DEBUG
   scaffold_print_debug(
      &module, "Removed %d clients from server. %d remaining.\n",
      deleted, hashmap_count( &(s->clients) )
   );
#endif /* DEBUG */
   hashmap_cleanup( &(s->clients) );
}

static void server_free_final( const struct REF* ref ) {
   struct SERVER* s =
      scaffold_container_of( ref, struct SERVER, self.refcount );

   server_cleanup( s );

   mem_free( s );
}

BOOL server_free( struct SERVER* s ) {
   if( NULL == s ) {
      return FALSE;
   }
   return refcount_dec( &(s->self), "server" );
}

void server_init( struct SERVER* s, const bstring myhost ) {
   int bstr_result;
   client_init( &(s->self), FALSE );
   s->self.refcount.gc_free = server_free_final;
   hashmap_init( &(s->clients) );
   s->self.sentinal = SERVER_SENTINAL;
   bstr_result = bassign( s->self.remote, myhost );
   scaffold_check_nonzero( bstr_result );
cleanup:
   return;
}

void server_stop( struct SERVER* s ) {
   if( ipc_is_listening( s->self.link ) ) {
      scaffold_print_info( &module, "Server shutting down...\n" );
      ipc_stop( s->self.link );
   }
   while(
      0 < hashmap_count( &(s->clients) ) &&
      0 < hashmap_count( &(s->self.channels) )
   ) {
      server_service_clients( s );
      graphics_sleep( 1000 );
   }
   /*
   client_free_channels( &(s->self) );
   server_free_clients( s );
   */
   scaffold_assert( 0 == hashmap_count( &(s->self.channels) ) );
   scaffold_assert( 0 == hashmap_count( &(s->clients) ) );
   s->self.running = FALSE;
}

void server_channel_send( struct SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, bstring buffer ) {
   struct VECTOR* l_clients = NULL;
   bstring skip_nick = NULL;

   if( NULL != c_skip ) {
      skip_nick = c_skip->nick;
   }

   l_clients =
      hashmap_iterate_v( &(l->clients), callback_search_clients_r, skip_nick );
   scaffold_check_null( l_clients );

   vector_iterate( l_clients, callback_send_clients, buffer );

cleanup:
   if( NULL != l_clients ) {
      vector_remove_cb( l_clients, callback_free_clients, NULL );
      vector_cleanup( l_clients );
      mem_free( l_clients );
   }
   scaffold_assert_server();
}

void server_channel_printf( struct SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   buffer = bfromcstralloc( strlen( message ), "" );
   scaffold_check_null( buffer );

   va_start( varg, message );
   scaffold_vsnprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == scaffold_error ) {
      server_channel_send( s, l, c_skip, buffer );
   }

   scaffold_assert_server();

cleanup:
   bdestroy( buffer );
   return;
}

void server_add_client( struct SERVER* s, struct CLIENT* c ) {
   if( 0 >= blength( c->nick ) ) {
      /* Generate a temporary random nick not already existing. */
      do {
         scaffold_random_string( c->nick, SERVER_RANDOM_NICK_LEN );
      } while( NULL != hashmap_get( &(s->clients), c->nick ) );
   }
   scaffold_assert( NULL == hashmap_get( &(s->clients), c->nick ) );
   hashmap_put( &(s->clients), c->nick, c );
   scaffold_print_debug(
      &module, "Client %p added to server with nick: %s\n",
      c, bdata( c->nick )
   );
}

struct CHANNEL* server_add_channel( struct SERVER* s, bstring l_name, struct CLIENT* c_first ) {
   struct CHANNEL* l = NULL;
#ifdef DEBUG
   SCAFFOLD_SIZE old_count;
#endif /* DEBUG */

   /* Get the channel, or create it if it does not exist. */
   l = server_get_channel_by_name( s, l_name );

   if( NULL == l ) {
      /* Create a new channel on the server. */
      channel_new( l, l_name, FALSE, &(s->self) );
      client_add_channel( &(s->self), l );
      scaffold_print_debug(
         &module, "Server: Channel created: %s\n", bdata( l->name ) );

      channel_load_tilemap( l );
      hashmap_iterate(
         &(s->self.tilesets),
         callback_load_local_tilesets,
         &(s->self)
      );
      scaffold_check_nonzero( scaffold_error );
   } else {
      scaffold_print_debug(
         &module, "Server: Channel found on server: %s\n", bdata( l->name ) );
   }

   /* Make sure the user is not already in the channel. If they are, then  *
    * just shut up and explode.                                            */
   if( NULL != channel_get_client_by_name( l, c_first->nick ) ) {
      scaffold_print_debug(
         &module, "Server: %s already in channel %s; ignoring.\n",
         bdata( c_first->nick ), bdata( l->name )
      );
      l = NULL;
      goto cleanup;
   }

   scaffold_assert( 0 < c_first->refcount.count );
   scaffold_assert( 0 < l->refcount.count );

#ifdef DEBUG
   old_count = c_first->refcount.count;
   server_channel_add_client( l, c_first );
   scaffold_assert( c_first->refcount.count > old_count );
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

   channel_add_client( l, c, TRUE );

   hashmap_iterate( &(l->clients), callback_send_mobs_to_channel, l );

cleanup:
   return;
}

struct CLIENT* server_get_client( struct SERVER* s, const bstring nick ) {
   return hashmap_get( &(s->clients), nick );
}

struct CHANNEL* server_get_channel_by_name( struct SERVER* s, const bstring nick ) {
   return client_get_channel_by_name( &(s->self), (bstring)nick );
}

void server_drop_client( struct SERVER* s, const bstring nick ) {
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
#ifdef DEBUG
   scaffold_print_debug(
      &module, "Server: Removed %d clients (%b). %d remaining.\n",
      deleted, nick, hashmap_count( &(s->clients) )
   );
#endif /* DEBUG */

   hashmap_iterate( &(s->self.channels), callback_remove_clients, nick );

#ifdef DEBUG
   new_count = hashmap_count( &(s->clients) );
   scaffold_assert( new_count == old_count - deleted );

   old_count = hashmap_count( &(s->self.channels) );
#endif /* DEBUG */

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(s->self.channels), callback_free_empty_channels, NULL );
#ifdef DEBUG
   scaffold_print_debug(
      &module, "Removed %d channels from server. %d remaining.\n",
      deleted, hashmap_count( &(s->self.channels) )
   );
#endif /* DEBUG */

/* cleanup: */
   return;
}

BOOL server_listen( struct SERVER* s, int port ) {
   BOOL connected = FALSE;

   connected = ipc_listen( s->self.link, port );
   if( FALSE == connected ) {
      scaffold_print_error(
         &module, "Server: Unable to bind to specified port. Exiting.\n" );
   } else {
      s->self.running = TRUE;
   }

   return connected;
}

BOOL server_poll_new_clients( struct SERVER* s ) {
   static struct CLIENT* c = NULL;
   BOOL new_clients = FALSE;
#ifdef DEBUG
   SCAFFOLD_SIZE_SIGNED old_client_count = 0;

   old_client_count = hashmap_count( &(s->clients) );
   scaffold_trace_path = SCAFFOLD_TRACE_SERVER;
#endif /* DEBUG */

   /* Get a new standby client ready. */
   if( NULL == c ) {
      client_new( c, FALSE );
   }

   /* Check for new clients. */
   ipc_accept( s->self.link, c->link );
   if( !client_connected( c ) ) {
      goto cleanup;
   } else {

      /* Add some details to c before stowing it. */
      //connection_assign_remote_name( c->link, c->remote );

      server_add_client( s, c );

#ifdef DEBUG
      scaffold_assert( old_client_count < hashmap_count( &(s->clients) ) );
#endif /* DEBUG */

      /* The only association this client should start with is the server's   *
       * client hashmap, so get rid of its initial ref.                       */
      refcount_dec( c, "client" );
      c = NULL;

      new_clients = TRUE;
   }

cleanup:
   return new_clients;
}

/**
 * \return TRUE if a command was executed, or FALSE otherwise.
 */
BOOL server_service_clients( struct SERVER* s ) {
   IRC_COMMAND* cmd = NULL;
   BOOL retval = FALSE;

   scaffold_set_server();

   scaffold_check_null( s );

   /* Check for commands from existing clients. */
   if( 0 < hashmap_count( &(s->clients) ) ) {
      /* TODO: Make sure hashmap_iterate can't overwrite scaffold errors. */
      cmd = hashmap_iterate( &(s->clients), callback_ingest_commands, s );
   }

   if( SCAFFOLD_ERROR_CONNECTION_CLOSED == scaffold_error ) {
      /* A dummy was returned, so the connection closed. */
      scaffold_print_info(
         &module, "Remote client disconnected: %n\n", cmd->line
      );
      server_drop_client( s, cmd->line );
      bdestroy( cmd->line );
      mem_free( cmd );
      cmd = NULL;
   }

   if( NULL != cmd ) {
      /* A presumably real command was returned. */
      retval = TRUE;
      if( NULL != cmd->callback ) {
         cmd->callback( cmd->client, cmd->server, cmd->args, cmd->line );
      } else {
         scaffold_print_error(
            &module, "Server: Invalid command: %s\n", bdata( &(cmd->command) )
         );
      }
      irc_command_free( cmd );
      retval = TRUE;
   }

#ifdef USE_CHUNKS

   /* Send files in progress. */
   hashmap_iterate( &(s->clients), callback_proc_chunkers, s );

#endif /* USE_CHUNKS */

   /* Spawn NPC mobiles. */
   hashmap_iterate( &(s->self.channels), callback_proc_server_spawners, s );

cleanup:
   return retval;
}

/* Scaffold errors if unsuccessful:
 *
 * NULLPO = No nick given.
 * NOT_NULLPO = Nick already taken.
 * NONZERO = Allocation error.
 */
void server_set_client_nick( struct SERVER* s, struct CLIENT* c, const bstring nick ) {
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
