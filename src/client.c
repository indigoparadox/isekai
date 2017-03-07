#include "client.h"

#include "server.h"
#include "callbacks.h"
#include "irc.h"
#include "chunker.h"

static void client_cleanup( const struct REF *ref ) {
#ifdef DEBUG
   size_t deleted;
#endif /* DEBUG */
   struct CLIENT* c = NULL;
   c = scaffold_container_of( c, struct CLIENT, link );
   scaffold_assert( NULL != c );
   connection_cleanup( &(c->link) );
   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );
   client_clear_puppet( c );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      vector_remove_cb( &(c->command_queue), callback_free_commands, NULL );
   scaffold_print_debug(
      "Removed %d commands. %d remaining.\n",
      deleted, vector_count( &(c->command_queue) )
   );
   vector_free( &(c->command_queue) );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(c->chunkers), callback_free_chunkers, NULL );
   scaffold_print_debug(
      "Removed %d chunkers. %d remaining.\n",
      deleted, hashmap_count( &(c->chunkers) )
   );
   hashmap_cleanup( &(c->chunkers) );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(c->channels), callback_free_channels, NULL );
   scaffold_print_debug(
      "Removed %d channels. %d remaining.\n",
      deleted, hashmap_count( &(c->channels) )
   );
   hashmap_cleanup( &(c->channels) );

   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   /* free( c ); */
}

void client_init( struct CLIENT* c ) {
   ref_init( &(c->link.refcount), client_cleanup );
   hashmap_init( &(c->channels) );
   vector_init( &(c->command_queue ) );
   c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->sentinal = CLIENT_SENTINAL;
   hashmap_init( &(c->chunkers) );
   c->running = TRUE;
}

BOOL client_free( struct CLIENT* c ) {
   return ref_dec( &(c->link.refcount) );
}

struct CHANNEL* client_get_channel_by_name( struct CLIENT* c, const bstring name ) {
   return hashmap_get( &(c->channels), name );
}

void client_connect( struct CLIENT* c, const bstring server, int port ) {
   scaffold_set_client();

   connection_connect( &(c->link), server , port );
   scaffold_check_nonzero( scaffold_error );

   client_printf( c, "NICK %b", c->nick );
   client_printf( c, "USER %b", c->realname );

cleanup:

   return;
}

/* This runs on the local client. */
void client_update( struct CLIENT* c ) {
   IRC_COMMAND* cmd = NULL;

   scaffold_set_client();

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
         scaffold_print_error( "Client: Invalid command: %s\n", bdata( &(cmd->command) ) );
      }
      irc_command_free( cmd );
   }

   return;
}

void client_stop( struct CLIENT* c ) {
   bstring buffer = NULL;

   scaffold_assert_client();

   buffer = bfromcstr( "QUIT" );
   client_send( c, buffer );
   bdestroy( buffer );
}

void client_add_channel( struct CLIENT* c, struct CHANNEL* l ) {
   hashmap_put( &(c->channels), l->name, l );
}

void client_join_channel( struct CLIENT* c, const bstring name ) {
   bstring buffer = NULL;
   /* We won't record the channel in our list until the server confirms it. */

   scaffold_set_client();

   buffer = bfromcstr( "JOIN " );
   bconcat( buffer, name );
   client_send( c, buffer );
   bdestroy( buffer );
}

void client_leave_channel( struct CLIENT* c, const bstring lname ) {
   bstring buffer = NULL;
   /* We won't record the channel in our list until the server confirms it. */

   scaffold_assert_client();

   buffer = bfromcstr( "PART " );
   bconcat( buffer, lname );
   client_send( c, buffer );
   bdestroy( buffer );

   /* TODO: Add callback from parser and only delete channel on confirm. */
   hashmap_remove( &(c->channels), lname );
}

void client_send( struct CLIENT* c, const bstring buffer ) {

   /* TODO: Make sure we're still connected. */

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, TRUE );

#ifdef DEBUG_NETWORK
   scaffold_print_debug( "Client sent to server: %s\n", bdata( buffer ) );
#endif /* DEBUG_NETWORK */

   scaffold_assert_client();
}

void client_printf( struct CLIENT* c, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   scaffold_assert_client();

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

void client_send_file(
   struct CLIENT* c, const bstring channel, CHUNKER_DATA_TYPE type,
   const bstring serverpath, const bstring filepath
) {
   struct CHUNKER* h = NULL;

   scaffold_print_debug(
      "Sending file to client %d: %s\n",
      c->link.socket, bdata( filepath )
   );

   /* Begin transmitting tilemap. */
   h = (struct CHUNKER*)calloc( 1, sizeof( struct CHUNKER ) );
   scaffold_check_null( h );

   chunker_chunk_start_file(
      h,
      channel,
      type,
      serverpath,
      filepath,
      64
   );
   scaffold_check_nonzero( scaffold_error );

   hashmap_put( &(c->chunkers), filepath, h );

cleanup:
   if( NULL != h ) {
      chunker_free( h );
   }
   return;
}

void client_set_puppet( struct CLIENT* c, struct MOBILE* o ) {
   if( NULL != c->puppet ) { /* Take care of existing mob before anything. */
      mobile_free( c->puppet );
      c->puppet = NULL;
   }
   ref_inc(  &(o->refcount) ); /* Add first, to avoid deletion. */
   if( NULL != o ) {
      if( NULL != o->owner ) {
         client_clear_puppet( o->owner );
      }
      o->owner = c;
   }
   c->puppet = o; /* Assign to client last, to activate. */
}

void client_clear_puppet( struct CLIENT* c ) {
   client_set_puppet( c, NULL );
}

void client_request_file(
   struct CLIENT* c, struct CHANNEL* l, CHUNKER_DATA_TYPE type,
   const bstring filename
) {
   if( FALSE != hashmap_contains_key( &(l->gamedata.incoming_chunkers), filename ) ) {
      /* File already requested, so just be patient. */
      goto cleanup;
   }

   client_printf( c, "GRF %d %b %b", type, l->name, filename );

cleanup:
   return;
}
