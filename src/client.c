#include "client.h"

#include "parser.h"
#include "server.h"

void* client_cmp_nick( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 == bstrcmp( c->nick, nick ) ) {
      return c;
   }
   return NULL;
}

void client_init( CLIENT* c ) {
   vector_init( &(c->channels) );
   c->buffer = bfromcstralloc( CLIENT_BUFFER_ALLOC, "" );
   c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->sentinal = CLIENT_SENTINAL;
   c->running = TRUE;
}

void client_cleanup( CLIENT* c ) {
   vector_free( &(c->channels) );
   connection_cleanup( &(c->link) );
   bdestroy( c->buffer );
   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );
}

CHANNEL* client_get_channel_by_name( CLIENT* c, const bstring name ) {
   return vector_iterate( &(c->channels), channel_cmp_name, name );
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

#ifdef DEBUG
   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
#endif /* DEBUG */

   btrunc( c->buffer, 0 );
   last_read_count = connection_read_line( &(c->link), c->buffer, TRUE );
   btrimws( c->buffer );

   if( 0 >= last_read_count ) {
      /* TODO: Handle error reading. */
      goto cleanup;
   }

#ifdef DEBUG
   /* TODO: If for trace path? */
   scaffold_print_debug(
      "Client %d: Line received from server: %s\n",
      c->link.socket, bdata( c->buffer )
   );
   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
#endif /* DEBUG */

   parser_dispatch( c, d, c->buffer );

   gamedata_update_client( d, c );

cleanup:
   return;
}

void client_add_channel( CLIENT* c, CHANNEL* l ) {
   vector_add( &(c->channels), l );
}

void client_join_channel( CLIENT* c, bstring name ) {
   /* We won't record the channel in our list until the server confirms it. */
   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
   bstring buffer = NULL;
   buffer = bfromcstr( "JOIN " );
   bconcat( buffer, name );
   client_send( c, buffer );
   bdestroy( buffer );
}

void client_leave_channel( CLIENT* c, bstring lname ) {
   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
   /* TODO: Add callback from parser and only delete channel on confirm. */
   /* TODO: Cleanup channel. */
   vector_delete_cb( &(c->channels), channel_cmp_name, lname, TRUE );
}

void client_send( CLIENT* c, bstring buffer ) {

   /* TODO: Make sure we're still connected. */

   bconchar( buffer, '\r' );
   bconchar( buffer, '\n' );
   connection_write_line( &(c->link), buffer, TRUE );

   scaffold_print_debug( "Client sent to server: %s", bdata( buffer ) );
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
