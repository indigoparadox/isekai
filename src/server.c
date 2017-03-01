#include "server.h"

#include <stdlib.h>

#include "parser.h"
#include "heatshrink/heatshrink_encoder.h"
#include "b64/b64.h"
#include "callbacks.h"
#include "hashmap.h"

void server_init( SERVER* s, const bstring myhost ) {
   int bstr_result;
   client_init( &(s->self) );
   hashmap_init( &(s->clients) );
   s->servername =  blk2bstr( bsStaticBlkParms( "ProCIRCd" ) );
   s->version = blk2bstr(  bsStaticBlkParms( "0.1" ) );

   /* Setup the jobs mailbox. */
   //s->self.jobs = (MAILBOX*)calloc( 1, sizeof( MAILBOX ) );
   //vector_init( &(s->self.jobs->envelopes) );
   //vector_init( &(s->self.jobs->sockets_assigned) );
   //s->self.jobs->last_socket = 0;
   //s->self.jobs_socket = mailbox_listen( s->self.jobs );
   s->self.sentinal = SERVER_SENTINAL;
   bstr_result = bassign( s->self.remote, myhost );
   scaffold_check_nonzero( bstr_result );
cleanup:
   return;
}

inline void server_stop( SERVER* s ) {
   s->self.running = FALSE;
}

void server_cleanup( SERVER* s ) {
   size_t deleted = 0;

   if( TRUE == client_free( &(s->self) ) ) {
      bdestroy( s->version );
      bdestroy( s->servername );

      /* Remove clients. */
      deleted = hashmap_remove_cb( &(s->clients), callback_free_clients, NULL );
      scaffold_print_debug(
         "Removed %d clients from server. %d remaining.\n",
         deleted, hashmap_count( &(s->clients) )
      );
      hashmap_cleanup( &(s->clients) );

      deleted = hashmap_remove_cb( &(s->self.channels), callback_free_channels, NULL );
      scaffold_print_debug(
         "Removed %d channels from server. %d remaining.\n",
         deleted, hashmap_count( &(s->self.channels) )
      );
   }
}

void server_client_send( SERVER* s, CLIENT* c, bstring buffer ) {

   /* TODO: Make sure we're still connected. */
   //assert( NULL != server_get_client_by_ptr( s, c ) );
   assert( 0 != c->sentinal );

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, FALSE );

#ifdef DEBUG_RAW_LINES
   scaffold_print_debug( "Server sent to client %d: %s\n", c->link.socket, bdata( buffer ) );
#endif /* DEBUG_RAW_LINES */
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

/*
FIXME
void server_channel_send( SERVER* s, CHANNEL* l, CLIENT* c_skip, bstring buffer ) {
   SERVER_PBUFFER pbuffer;

   pbuffer.s = s;
   pbuffer.buffer = buffer;
   pbuffer.c_sender = c_skip;

   vector_iterate( &(l->clients), server_prn_channel, &pbuffer );

   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );
}
*/

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
   assert( NULL == hashmap_get( &(s->clients), c->nick ) );
   hashmap_put( &(s->clients), c->nick, c );
   //c->jobs_socket = mailbox_accept( s->self.jobs, -1 );
}

CHANNEL* server_add_channel( SERVER* s, bstring l_name, CLIENT* c_first ) {
   CHANNEL* l = NULL;
   bstring map_serial = NULL;
   BOOL just_created = FALSE;

   map_serial = bfromcstralloc( 1024, "" );
   scaffold_check_null( map_serial );

   /* Get the channel, or create it if it does not exist. */
   l = server_get_channel_by_name( s, l_name );

   if( NULL == l ) {
      channel_new( l, l_name );
      gamedata_init_server( &(l->gamedata), l_name );
      client_add_channel( &(s->self), l );
      scaffold_print_info( "Channel created: %s\n", bdata( l->name ) );
   } else {
      scaffold_print_info( "Channel found on server: %s\n", bdata( l->name ) );
      just_created = TRUE;
   }

   /* Make sure the user is not already in the channel. If they are, then  *
    * just shut up and explode.                                            */
   if( NULL != channel_get_client_by_name( l, c_first->nick ) ) {
      scaffold_print_debug(
         "%s already in channel %s; ignoring.\n",
         bdata( c_first->nick ), bdata( l->name )
      );
      l = NULL;
      goto cleanup;
   }

   channel_add_client( l, c_first );
   client_add_channel( c_first, l );

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
   size_t deleted;
#ifdef DEBUG
   size_t old_count = 0, new_count = 0;
#endif /* DEBUG */

#ifdef DEBUG
   old_count = hashmap_count( &(s->clients) );
#endif /* DEBUG */

   deleted = hashmap_remove_cb( &(s->clients), callback_free_clients, nick );
   scaffold_print_debug(
      "Removed %d clients from server. %d remaining.\n",
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
      //c->jobs_socket = mailbox_connect( c->jobs, -1, -1 );

      server_add_client( s, c );

      assert( NULL != c );
   }

   /* Check for commands from existing clients. */
   //for( i = 0 ; hashmap_count( &(s->clients) ) > i ; i++ ) {
   //}

   /* FIXME: Pass the mailbox. */
   hashmap_iterate( &(s->clients), callback_ingest_commands, s );

   /* TODO: Put the ingested commands into a mailbox so that we can lock it
    *       while cycling. */

   //hashmap_iterate( &(s->clients), server_prn_chunk, s );

   /* Handle outstanding jobs. */
   //mailbox_accept( s->self.jobs, s->self.jobs_socket );

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

   if( NULL != server_get_client( s, nick ) ) {
      retval = ERR_NICKNAMEINUSE;
      goto cleanup;
   }

   bstr_result = bassign( c->nick, nick );
   scaffold_check_nonzero( bstr_result );

cleanup:

   return retval;
}
