#include "server.h"

#include <stdlib.h>

#include "parser.h"

typedef struct {
   SERVER* s;
   bstring buffer;
   CLIENT* c_sender;
} SERVER_PBUFFER;

typedef struct {
   SERVER* s;
   bstring nick;
} SERVER_DUO;

/* This callback doesn't cleanup the channel, since it's just removing it     *
 * from a client's list.                                                      */
static void* server_client_del_chan(
   VECTOR* v, size_t idx, void* iter, void* arg
) {
   CHANNEL* l = (CHANNEL*)iter;
   CLIENT* c = (CLIENT*)arg;

   /* Remove the channel from the client's list, but don't free it. It might  *
    * still be in use!                                                        */
   channel_remove_client( l, c );
   return NULL;
}

static void* server_del_all_clients(
   VECTOR* v, size_t idx, void* iter, void* arg
) {
   CLIENT* c = (CLIENT*)iter;
   vector_delete_cb( &(c->channels), server_client_del_chan, c, TRUE );
   client_cleanup( c );
   return c;
}

static void* server_del_chan( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CHANNEL* l = (CHANNEL*)iter;
   if( 0 >= vector_count( &(l->clients ) ) ) {
      /* No clients left in this channel, so remove it from the server. */
      channel_cleanup( l );
      return l;
   }
   return NULL;
}

static void* server_del_client( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   SERVER_DUO* duo = (SERVER_DUO*)arg;
   if( 0 == bstrcmp( c->nick, duo->nick ) ) {
      /* Locks shouldn't conflict, since it's two different vectors. */
      vector_delete_cb( &(c->channels), server_client_del_chan, c, TRUE );
      return c;
   }
   return NULL;
}

void server_init( SERVER* s, const bstring myhost ) {
   client_init( &(s->self) );
   vector_init( &(s->clients) );
   s->self.remote = bstrcpy( myhost );
   s->servername =  blk2bstr( bsStaticBlkParms( "ProCIRCd" ) );
   s->version = blk2bstr(  bsStaticBlkParms( "0.1" ) );
   vector_init( &(s->queue.envelopes) );
   s->queue.last_socket = 0;
   s->self.sentinal = SERVER_SENTINAL;
}

inline void server_stop( SERVER* s ) {
   s->self.running = FALSE;
}

void server_cleanup( SERVER* s ) {
   size_t deleted = 0;

   /* Remove clients. */
   deleted = vector_delete_cb( &(s->clients), server_del_all_clients, NULL, TRUE );
   scaffold_print_debug(
      "Removed %d clients from server. %d remaining.\n",
      deleted, vector_count( &(s->clients) )
   );
   vector_free( &(s->clients) );

   deleted = vector_delete_cb( &(s->self.channels), server_del_chan, NULL, TRUE );
   scaffold_print_debug(
      "Removed %d channels from server. %d remaining.\n",
      deleted, vector_count( &(s->self.channels) )
   );
   /* Channels vector is freed in client_cleanup() below. */

   client_cleanup( &(s->self) );
}

void server_client_send( SERVER* s, CLIENT* c, bstring buffer ) {

   /* TODO: Make sure we're still connected. */

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, FALSE );

   scaffold_print_debug( "Server sent to client %d: %s", c->link.socket, bdata( buffer ) );
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
      server_client_send( s, c, buffer );
   }

   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );

cleanup:
   bdestroy( buffer );
   return;
}

static void* server_prn_channel( VECTOR* v, size_t idx, void* iter, void* arg ) {
   SERVER_PBUFFER* pbuffer = (SERVER_PBUFFER*)arg;
   CLIENT* c_iter = (CLIENT*)iter;
   if( 0 == bstrcmp( pbuffer->c_sender->nick, c_iter->nick ) ) {
      return NULL;
   }
   server_client_send( pbuffer->s, c_iter, pbuffer->buffer );
   return NULL;
}

void server_channel_send( SERVER* s, CHANNEL* l, CLIENT* c_skip, bstring buffer ) {
   SERVER_PBUFFER pbuffer;

   pbuffer.s = s;
   pbuffer.buffer = buffer;
   pbuffer.c_sender = c_skip;

   vector_iterate( &(l->clients), server_prn_channel, &pbuffer );

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
      server_channel_send( s, l, c_skip, buffer );
   }

   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );

cleanup:
   bdestroy( buffer );
   return;
}

void server_add_client( SERVER* s, CLIENT* c ) {
   vector_add( &(s->clients), c );
}

