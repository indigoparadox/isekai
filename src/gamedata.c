#include "gamedata.h"

#include <memory.h>

#include "channel.h"
#include "graphics.h"
#include "ui.h"
#include "datafile.h"
#include "input.h"
#include "callbacks.h"

struct tagbstring str_gamedata_cache_path =
   bsStatic( "testdata/livecache" );
struct tagbstring str_gamedata_server_path =
   bsStatic( "testdata/server" );

static struct GRAPHICS_TILE_WINDOW* twindow = NULL;

static inline struct CHANNEL* gamedata_get_channel( struct GAMEDATA* d ) {
   return (struct CHANNEL*)d;
}

void gamedata_init_server( struct GAMEDATA* d, const bstring name ) {
   bstring mapdata_path = NULL;
   bstring mapdata_filename = NULL;

#ifdef INIT_ZEROES
   memset( d, '\0', sizeof( struct GAMEDATA ) );
#endif /* INIT_ZEROES */

   tilemap_init( &(d->tmap), TRUE );
   vector_init( &(d->mobiles ) );

   scaffold_print_info( "Loading data for channel: %s\n", bdata( name ) );
   mapdata_filename = bstrcpy( name );
   scaffold_check_null( mapdata_filename );
   bdelete( mapdata_filename, 0, 1 );
   mapdata_path = bformat( "./%s.tmx", bdata( mapdata_filename ) );
   scaffold_check_null( mapdata_path );
   scaffold_print_info( "Loading for data in: %s\n", bdata( mapdata_path ) );
   datafile_load_file( &(d->tmap), mapdata_path, TRUE, datafile_parse_tilemap );

cleanup:
   bdestroy( mapdata_path );
   bdestroy( mapdata_filename );
   return;
}

void gamedata_cleanup( struct GAMEDATA* d ) {
#ifdef DEBUG
   size_t deleted;
#endif /* DEBUG */

   tilemap_free( &(d->tmap) );

#ifdef DEBUG
   deleted =
#endif /* DEBUG */
      vector_remove_cb( &(d->mobiles), callback_free_mobiles, NULL );
   scaffold_print_debug(
      "Removed %d mobiles. %d remaining.\n",
      deleted, vector_count( &(d->mobiles) )
   );
   vector_free( &(d->mobiles) );

   hashmap_remove_cb( &(d->mob_sprites), callback_free_graphics, NULL );
   hashmap_init( &(d->mob_sprites ) );

   /* Don't cleanup UI since other things probably use that. */
}

void gamedata_init_client( struct GAMEDATA* d ) {
   d->incoming_buffer_len = 0;
   d->incoming_buffer = NULL;
   tilemap_init( &(d->tmap), FALSE );
   hashmap_init( &(d->incoming_chunkers) );
   vector_init( &(d->mobiles ) );
   hashmap_init( &(d->mob_sprites) );
}

void gamedata_update_server(
   struct GAMEDATA* d, struct CLIENT* c, const struct bstrList* args,
   bstring* reply_c, bstring* reply_l
) {
   /* struct CHANNEL* l = gamedata_get_channel( d ); */


}

void gamedata_react_client(
   struct GAMEDATA* d, struct CLIENT* c, const struct bstrList* args, bstring* reply
) {
}

