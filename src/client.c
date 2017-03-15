
#define CLIENT_C
#include "client.h"

#include "server.h"
#include "callback.h"
#include "proto.h"
#include "chunker.h"
#include "input.h"
#include "tilemap.h"
#include "datafile.h"

static void client_cleanup( const struct REF *ref ) {
   CONNECTION* n =
      (CONNECTION*)scaffold_container_of( ref, CONNECTION, refcount );
   struct CLIENT* c = scaffold_container_of( n, struct CLIENT, link );
   scaffold_assert( NULL != c );

   connection_cleanup( &(c->link) );

   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );
   hashmap_cleanup( &(c->sprites) );

   client_stop( c );

   vector_free( &(c->command_queue) );
   hashmap_cleanup( &(c->chunkers) );
   hashmap_cleanup( &(c->channels) );

   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   /* free( c ); */
}

void client_init( struct CLIENT* c, BOOL client_side ) {
   ref_init( &(c->link.refcount), client_cleanup );

   hashmap_init( &(c->channels) );
   vector_init( &(c->command_queue ) );
   hashmap_init( &(c->sprites) );
   hashmap_init( &(c->chunkers) );

   c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );

   c->sentinal = CLIENT_SENTINAL;
   c->client_side = client_side;
   c->running = TRUE;
}

 /** \brief This should ONLY be called from server_free in order to avoid
  *         an infinite loop.
  *
  * \param c - The CLIENT struct from inside of the SERVER being freed.
  * \return TRUE for now.
  *
  */
BOOL client_free_from_server( struct CLIENT* c ) {
   /* Kind of a hack, but make sure "running" is set to true to fool the      *
    * client_stop() call that comes later.                                    */
   c->running = TRUE;
   client_cleanup( &(c->link.refcount) );
   return TRUE;
}

BOOL client_free( struct CLIENT* c ) {
   return refcount_dec( &(c->link), "client" );
}

struct CHANNEL* client_get_channel_by_name( struct CLIENT* c, const bstring name ) {
   return hashmap_get( &(c->channels), name );
}

void client_connect( struct CLIENT* c, const bstring server, int port ) {
   scaffold_set_client();

   connection_connect( &(c->link), server , port );
   scaffold_check_nonzero( scaffold_error );

   proto_register( c );

cleanup:

   return;
}

/* This runs on the local client. */
void client_update( struct CLIENT* c, GRAPHICS* g ) {
   IRC_COMMAND* cmd = NULL;

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

/* cleanup: */
   return;
}

void client_stop( struct CLIENT* c ) {
   bstring buffer = NULL;
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;

   scaffold_assert( TRUE == c->running );

   if( TRUE == c->client_side ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }
#endif /* DEBUG */

   client_clear_puppet( c );

   buffer = bfromcstralloc( CLIENT_BUFFER_ALLOC, "" );
   scaffold_check_null( buffer );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(c->channels), callback_free_channels, NULL );
   scaffold_print_debug(
      "Removed %d channels. %d remaining.\n",
      deleted, hashmap_count( &(c->channels) )
   );
   scaffold_assert( 0 == hashmap_count( &(c->channels) ) );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      vector_remove_cb( &(c->command_queue), callback_free_commands, NULL );
   scaffold_print_debug(
      "Removed %d commands. %d remaining.\n",
      deleted, vector_count( &(c->command_queue) )
   );
   scaffold_assert( 0 == vector_count( &(c->command_queue) ) );

#ifdef USE_CHUNKS

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(c->chunkers), callback_free_chunkers, NULL );
   scaffold_print_debug(
      "Removed %d chunkers. %d remaining.\n",
      deleted, hashmap_count( &(c->chunkers) )
   );
   scaffold_assert( 0 == hashmap_count( &(c->chunkers) ) );

#endif /* USE_CHUNKS */

   /* Empty receiving buffer. */
   while( 0 < connection_read_line( &(c->link), buffer, c->client_side ) );

   client_clear_puppet( c );
   if( TRUE == c->client_side ) {
      hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }

   c->running = FALSE;

