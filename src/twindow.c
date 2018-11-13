
#define TWINDOW_C
#include "twindow.h"

#include "tilemap.h"
#include "client.h"
#include "callback.h"

struct UI;

struct TWINDOW {
   GRAPHICS* g;            /*!< Graphics element to draw on. */
   TILEMAP_COORD_TILE x;        /*!< Window left in tiles. */
   TILEMAP_COORD_TILE y;        /*!< Window top in tiles. */
   TILEMAP_COORD_TILE width;    /*!< Window width in tiles. */
   TILEMAP_COORD_TILE height;   /*!< Window height in tiles. */
   TILEMAP_COORD_TILE max_x;    /*!< Right-most window tile. */
   TILEMAP_COORD_TILE max_y;    /*!< Bottom-most window tile. */
   TILEMAP_COORD_TILE min_x;    /*!< Left-most window tile. */
   TILEMAP_COORD_TILE min_y;    /*!< Top-most window tile. */
   GFX_COORD_PIXEL grid_w;
   GFX_COORD_PIXEL grid_h;
   struct CLIENT* local_client;
   struct UI* ui;
};

struct TWINDOW* twindow_new() {
   struct TWINDOW* out = NULL;

   out = mem_alloc( 1, struct TWINDOW );
   lgc_null( out );

   out->local_client = NULL;
   out->ui = NULL;
   out->g = NULL;

cleanup:
   return out;
}

void twindow_update_details( struct TWINDOW* twindow ) {
   struct TILEMAP_POSITION smallest_tile = { 0 };
   struct TILEMAP* t = NULL;

   lg_debug( __FILE__, "Setting up tilemap screen window...\n" );

   t = twindow_get_tilemap_active( twindow );
   lgc_null( t );

   /* smallest_tile will have the dimensions of the smallest tileset's tiles. */
   vector_iterate(
      t->tilesets, callback_search_tilesets_small, &smallest_tile );
   twindow->grid_w = smallest_tile.x;
   twindow->grid_h = smallest_tile.y;
   lg_debug(
      __FILE__, "Smallest tile: %d x %d\n", twindow->grid_w, twindow->grid_h );
   twindow->width = GRAPHICS_SCREEN_WIDTH / twindow->grid_w;
   twindow->height = GRAPHICS_SCREEN_HEIGHT / twindow->grid_h;

cleanup:
   return;
}

struct UI* twindow_get_ui( const struct TWINDOW* w ) {
   struct UI* ui = NULL;
   lgc_null( w );
   ui = w->ui;
cleanup:
   return ui;
}

struct GRAPHICS* twindow_get_screen( const struct TWINDOW* w ) {
   struct GRAPHICS* g = NULL;
   lgc_null( w );

   g = w->g;

cleanup:
   return g;
}

GFX_COORD_PIXEL twindow_get_grid_w( const struct TWINDOW* w ) {
   if( NULL != w ) {
      return w->grid_w;
   }
   return 0;
}

GFX_COORD_PIXEL twindow_get_grid_h( const struct TWINDOW* w ) {
   if( NULL != w ) {
      return w->grid_h;
   }
   return 0;
}

TILEMAP_COORD_TILE twindow_get_x( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->x;
}

TILEMAP_COORD_TILE twindow_get_y( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->y;
}

TILEMAP_COORD_TILE twindow_get_width( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->width;
}

TILEMAP_COORD_TILE twindow_get_height( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->height;
}

TILEMAP_COORD_TILE twindow_get_max_x( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->max_x;
}

TILEMAP_COORD_TILE twindow_get_max_y( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->max_y;
}

TILEMAP_COORD_TILE twindow_get_min_x( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->min_x;
}

TILEMAP_COORD_TILE twindow_get_min_y( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->min_y;
}

void twindow_set_x( struct TWINDOW* w, TILEMAP_COORD_TILE x ) {
   scaffold_assert( NULL != w );
   w->x = x;
}

void twindow_set_y( struct TWINDOW* w, TILEMAP_COORD_TILE y ) {
   scaffold_assert( NULL != w );
   w->y = y;
}

void twindow_set_max_x( struct TWINDOW* w, TILEMAP_COORD_TILE x ) {
   scaffold_assert( NULL != w );
   w->max_x = x;
}

void twindow_set_max_y( struct TWINDOW* w, TILEMAP_COORD_TILE y ) {
   scaffold_assert( NULL != w );
   w->max_y = y;
}

void twindow_set_min_x( struct TWINDOW* w, TILEMAP_COORD_TILE x ) {
   scaffold_assert( NULL != w );
   w->min_x = x;
}

void twindow_set_min_y( struct TWINDOW* w, TILEMAP_COORD_TILE y ) {
   scaffold_assert( NULL != w );
   w->min_y = y;
}

void twindow_shrink_height( struct TWINDOW* w, TILEMAP_COORD_TILE h_shrink ) {
   scaffold_assert( NULL != w );
   w->height -= h_shrink;
}

void twindow_grow_height( struct TWINDOW* w, TILEMAP_COORD_TILE h_grow ) {
   scaffold_assert( NULL != w );
   w->height += h_grow;
}

void twindow_shift_left_x( struct TWINDOW* w, TILEMAP_COORD_TILE x_left ) {
   scaffold_assert( NULL != w );
   w->x -= x_left;
}

void twindow_shift_right_x( struct TWINDOW* w, TILEMAP_COORD_TILE x_right ) {
   scaffold_assert( NULL != w );
   w->x += x_right;
}

void twindow_shift_up_y( struct TWINDOW* w, TILEMAP_COORD_TILE y_up ) {
   scaffold_assert( NULL != w );
   w->y -= y_up;
}

void twindow_shift_down_y( struct TWINDOW* w, TILEMAP_COORD_TILE y_down ) {
   scaffold_assert( NULL != w );
   w->y += y_down;
}

void twindow_set_ui( struct TWINDOW* w, struct UI* ui ) {
   scaffold_assert( NULL != w );
   w->ui = ui;
}

void twindow_set_screen( struct TWINDOW* w, struct GRAPHICS* g ) {
   scaffold_assert( NULL != w );
   w->g = g;
}

void twindow_set_local_client( struct TWINDOW* w, struct CLIENT* c ) {
   scaffold_assert( NULL != w );
   w->local_client = c;
}

struct TILEMAP* twindow_get_tilemap_active( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   scaffold_assert( NULL != w->local_client );
   return client_get_tilemap_active( w->local_client );
}

struct CLIENT* twindow_get_local_client( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->local_client;
}

TILEMAP_COORD_TILE twindow_get_right( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->x + w->width;
}

TILEMAP_COORD_TILE twindow_get_bottom( const struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   return w->y + w->height;
}
