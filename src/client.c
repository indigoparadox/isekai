#include "client.h"

#include "server.h"
#include "callbacks.h"
#include "irc.h"
#include "chunker.h"

static void client_cleanup( const struct _REF *ref ) {
#ifdef DEBUG
   size_t deleted;
#endif /* DEBUG */
   CLIENT* c = scaffold_container_of( c, struct _CLIENT, link );
   connection_cleanup( &(c->link) );
   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );
   client_clear_puppet( c );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(c->channels), callback_free_channels, NULL );
   scaffold_print_debug(
      "Removed %d channels. %d remaining.\n",
      deleted, hashmap_count( &(c->channels) )
   );
   hashmap_cleanup( &(c->channels) );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(c->mobiles), callback_free_mobiles, NULL );
   scaffold_print_debug(
      "Removed %d mobiles. %d remaining.\n",
      deleted, hashmap_count( &(c->mobiles) )
   );
   hashmap_cleanup( &(c->mobiles) );

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
      vector_remove_cb( &(c->command_queue), callback_free_commands, NULL );
   scaffold_print_debug(
      "Removed %d commands. %d remaining.\n",
      deleted, vector_count( &(c->command_queue) )
   );
   vector_free( &(c->command_queue) );

   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   /* free( c ); */
}

void client_init( CLIENT* c ) {
   ref_init( &(c->link.refcount), client_cleanup );
   hashmap_init( &(c->channels) );
   vector_init( &(c->command_queue ) );
   c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->sentinal = CLIENT_SENTINAL;
   hashmap_init( &(c->chunkers) );
   hashmap_init( &(c->mobiles ) );
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

   scaffold_set_client();

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
void client_update( CLIENT* c ) {
   //ssize_t last_read_count = 0;
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

void client_stop( CLIENT* c ) {
   bstring buffer = NULL;

   scaffold_assert_client();

   buffer = bfromcstr( "QUIT" );
   client_send( c, buffer );
   bdestroy( buffer );
}

void client_add_channel( CLIENT* c, CHANNEL* l ) {
   hashmap_put( &(c->channels), l->name, l );
}

void client_join_channel( CLIENT* c, bstring name ) {
   bstring buffer = NULL;
   /* We won't record the channel in our list until the server confirms it. */

   scaffold_set_client();

   buffer = bfromcstr( "JOIN " );
   bconcat( buffer, name );
   client_send( c, buffer );
   bdestroy( buffer );
}

void client_leave_channel( CLIENT* c, bstring lname ) {
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

void client_send( CLIENT* c, bstring buffer ) {

   /* TODO: Make sure we're still connected. */

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, TRUE );

#ifdef DEBUG_NETWORK
   scaffold_print_debug( "Client sent to server: %s\n", bdata( buffer ) );
#endif /* DEBUG_NETWORK */

   scaffold_assert_client();
}

void client_printf( CLIENT* c, const char* message, ... ) {
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
   CLIENT* c, bstring channel, CHUNKER_DATA_TYPE type, bstring serverpath,
   bstring filepath
) {
   CHUNKER* h = NULL;

   scaffold_print_debug(
      "Sending file to client %d: %s\n",
      c->link.socket, bdata( filepath )
   );

   /* Begin transmitting tilemap. */
   h = (CHUNKER*)calloc( 1, sizeof( CHUNKER ) );
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

void client_add_puppet( CLIENT* c, MOBILE* o ) {
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

void client_clear_puppet( CLIENT* c ) {
   client_add_puppet( c, NULL );
}