cleanup:
   bdestroy( buffer );
   return;
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
   int bstr_retval;
   bstring buffer = NULL;

   /* We won't record the channel in our list until the server confirms it. */

   scaffold_assert_client();

   buffer = bfromcstr( "PART " );
   scaffold_check_null( buffer );
   bstr_retval = bconcat( buffer, lname );
   scaffold_check_nonzero( bstr_retval );
   client_send( c, buffer );

   /* TODO: Add callback from parser and only delete channel on confirm. */
   hashmap_remove( &(c->channels), lname );

cleanup:
   bdestroy( buffer );
   return;
}

void client_send( struct CLIENT* c, const bstring buffer ) {
   int bstr_retval;
   bstring buffer_copy = NULL;

   /* TODO: Make sure we're still connected. */

   buffer_copy = bstrcpy( buffer );
   scaffold_check_null( buffer_copy );
   bstr_retval = bconchar( buffer_copy, '\r' );
   scaffold_check_nonzero( bstr_retval );
   bstr_retval = bconchar( buffer_copy, '\n' );
   scaffold_check_nonzero( bstr_retval );
   connection_write_line( &(c->link), buffer_copy, c->client_side );

#ifdef DEBUG_NETWORK
   if( TRUE == c->client_side ) {
      scaffold_print_debug( "Client sent to server: %s\n", bdata( buffer ) );
   } else {
      scaffold_print_debug( "Server sent to client %d: %s\n", c->link.socket, bdata( buffer ) );
   }
#endif /* DEBUG_NETWORK */

cleanup:
   bdestroy( buffer_copy );
   if( TRUE == c->client_side ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }
   return;
}

void client_printf( struct CLIENT* c, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   if( TRUE == c->client_side ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }

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

#ifdef USE_CHUNKS

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

   scaffold_print_debug( "Server: Adding chunker to send: %s\n", bdata( filepath ) );
   hashmap_put( &(c->chunkers), filepath, h );

cleanup:
   if( NULL != h ) {
      chunker_free( h );
   }
   return;
}

#endif /* USE_CHUNKS */

void client_set_puppet( struct CLIENT* c, struct MOBILE* o ) {
   if( NULL != c->puppet ) { /* Take care of existing mob before anything. */
      mobile_free( c->puppet );
      c->puppet = NULL;
   }
   if( NULL != o ) {
      refcount_inc( o, "mobile" ); /* Add first, to avoid deletion. */
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
#ifdef USE_CHUNKS
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
         h, type, filename, &str_client_cache_path
      );
      scaffold_print_debug( "Adding unchunker to receive: %s\n", bdata( filename ) );
      hashmap_put_nolock( &(c->chunkers), filename, h );
      scaffold_check_nonzero( scaffold_error );

      proto_request_file( c, filename, type );
   }

cleanup:
   hashmap_lock( &(c->chunkers), FALSE );
   return;
#else
#warning No file receiving method implemented!
#endif /* USE_CHUNKS */
}

#ifdef USE_CHUNKS

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
   bstring lname = NULL;
#ifdef USE_EZXML
   ezxml_t xml_data = NULL;
