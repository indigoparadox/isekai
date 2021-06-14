
#define SERVER_C
#include "server.h"

#include "callback.h"
#include "hashmap.h"
#include "proto.h"
#include "channel.h"
#include "ipc.h"

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
      hashmap_remove_all( &(s->clients) );
      //hashmap_remove_cb( &(s->clients), callback_free_clients, NULL );
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
   s->self.local_client = FALSE;
   client_init( &(s->self) );
   s->self.refcount.gc_free = server_free_final;
   hashmap_init( &(s->clients) );
   s->self.sentinal = SERVER_SENTINAL;
   bstr_result = bassign( s->self.remote, myhost );
   scaffold_check_nonzero( bstr_result );
cleanup:
   return;
}

void server_stop_clients( struct SERVER* s ) {
   // This is kind of a hack..?
   server_drop_client( s, NULL );
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

short server_add_client( struct SERVER* s, struct CLIENT* c ) {
   if( 0 >= blength( c->nick ) ) {
      /* Generate a temporary random nick not already existing. */
      do {
         scaffold_random_string( c->nick, SERVER_RANDOM_NICK_LEN );
      } while( NULL != hashmap_get( &(s->clients), c->nick ) );
   }
   scaffold_assert( NULL == hashmap_get( &(s->clients), c->nick ) );
   if( hashmap_put( &(s->clients), c->nick, c, FALSE ) ) {
      scaffold_print_error( &module, "Attempted to double-add client: %b\n",
         c->nick );
      client_free( c );
      return 1;
   } else {
      scaffold_print_debug(
         &module, "Client %p added to server with nick: %s\n",
         c, bdata( c->nick )
      );
      return 0;
   }
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
      /* Use the channel we just found. */
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

   /* Add this user to the channel. */
//#ifdef DEBUG
   /* TODO: Make this dump us back at the menu. */
   /* Problem is: couldn't find map file! */
   old_count = c_first->refcount.count;
   /* scaffold_assert( 0 < c_first->refcount.count );
   scaffold_assert( 0 < l->refcount.count );
   scaffold_assert( c_first->refcount.count > old_count ); */
   if(
      0 < c_first->refcount.count &&
      0 < l->refcount.count
   ) {
      scaffold_print_debug(
         &module, "Adding %b to channel %b on server...\n",
         c_first->nick, l->name
      );
      server_channel_add_client( l, c_first );
      //client_add_channel( c_first, l );
   } else {
      l->error = bformat(
         "Unable to add %s to channel %s on server.",
         bdata( c_first->nick ), bdata( l->name )
      );
      scaffold_print_error(
         &module, "Unable to add %b to channel %b on server.\n",
         c_first->nick, l->name
      );
      scaffold_assert( NULL != l->error );
   }
   scaffold_assert( c_first->refcount.count > old_count );
//#endif /* DEBUG */

cleanup:
   if( NULL != l ) {
      scaffold_assert( 0 < hashmap_count( l->clients ) );
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

   hashmap_iterate( l->clients, callback_send_mobs_to_channel, l );

cleanup:
   return;
}

uint16_t server_get_port( struct SERVER* s ) {
   return ipc_get_port( s->self.link );
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
      client_new( c );
   }

   /* Check for new clients. */
   ipc_accept( s->self.link, c->link );
   if( !client_connected( c ) ) {
      goto cleanup;
   } else {

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

static void* server_srv_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct SERVER* s = (struct SERVER*)arg;
   BOOL keep_going = TRUE;

   keep_going = proto_dispatch( c, s );
   if( FALSE == keep_going ) {
      return c;
   }

   return NULL;
}

/**
 * \return TRUE if a command was executed, or FALSE otherwise.
 */
BOOL server_service_clients( struct SERVER* s ) {
   BOOL retval = FALSE;
   struct CLIENT* c_stop = NULL;

   scaffold_set_server();

   scaffold_check_null( s );

   /* Check for commands from existing clients. */
   if( 0 < hashmap_count( &(s->clients) ) ) {
      c_stop = hashmap_iterate_nolock( &(s->clients), server_srv_cb, s );
   }

   if( NULL != c_stop ) {
      /* A dummy was returned, so the connection closed. */
      scaffold_print_info(
         &module, "Remote client disconnected: %b\n", c_stop->nick
      );
      server_drop_client( s, c_stop->nick );
   }

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
