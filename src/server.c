
#define SERVER_C
#include "server.h"

#include "callback.h"
#include "proto.h"
#include "channel.h"
#include "ipc.h"

#include "clistruct.h"

struct SERVER {
   /* "Root" class is REF*/

   /* "Parent class" */
   struct CLIENT self;

   /* Items after this line are server-specific. */
   struct HASHMAP* clients;
};

/** \brief Kick and free clients on all channels in the given list.
 **/
static void* callback_remove_clients( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring nick = (bstring)arg;

   hashmap_iterate( l->clients, callback_stop_clients, nick );
   hashmap_remove_cb( l->clients, callback_h_free_clients, nick );

   return NULL;
}

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
      //hashmap_remove_all( &(s->clients) );
      hashmap_remove_cb( s->clients, callback_h_free_clients, NULL );
#ifdef DEBUG
   lg_debug(
      __FILE__, "Removed %d clients from server. %d remaining.\n",
      deleted, hashmap_count( s->clients )
   );
#endif /* DEBUG */
   hashmap_free( &(s->clients) );
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

struct SERVER* server_new( const bstring myhost ) {
   struct SERVER* s = NULL;
   s = mem_alloc( 1, struct SERVER );
   lgc_null( s );
   server_init( s, myhost );
cleanup:
   return s;
}

void server_init( struct SERVER* s, const bstring myhost ) {
   int bstr_result;
   s->self.local_client = FALSE;
   client_init( &(s->self) );
   s->self.refcount.gc_free = server_free_final;
   s->clients = hashmap_new();
   s->self.sentinal = SERVER_SENTINAL;
   bstr_result = bassign( s->self.remote, myhost );
   lgc_nonzero( bstr_result );
cleanup:
   return;
}

void server_stop_clients( struct SERVER* s ) {
   // This is kind of a hack..?
   server_drop_client( s, NULL );
}

void server_stop( struct SERVER* s ) {
   if( ipc_is_listening( s->self.link ) ) {
      lg_info( __FILE__, "Server shutting down...\n" );
      ipc_stop( s->self.link );
   }
   while(
      0 < hashmap_count( s->clients ) &&
      0 < hashmap_count( s->self.channels )
   ) {
      server_service_clients( s );
      graphics_sleep( 1000 );
   }
   /*
   client_free_channels( &(s->self) );
   server_free_clients( s );
   */
   scaffold_assert( 0 == hashmap_count( s->self.channels ) );
   scaffold_assert( 0 == hashmap_count( s->clients ) );
   s->self.running = FALSE;
}

short server_add_client( struct SERVER* s, struct CLIENT* c ) {
   if( 0 >= blength( client_get_nick( c ) ) ) {
      /* Generate a temporary random nick not already existing. */
      do {
         scaffold_random_string( client_get_nick( c ), SERVER_RANDOM_NICK_LEN );
      } while( NULL != hashmap_get( s->clients, client_get_nick( c ) ) );
   }
   scaffold_assert( NULL == hashmap_get( s->clients, client_get_nick( c ) ) );
   if( hashmap_put( s->clients, client_get_nick( c ), c, FALSE ) ) {
      lg_error( __FILE__, "Attempted to double-add client: %b\n",
         client_get_nick( c ) );
      client_free( c );
      return 1;
   } else {
      lg_debug(
         __FILE__, "Client %p added to server with nick: %s\n",
         c, bdata( client_get_nick( c ) )
      );
      return 0;
   }
}

struct CHANNEL* server_add_channel( struct SERVER* s, bstring l_name, struct CLIENT* c_first ) {
   struct CHANNEL* l = NULL;
#ifdef DEBUG
   //SCAFFOLD_SIZE old_count;
#endif /* DEBUG */

   /* Get the channel, or create it if it does not exist. */
   l = server_get_channel_by_name( s, l_name );

