#include "gamedata.h"

#include <memory.h>

#include "channel.h"
#include "graphics.h"
#include "ui.h"
#include "datafile.h"

static inline CHANNEL* gamedata_get_channel( GAMEDATA* d ) {
   return (CHANNEL*)d;
}

void gamedata_init_server( GAMEDATA* d, const bstring name ) {
   bstring mapdata_path = NULL;
   bstring mapdata_filename = NULL;

#ifdef INIT_ZEROES
   memset( d, '\0', sizeof( GAMEDATA ) );
#endif /* INIT_ZEROES */

   tilemap_init( &(d->tmap) );

   scaffold_print_info( "Loading data for channel: %s\n", bdata( name ) );
   mapdata_filename = bstrcpy( name );
   scaffold_check_null( mapdata_filename );
   bdelete( mapdata_filename, 0, 1 );
   mapdata_path = bformat( "./%s.tmx", bdata( mapdata_filename ) );
   scaffold_check_null( mapdata_path );
   scaffold_print_info( "Loading for data in: %s\n", bdata( mapdata_path ) );
   datafile_load_file( &(d->tmap), mapdata_path, datafile_parse_tilemap );

cleanup:
   bdestroy( mapdata_path );
   bdestroy( mapdata_filename );
   return;
}

void gamedata_cleanup( GAMEDATA* d ) {
   tilemap_free( &(d->tmap) );

   /* Don't cleanup UI since other things probably use that. */
}

void gamedata_init_client( GAMEDATA* d ) {
   //d->ui = ui;
   d->incoming_buffer_len = 0;
   d->incoming_buffer = NULL;
   tilemap_init( &(d->tmap) );
   //vector_init( &(d->incoming_chunkers ) );
   hashmap_init( &(d->incoming_chunkers) );
}

void gamedata_update_server(
   GAMEDATA* d, CLIENT* c, const struct bstrList* args,
   bstring* reply_c, bstring* reply_l
) {
   /* CHANNEL* l = gamedata_get_channel( d ); */


}

void gamedata_react_client(
   GAMEDATA* d, CLIENT* c, const struct bstrList* args, bstring* reply
) {
}

void gamedata_update_client( CLIENT* c, GRAPHICS* g, UI* ui ) {
   TILEMAP_TILESET* set = NULL;
   //TILEMAP_TILESET_IMAGE* image = NULL;
   GRAPHICS* image = NULL;
   GAMEDATA* d = NULL;
   CHANNEL* l = NULL;

   l = hashmap_get_first( &(c->channels) );
   if( NULL == l ) {
      /* TODO: What to display when no channel is up? */
      goto cleanup;
   }

   d = &(l->gamedata);

   if( 0 >= hashmap_count( &(d->tmap.tilesets) ) ) {
      /* TODO: Handle lack of tilesets. */
      goto cleanup;
   }

   set = hashmap_get_first( &(d->tmap.tilesets) );
   if( NULL != set ) {
      image = hashmap_get_first( &(set->images) );
   }

   if( NULL != image ) {
      graphics_blit( g, 0, 0, 32, 32, image );
      graphics_flip_screen( g );
   }

cleanup:
   return;
}
