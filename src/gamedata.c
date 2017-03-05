#include "gamedata.h"

#include <memory.h>

#include "channel.h"
#include "graphics.h"
#include "ui.h"
#include "datafile.h"
#include "input.h"

static inline CHANNEL* gamedata_get_channel( GAMEDATA* d ) {
   return (CHANNEL*)d;
}

void gamedata_init_server( GAMEDATA* d, const bstring name ) {
   bstring mapdata_path = NULL;
   bstring mapdata_filename = NULL;

#ifdef INIT_ZEROES
   memset( d, '\0', sizeof( GAMEDATA ) );
#endif /* INIT_ZEROES */

   tilemap_init( &(d->tmap), TRUE );

   hashmap_init( &(d->cached_gfx) );
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

void gamedata_cleanup( GAMEDATA* d ) {
   tilemap_free( &(d->tmap) );

   /* Don't cleanup UI since other things probably use that. */
}

void gamedata_init_client( GAMEDATA* d ) {
   hashmap_init( &(d->cached_gfx) );
   d->incoming_buffer_len = 0;
   d->incoming_buffer = NULL;
   tilemap_init( &(d->tmap), FALSE );
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

void gamedata_poll_input( GAMEDATA* d, CLIENT* c ) {
   INPUT input;

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

void gamedata_update_client( CLIENT* c, GRAPHICS* g, UI* ui ) {
   GAMEDATA* d = NULL;
   CHANNEL* l = NULL;
   TILEMAP_WINDOW twindow = { 0 };

   l = hashmap_get_first( &(c->channels) );
   if( NULL == l ) {
      /* TODO: What to display when no channel is up? */
      goto cleanup;
   }

   d = &(l->gamedata);

   twindow.cached_gfx = &(d->cached_gfx);
   twindow.width = 640 / 32;
   twindow.height = 480 / 32;

   if( TILEMAP_SENTINAL == d->tmap.sentinal ) {
      tilemap_draw_ortho( &(d->tmap), g, &twindow );
      graphics_flip_screen( g );
   }

   gamedata_poll_input( d, c );

cleanup:
   return;
}