   if( NULL == l ) {
      /* Create a new channel on the server. */
      channel_new( l, l_name, FALSE, &(s->self) );
      client_add_channel( &(s->self), l );
      lg_debug(
         __FILE__, "Server: Channel created: %s\n", bdata( l->name ) );

      channel_load_tilemap( l );
      hashmap_iterate(
         s->self.tilesets,
         callback_load_local_tilesets,
         &(s->self)
      );
      lgc_nonzero( lgc_error );
   } else {
      /* Use the channel we just found. */
      lg_debug(
         __FILE__, "Server: Channel found on server: %s\n", bdata( l->name ) );
   }

   /* Make sure the user is not already in the channel. If they are, then  *
    * just shut up and explode.                                            */
   if( NULL != channel_get_client_by_name( l, c_first->nick ) ) {
      lg_debug(
         __FILE__, "Server: %s already in channel %s; ignoring.\n",
         bdata( c_first->nick ), bdata( l->name )
      );
      l = NULL;
      goto cleanup;
   }

   /* Add this user to the channel. */
//#ifdef DEBUG
   /* TODO: Make this dump us back at the menu. */
   /* Problem is: couldn't find map file! */
   //old_count = c_first->refcount.count;
   /* scaffold_assert( 0 < c_first->refcount.count );
   scaffold_assert( 0 < l->refcount.count );
   scaffold_assert( c_first->refcount.count > old_count ); */
   /*if(
      0 < c_first->refcount.count &&
      0 < l->refcount.count
   ) {*/
      lg_debug(
         __FILE__, "Adding %b to channel %b on server...\n",
         client_get_nick( c_first ), l->name
      );
      server_channel_add_client( l, c_first );
      //client_add_channel( c_first, l );
   /*} else {
      l->error = bformat(
         "Unable to add %s to channel %s on server.",
         client_get_nick( c_first ), bdata( l->name )
      );
      lg_error(
         __FILE__, "Unable to add %b to channel %b on server.\n",
         client_get_nick( c_first ), l->name
      );
      scaffold_assert( NULL != l->error );
   }*/
   //scaffold_assert( c_first->refcount.count > old_count );
//#endif /* DEBUG */

cleanup:
   if( NULL != l ) {
      scaffold_assert( 0 < hashmap_count( l->clients ) );
      scaffold_assert( 0 < client_get_channels_count( c_first ) );
   }
   return l;
}