#endif /* USE_EZXML */

   assert( TRUE == chunker_unchunk_finished( h ) );

   switch( h->type ) {
   case CHUNKER_DATA_TYPE_TILEMAP:
      lname = bfromcstr( "" );
#ifdef USE_EZXML
      xml_data = datafile_tilemap_ezxml_peek_lname(
         (BYTE*)h->raw_ptr, h->raw_length, lname
      );
#endif /* USE_EZXML */

      l = client_get_channel_by_name( c, lname );
      scaffold_check_null_msg( l, "Unable to find channel to attach loaded tileset." );

#ifdef USE_EZXML
      datafile_parse_tilemap_ezxml_t( &(l->tilemap), xml_data, TRUE );
#endif /* USE_EZXML */

      /* Go through the parsed tilemap and load graphics. */
      hashmap_iterate( &(l->tilemap.tilesets), callback_proc_tileset_imgs, c );

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

      graphics_surface_new( g, 0, 0, 0, 0 );
      scaffold_check_null( g );
      graphics_set_image_data( g, h->raw_ptr, h->raw_length );
      scaffold_check_null_msg( g->surface, "Unable to load tileset image." );

      set = hashmap_iterate(
         &(l->tilemap.tilesets), callback_search_tilesets_img_name, h->filename
      );
      scaffold_check_null( set )

      hashmap_put( &(set->images), h->filename, g );
      scaffold_print_info(
         "Client: Tilemap image %s successfully loaded into tileset cache.\n",
         bdata( h->filename )
      );

      /* TODO: When do we ask for the mobs when not using chunkers? */
      proto_client_request_mobs( c, l );
      tilemap_set_redraw_state( &(l->tilemap), TILEMAP_REDRAW_ALL );
      break;

   case CHUNKER_DATA_TYPE_MOBSPRITES:
      graphics_surface_new( g, 0, 0, 0, 0 );
      scaffold_check_null( g );
      graphics_set_image_data( g, h->raw_ptr, h->raw_length );
      scaffold_check_null( g->surface );
      hashmap_put( &(c->sprites), h->filename, g );
      scaffold_print_info(
         "Client: Mobile spritesheet %s successfully loaded into cache.\n",
         bdata( h->filename )
      );
      break;
   }

cleanup:
   bdestroy( lname );
#ifdef USE_EZXML
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
#endif /* USE_EZXML */
   scaffold_print_debug(
      "Client: Removing finished chunker for: %s\n", bdata( h->filename )
   );
   chunker_free( h );
   hashmap_remove( &(c->chunkers), h->filename );
   return;
}

#endif /* USE_CHUNKS */

void client_poll_input( struct CLIENT* c ) {
   struct INPUT input;
   struct MOBILE_UPDATE_PACKET update;
   struct MOBILE* puppet = NULL;
#ifdef DEBUG_TILES
   bstring tilemap_dbg_key = NULL;
#endif /* DEBUG_TILES */

   scaffold_set_client();

   if( NULL != c ) {
      puppet = c->puppet;
   }

   input_get_event( &input );
   update.o = puppet;

   if( NULL != puppet ) {
      update.l = puppet->channel;
      scaffold_check_null( update.l );
   }

   if( INPUT_TYPE_KEY == input.type ) {
      if(
         NULL != puppet &&
         (puppet->steps_remaining < -8 || puppet->steps_remaining > 8)
      ) {
         goto cleanup; /* Silently ignore input until animations are done. */
      }

      switch( input.character ) {
      case 'q':
         if( NULL != puppet ) {
            proto_client_stop( c );
         } else {
            /* TODO: Emergency stop. */
         }
         break;
      case 'w':
         update.update = MOBILE_UPDATE_MOVEUP;
         if( NULL != puppet ) {
            proto_client_send_update( c, &update );
         }
         break;
      case 'a':
         update.update = MOBILE_UPDATE_MOVELEFT;
         if( NULL != puppet ) {
            proto_client_send_update( c, &update );
         }
         break;
      case 's':
         update.update = MOBILE_UPDATE_MOVEDOWN;
         if( NULL != puppet ) {
            proto_client_send_update( c, &update );
         }
         break;
      case 'd':
         update.update = MOBILE_UPDATE_MOVERIGHT;
         if( NULL != puppet ) {
            proto_client_send_update( c, &update );
         }
         break;
#ifdef DEBUG_TILES
      case 't':
         switch( tilemap_dt_state ) {
         case TILEMAP_DEBUG_TERRAIN_OFF:
            tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_COORDS;
            break;
         case TILEMAP_DEBUG_TERRAIN_COORDS:
            tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_NAMES;
            break;
         case TILEMAP_DEBUG_TERRAIN_NAMES:
            tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_QUARTERS;
            break;
         case TILEMAP_DEBUG_TERRAIN_QUARTERS:
            tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_OFF;
            break;
         }
         break;
      case 'l':
         tilemap_dt_layer++;
         break;
#endif /* DEBUG_TILES */
      }
   }

cleanup:
   return;
}

BOOL client_connected( struct CLIENT* c ) {
   if( 0 < c->link.socket ) {
      return TRUE;
   } else {
      return FALSE;
   }
}