void gamedata_poll_input( struct GAMEDATA* d, struct CLIENT* c ) {
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

void gamedata_update_client( struct CLIENT* c, GRAPHICS* g, struct UI* ui ) {
   struct GAMEDATA* d = NULL;
   struct CHANNEL* l = NULL;

   scaffold_set_client();

   /* Removed any finished chunkers. */
   /*
   hashmap_remove_cb(
      &(c->chunkers), callback_free_finished_unchunkers, NULL
   );
   */

   l = hashmap_get_first( &(c->channels) );
   if( NULL == l ) {
      /* TODO: What to display when no channel is up? */
      goto cleanup;
   }

   d = &(l->gamedata);
   if( NULL == twindow ) {
      /* TODO: Free this, somehow. */
      twindow = calloc( 1, sizeof( struct GRAPHICS_TILE_WINDOW ) );
   }

   twindow->width = 640 / 32;
   twindow->height = 480 / 32;
   twindow->g = g;
   twindow->t = &(d->tmap);

   if( TILEMAP_SENTINAL == d->tmap.sentinal ) {
      tilemap_draw_ortho( &(d->tmap), g, twindow );
      graphics_flip_screen( g );
   }

   gamedata_poll_input( d, c );

   vector_iterate( &(d->mobiles), callback_draw_mobiles, twindow );

cleanup:
   return;
}

void gamedata_add_mobile( struct GAMEDATA* d, struct MOBILE* o ) {
   vector_set( &(d->mobiles), o->serial, o, TRUE );
}

void gamedata_remove_mobile( struct GAMEDATA* d, size_t serial ) {
   vector_remove( &(d->mobiles), serial );
}

void gamedata_process_data_block(
   struct GAMEDATA* d, struct CLIENT* c, struct CHUNKER_PROGRESS* progress
) {
   struct CHANNEL_CLIENT* lc = NULL;
   struct TILEMAP_TILESET* set = NULL;
   GRAPHICS* g = NULL;
   struct CHUNKER* h = NULL;
   struct CHANNEL* l = NULL;
   int8_t chunker_percent;

   assert( 0 < blength( progress->data ) );
   assert( 0 < blength( progress->filename ) );

   l = scaffold_container_of( d, struct CHANNEL, gamedata );
   scaffold_check_null( l );

   if( progress->current > progress->total ) {
      scaffold_print_error( "Invalid progress for %s.\n", bdata( progress->filename ) );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   if( 0 < d->incoming_buffer_len && d->incoming_buffer_len != progress->total ) {
      scaffold_print_error( "Invalid total for %s.\n", bdata( progress->filename ) );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   h = hashmap_get( &(d->incoming_chunkers), progress->filename );
   if( NULL == h ) {
      h = (struct CHUNKER*)calloc( 1, sizeof( struct CHUNKER ) );
      chunker_unchunk_start(
         h, l->name, progress->type, progress->total, progress->filename,
         &str_gamedata_cache_path
      );
      hashmap_put( &(d->incoming_chunkers), progress->filename, h );
      scaffold_check_nonzero( scaffold_error );
   }

   chunker_unchunk_pass( h, progress->data, progress->current, progress->chunk_size );

   chunker_percent = chunker_unchunk_percent_progress( h, FALSE );
   if( 0 < chunker_percent ) {
      scaffold_print_debug( "Chunker: %s: %d%%\n", bdata( h->filename ), chunker_percent );
   }

   if( chunker_unchunk_finished( h ) ) {
      /* Cached file found, so abort transfer. */
      if( TRUE == h->force_finish ) {
         scaffold_print_debug(
            "Client: Aborting transfer of %s from server due to cached copy.\n",
            bdata( progress->filename )
         );
         client_printf( c, "GDA %b", progress->filename );
      }

      switch( h->type ) {
      case CHUNKER_DATA_TYPE_TILEMAP:
         datafile_parse_tilemap( &(d->tmap), progress->filename, (BYTE*)h->raw_ptr, h->raw_length );

         /* Go through the parsed tilemap and load graphics. */
         lc = (struct CHANNEL_CLIENT*)calloc( 1, sizeof( struct CHANNEL_CLIENT ) );
         lc->l = l;
         lc->c = c;
         hashmap_iterate( &(d->tmap.tilesets), callback_proc_tileset_imgs, lc );
         free( lc );

         scaffold_print_info(
            "Client: Tilemap for %s successfully loaded into cache.\n", bdata( l->name )
         );
         break;

      case CHUNKER_DATA_TYPE_TILESET_IMG:
         graphics_surface_new( g, 0, 0, 0, 0 );
         scaffold_check_null( g );
         graphics_set_image_data( g, h->raw_ptr, h->raw_length );
         scaffold_check_null( g->surface );
         set = hashmap_iterate( &(l->gamedata.tmap.tilesets), callback_search_tilesets_img_name, progress->filename );
         scaffold_check_null( set )
         hashmap_put( &(set->images), progress->filename, g );
         scaffold_print_info(
            "Client: Tilemap image %s successfully loaded into cache.\n",
            bdata( progress->filename )
         );
         break;

      case CHUNKER_DATA_TYPE_MOBSPRITES:
         graphics_surface_new( g, 0, 0, 0, 0 );
         scaffold_check_null( g );
         graphics_set_image_data( g, h->raw_ptr, h->raw_length );
         scaffold_check_null( g->surface );
         hashmap_put( &(d->mob_sprites), progress->filename, g );
         scaffold_print_info(
            "Client: Mobile spritesheet %s successfully loaded into cache.\n",
            bdata( progress->filename )
         );
         break;
      }
   }

cleanup:
   return;
}