CHANNEL* server_add_channel( SERVER* s, bstring l_name, CLIENT* c_first ) {
   CHANNEL* l = NULL;
#ifndef USE_NO_SERIALIZE_CACHE
   bstring map_serial = NULL;

   map_serial = bfromcstralloc( 1024, "" );
   scaffold_check_null( map_serial );
#endif /* USE_NO_SERIALIZE_CACHE */

   /* Get the channel, or create it if it does not exist. */
   l = server_get_channel_by_name( s, l_name );

   if( NULL == l ) {
      channel_new( l, l_name );
      gamedata_init_server( &(l->gamedata), l_name );
#ifndef USE_NO_SERIALIZE_CACHE
      tilemap_serialize( &(l->gamedata.tmap), map_serial );
      scaffold_check_null( map_serial );
      l->gamedata.tmap.serialize_buffer = bsplit( map_serial, '\n' );
      scaffold_check_null( l->gamedata.tmap.serialize_buffer );
#endif /* USE_NO_SERIALIZE_CACHE */
      client_add_channel( &(s->self), l );
      scaffold_print_info( "Channel created: %s\n", bdata( l->name ) );
   } else {
      scaffold_print_info( "Channel found on server: %s\n", bdata( l->name ) );
   }

   /* Make sure the user is not already in the channel. If they are, then  *
    * just shut up and explode.                                            */
   if( NULL != channel_get_client_by_name( l, c_first->nick ) ) {
      scaffold_print_debug(
         "%s already in channel %s; ignoring.\n",
         bdata( c_first->nick ), bdata( l->name )
      );
      goto cleanup;
   }

   channel_add_client( l, c_first );
   client_add_channel( c_first, l );

cleanup:
#ifndef USE_NO_SERIALIZE_CACHE
   bdestroy( map_serial );
#endif /* USE_NO_SERIALIZE_CACHE */
   assert( 0 < vector_count( &(l->clients) ) );
   assert( 0 < vector_count( &(c_first->channels) ) );
   return l;
}

CLIENT* server_get_client( SERVER* s, int index ) {
   return (CLIENT*)vector_get( &(s->clients), index );
}

CLIENT* server_get_client_by_nick( SERVER* s, const bstring nick ) {
   return vector_iterate( &(s->clients), client_cmp_nick, (bstring)nick );
}

CHANNEL* server_get_channel_by_name( SERVER* s, bstring nick ) {
   return client_get_channel_by_name( &(s->self), nick );
}

void server_drop_client( SERVER* s, bstring nick ) {
   size_t deleted;
   SERVER_DUO duo = { 0 };
#ifdef DEBUG
   size_t old_count = 0;
   size_t new_count = 0;
#endif /* DEBUG */

   duo.s = s;
   duo.nick = nick;

#ifdef DEBUG
   old_count = vector_count( &(s->clients) );
#endif /* DEBUG */

   deleted = vector_delete_cb( &(s->clients), server_del_client, &duo, FALSE );
   scaffold_print_debug(
      "Removed %d clients from server. %d remaining.\n",
      deleted, vector_count( &(s->clients) )
   );

#ifdef DEBUG
   new_count = vector_count( &(s->clients) );
   assert( new_count == old_count - deleted );

   old_count = vector_count( &(s->self.channels) );
#endif /* DEBUG */

   deleted = vector_delete_cb( &(s->self.channels), server_del_chan, NULL, TRUE );
   scaffold_print_debug(
      "Removed %d channels from server. %d remaining.\n",
      deleted, vector_count( &(s->self.channels) )
   );

#ifdef DEBUG
   new_count = vector_count( &(s->self.channels) );
   assert( new_count == old_count - deleted );
#endif /* DEBUG */
}

void server_listen( SERVER* s, int port ) {
   s->self.link.arg = s;
   connection_listen( &(s->self.link), port );
   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      scaffold_print_error( "Unable to bind to specified port. Exiting.\n" );
   }
}

void server_service_clients( SERVER* s ) {
   ssize_t last_read_count = 0;
   CLIENT* c = NULL;
   CONNECTION* n_client = NULL;
   int i = 0;

#ifdef DEBUG
   scaffold_trace_path = SCAFFOLD_TRACE_SERVER;
#endif /* DEBUG */

   /* Check for new clients. */
   n_client = connection_register_incoming( &(s->self.link) );
   if( NULL != n_client ) {
      client_new( c );
      memcpy( &(c->link), n_client, sizeof( CONNECTION ) );
      free( n_client ); /* Don't clean up, because data is still valid. */

      /* Add some details to c before stowing it. */
      connection_assign_remote_name( &(c->link), c->remote );

      server_add_client( s, c );

      assert( NULL != c );
   }

   /* Check for commands from existing clients. */
   for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
      c = vector_get( &(s->clients), i );

      btrunc( s->self.buffer, 0 );
      last_read_count = connection_read_line( &(c->link), s->self.buffer, FALSE );
      btrimws( s->self.buffer );

      if( 0 >= last_read_count ) {
         /* TODO: Handle error reading. */
         continue;
      }

      scaffold_print_debug(
         "Server: Line received from %d: %s\n",
         c->link.socket, bdata( s->self.buffer )
      );

      parser_dispatch( s, c, s->self.buffer );
   }

cleanup:

   return;
}

/* Returns 0 if successful or IRC numeric error otherwise. */
int server_set_client_nick( SERVER* s, CLIENT* c, const bstring nick ) {
   int retval = 0;
   int bstr_result = 0;

   if( NULL == nick ) {
      retval = ERR_NONICKNAMEGIVEN;
      goto cleanup;
   }

   if( NULL != server_get_client_by_nick( s, nick ) ) {
      retval = ERR_NICKNAMEINUSE;
      goto cleanup;
   }

   bstr_result = bassign( c->nick, nick );
   scaffold_check_nonzero( bstr_result );

cleanup:

   return retval;
}
