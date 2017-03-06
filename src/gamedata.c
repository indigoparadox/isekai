#include "gamedata.h"

#include <memory.h>

#include "channel.h"
#include "graphics.h"
#include "ui.h"
#include "datafile.h"
#include "input.h"
#include "callbacks.h"

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

   /* Don't cleanup UI since other things probably use that. */
}

void gamedata_init_client( struct GAMEDATA* d ) {
   d->incoming_buffer_len = 0;
   d->incoming_buffer = NULL;
   tilemap_init( &(d->tmap), FALSE );
   hashmap_init( &(d->incoming_chunkers) );
   vector_init( &(d->mobiles ) );
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

void gamedata_update_client( struct CLIENT* c, GRAPHICS* g, struct UI* ui ) {
   struct GAMEDATA* d = NULL;
   struct CHANNEL* l = NULL;
   struct GRAPHICS_TILE_WINDOW twindow = { 0 };

   l = hashmap_get_first( &(c->channels) );
   if( NULL == l ) {
      /* TODO: What to display when no channel is up? */
      goto cleanup;
   }

   d = &(l->gamedata);

   twindow.width = 640 / 32;
   twindow.height = 480 / 32;

   if( TILEMAP_SENTINAL == d->tmap.sentinal ) {
      tilemap_draw_ortho( &(d->tmap), g, &twindow );
      graphics_flip_screen( g );
   }

   gamedata_poll_input( d, c );

   vector_iterate( &(d->mobiles), callback_draw_mobiles, &twindow );

cleanup:
   return;
}

void gamedata_add_mobile( struct GAMEDATA* d, struct MOBILE* o ) {
   vector_set( &(d->mobiles), o->serial, o );
}

void gamedata_remove_mobile( struct GAMEDATA* d, size_t serial ) {
   vector_remove( &(d->mobiles), serial );
}
