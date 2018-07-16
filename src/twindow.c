
#define TWINDOW_C
#include "twindow.h"

#include "tilemap.h"
#include "client.h"
#include "callback.h"

void twindow_update_details( struct TWINDOW* twindow ) {
   struct TILEMAP_POSITION smallest_tile = { 0 };
   struct CLIENT* c = NULL;
   struct TILEMAP* t = NULL;

   scaffold_print_debug( &module, "Setting up tilemap screen window...\n" );

   c = scaffold_container_of( twindow, struct CLIENT, local_window );
   t = c->active_tilemap;

   /* smallest_tile will have the dimensions of the smallest tileset's tiles. */
   vector_iterate(
      &(t->tilesets), callback_search_tilesets_small, &smallest_tile );
   twindow->grid_w = smallest_tile.x;
   twindow->grid_h = smallest_tile.y;
   scaffold_print_debug(
      &module, "Smallest tile: %d x %d\n", twindow->grid_w, twindow->grid_h );
   twindow->width = GRAPHICS_SCREEN_WIDTH / twindow->grid_w;
   twindow->height = GRAPHICS_SCREEN_HEIGHT / twindow->grid_h;
}
