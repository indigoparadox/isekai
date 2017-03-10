#include "client.h"

#include "server.h"
#include "callbacks.h"
#include "proto.h"
#include "chunker.h"
#include "input.h"
#include "tilemap.h"

static struct GRAPHICS_TILE_WINDOW* twindow = NULL;

static void client_cleanup( const struct REF *ref ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;
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
   hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   hashmap_cleanup( &(c->sprites) );

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
   hashmap_init( &(c->sprites) );
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
void client_update( struct CLIENT* c, GRAPHICS* g ) {
   IRC_COMMAND* cmd = NULL;
   struct CHANNEL* l = NULL;

   scaffold_set_client();

   /* FIXME: Merge the next two loops. */

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

   /* Do drawing. */
   l = hashmap_get_first( &(c->channels) );
   if( NULL == l ) {
      /* TODO: What to display when no channel is up? */
      goto cleanup;
   }

   if( NULL == twindow ) {
      /* TODO: Free this, somehow. */
      twindow = calloc( 1, sizeof( struct GRAPHICS_TILE_WINDOW ) );
   }

   twindow->width = 640 / 32;
   twindow->height = 480 / 32;
   twindow->g = g;
   twindow->t = l->tilemap;

   if( NULL != l->tilemap && TILEMAP_SENTINAL == l->tilemap->sentinal ) {
      tilemap_draw_ortho( l->tilemap, g, twindow );
   } else {
      /* TODO: Loading... */
   }

   client_poll_input( c );

   vector_iterate( &(l->mobiles), callback_draw_mobiles, twindow );
      graphics_flip_screen( g );

cleanup:
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
   int bstr_retval;
   /* We won't record the channel in our list until the server confirms it. */

   scaffold_set_client();

   buffer = bfromcstr( "JOIN " );
   scaffold_check_null( buffer );
   bstr_retval = bconcat( buffer, name );
   scaffold_check_nonzero( bstr_retval );

   client_send( c, buffer );

cleanup:
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
   struct CLIENT* c, CHUNKER_DATA_TYPE type,
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
   struct CLIENT* c, CHUNKER_DATA_TYPE type, const bstring filename
) {
   struct CHUNKER* h = NULL;

   hashmap_lock( &(c->chunkers), TRUE );

   if( FALSE != hashmap_contains_key_nolock( &(c->chunkers), filename ) ) {
      /* File already requested, so just be patient. */
      goto cleanup;
   }

   h = hashmap_get_nolock( &(c->chunkers), filename );
   if( NULL == h ) {
      /* Create a chunker and get it started, since one is not in progress. */
      /* TODO: Verify cached file hash from server. */
      h = (struct CHUNKER*)calloc( 1, sizeof( struct CHUNKER ) );
      chunker_unchunk_start(
         h, type, filename, &str_chunker_cache_path
      );
      hashmap_put_nolock( &(c->chunkers), filename, h );
      scaffold_check_nonzero( scaffold_error );

      proto_request_file( c, filename, type );
   }

cleanup:
   hashmap_lock( &(c->chunkers), FALSE );
   return;
}

void client_process_chunk( struct CLIENT* c, struct CHUNKER_PROGRESS* cp ) {
   struct CHUNKER* h = NULL;
   //struct CHANNEL* l = NULL;
   int8_t chunker_percent;

   scaffold_assert( 0 < blength( cp->data ) );
   scaffold_assert( 0 < blength( cp->filename ) );

   if( cp->current > cp->total ) {
      scaffold_print_error( "Invalid progress for %s.\n", bdata( cp->filename ) );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   /*
   if( 0 < d->incoming_buffer_len && d->incoming_buffer_len != progress->total ) {
      scaffold_print_error( "Invalid total for %s.\n", bdata( progress->filename ) );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }
   */

   h = hashmap_get( &(c->chunkers), cp->filename );
   if( NULL == h ) {
      scaffold_print_error(
         "Client: Invalid data block received (I didn't ask for this?): %s\n",
         bdata( cp->filename )
      )
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   chunker_unchunk_pass( h, cp->data, cp->current, cp->total, cp->chunk_size );

   chunker_percent = chunker_unchunk_percent_progress( h, FALSE );
   if( 0 < chunker_percent ) {
      scaffold_print_debug( "Chunker: %s: %d%%\n", bdata( h->filename ), chunker_percent );
   }

   if( chunker_unchunk_finished( h ) ) {
      /* Cached file found, so abort transfer. */
      if( chunker_unchunk_cached( h ) ) {
         proto_abort_chunker( c, h );
      }

      client_handle_finished_chunker( c, h );
   }

cleanup:
   return;
}

void client_handle_finished_chunker( struct CLIENT* c, struct CHUNKER* h ) {
   struct CHANNEL* l = NULL;
   GRAPHICS* g = NULL;
   struct TILEMAP_TILESET* set = NULL;
   struct TILEMAP* t = NULL;

   assert( TRUE == chunker_unchunk_finished( h ) );

   switch( h->type ) {
   case CHUNKER_DATA_TYPE_TILEMAP:
      tilemap_new( t, TRUE );

#ifdef USE_EZXML
      datafile_parse_tilemap_ezxml( t, (BYTE*)h->raw_ptr, h->raw_length, TRUE );
#endif /* USE_EZXML */

      l = client_get_channel_by_name( c, t->lname );
      scaffold_assert( NULL != l );
      scaffold_check_null_msg( l, "Unable to find channel to attach loaded tileset." );

      if( NULL != l->tilemap ) {
         tilemap_free( l->tilemap );
      }
      l->tilemap = t;

      /* Go through the parsed tilemap and load graphics. */
      hashmap_iterate( &(t->tilesets), callback_proc_tileset_imgs, c );

      scaffold_print_info(
         "Client: Tilemap for %s successfully attached to channel.\n", bdata( l->name )
      );
      break;

   case CHUNKER_DATA_TYPE_TILESET_IMG:

      l = hashmap_iterate(
         &(c->channels),
         callback_search_channels_tilemap_img_name,
         h->filename
      );
      scaffold_assert( NULL != l );
      t = l->tilemap;
      scaffold_assert( NULL != t );

      graphics_surface_new( g, 0, 0, 0, 0 );
      scaffold_check_null( g );
      graphics_set_image_data( g, h->raw_ptr, h->raw_length );
      scaffold_check_null_msg( g->surface, "Unable to load tileset image." );

      set = hashmap_iterate( &(t->tilesets), callback_search_tilesets_img_name, h->filename );
      scaffold_check_null( set )

      hashmap_put( &(set->images), h->filename, g );
      scaffold_print_info(
         "Client: Tilemap image %s successfully loaded into tileset cache.\n",
         bdata( h->filename )
      );
      break;

   case CHUNKER_DATA_TYPE_MOBSPRITES:
      graphics_surface_new( g, 0, 0, 0, 0 );
      scaffold_check_null( g );
      //graphics_set_image_data( g, h->raw_ptr, h->raw_length );
      scaffold_check_null( g->surface );
      hashmap_put( &(c->sprites), h->filename, g );
      scaffold_print_info(
         "Client: Mobile spritesheet %s successfully loaded into cache.\n",
         bdata( h->filename )
      );
      break;
   }

cleanup:
   return;
}

void client_poll_input( struct CLIENT* c ) {
   struct INPUT input;

   input_get_event( &input );

   if( INPUT_TYPE_KEY == input.type ) {
      switch( input.character ) {
      case 'q':
         scaffold_set_client();
         client_stop( c );
         break;
      }
   }
}