void server_channel_add_client( struct CHANNEL* l, struct CLIENT* c ) {

   lgc_null( c );

   if( NULL != channel_get_client_by_name( l, client_get_nick( c ) ) ) {
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
   return hashmap_get( s->clients, nick );
}

struct CHANNEL* server_get_channel_by_name( struct SERVER* s, const bstring nick ) {
   return client_get_channel_by_name( &(s->self), (bstring)nick );
}

void server_drop_client( struct SERVER* s, const bstring nick ) {
   BOOL deffered_lock = FALSE;
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;
   SCAFFOLD_SIZE old_count = 0, new_count = 0;

   old_count = hashmap_count( s->clients );
#endif /* DEBUG */

   /* Perform the deletion. */
   /* TODO: Remove the client from all of the other lists that have upped its *
    *       ref count.                                                        */
   if( hashmap_is_locked( s->clients ) ) {
      deffered_lock = TRUE;
      hashmap_lock( s->clients, FALSE );
   }
#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( s->clients, callback_h_free_clients, nick );
   if( deffered_lock ) {
      hashmap_lock( s->clients, TRUE );
   }
#ifdef DEBUG
   lg_debug(
      __FILE__, "Server: Removed %d clients (%b). %d remaining.\n",
      deleted, nick, hashmap_count( s->clients )
   );
#endif /* DEBUG */

   hashmap_iterate( s->self.channels, callback_remove_clients, nick );

#ifdef DEBUG
   new_count = hashmap_count( s->clients );
   scaffold_assert( new_count == old_count - deleted );

   old_count = hashmap_count( s->self.channels );
#endif /* DEBUG */

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb(
         s->self.channels, callback_free_empty_channels, NULL );
#ifdef DEBUG
   lg_debug(
      __FILE__, "Removed %d channels from server. %d remaining.\n",
      deleted, hashmap_count( s->self.channels )
   );
#endif /* DEBUG */

cleanup:
   return;
}

BOOL server_listen( struct SERVER* s, int port ) {
   BOOL connected = FALSE;

   connected = ipc_listen( s->self.link, port );
   if( FALSE == connected ) {
      lg_error(
         __FILE__, "Server: Unable to bind to specified port. Exiting.\n" );
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

   assert( ipc_is_listening( s->self.link ) );

   old_client_count = hashmap_count( s->clients );
   scaffold_set_server();
#endif /* DEBUG */

   /* Get a new standby client ready. */
   if( NULL == c ) {
      c = client_new();
   }

   /* Check for new clients. */
   ipc_accept( s->self.link, c->link );
   if( !client_is_connected( c ) ) {
      goto cleanup;
   } else {

      server_add_client( s, c );

#ifdef DEBUG
      scaffold_assert( old_client_count < hashmap_count( s->clients ) );
#endif /* DEBUG */

      /* The only association this client should start with is the server's   *
       * client hashmap, so get rid of its initial ref.                       */
      //refcount_dec( c, "client" ); Hashmap is no longer a ref!
      c = NULL;

      new_clients = TRUE;
   }

cleanup:
   return new_clients;
}

static void* server_srv_cb( bstring idx, void* iter, void* arg ) {
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

   lgc_null( s );

   /* Check for commands from existing clients. */
   if( 0 < hashmap_count( s->clients ) ) {
      // XXX: NOLOCK
      c_stop = hashmap_iterate( s->clients, server_srv_cb, s );
   }

   if( NULL != c_stop ) {
      /* A dummy was returned, so the connection closed. */
      lg_info(
         __FILE__, "Remote client disconnected: %b\n", client_get_nick( c_stop )
      );
      server_drop_client( s, client_get_nick( c_stop ) );
   }

#ifdef USE_CHUNKS

   /* Send files in progress. */
   hashmap_iterate( s->clients, callback_proc_chunkers, s );

#endif /* USE_CHUNKS */

   /* Spawn NPC mobiles. */
   hashmap_iterate( s->self.channels, callback_proc_server_spawners, s );

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
   //struct CLIENT* c_test = NULL;

   lgc_null( nick );

   /* TODO: Make sure the requested nick does not already exist! */
   //c_test = server_get_client( s, nick );
   //lgc_not_null( c_test );

   bstr_result = bassign( client_get_nick( c ), nick );
   lgc_nonzero( bstr_result );

cleanup:
   return;
}

BOOL server_is_running( struct SERVER* s ) {
   return client_is_running( &(s->self) );
}

BOOL server_is_listening( struct SERVER* s ) {
   return client_is_listening( &(s->self) );
}

bstring server_get_remote( struct SERVER* s ) {
   return client_get_remote( &(s->self) );
}

size_t server_get_client_count( struct SERVER* s ) {
   return hashmap_count( s->clients );
}

/* Return any client that is in the bstrlist arg. */
static void* callback_search_clients_l( bstring idx, void* iter, void* arg ) {
   struct VECTOR* list = (struct VECTOR*)arg;
   struct CLIENT* c = (struct CLIENT*)iter;
   return vector_iterate( list, callback_search_bstring_i, client_get_nick( c ) );
}

struct VECTOR* server_get_clients_online( struct SERVER* s, struct VECTOR* filter ) {
   struct VECTOR* ison = NULL;
   ison = hashmap_iterate_v( s->clients, callback_search_clients_l, filter );
   return ison;
}

size_t server_get_channels_count( struct SERVER* s ) {
   return client_get_channels_count( &(s->self) );
}
