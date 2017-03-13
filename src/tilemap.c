#include "tilemap.h"

#ifdef DEBUG_TILES
volatile TILEMAP_DEBUG_TERRAIN_STATE tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_OFF;
volatile uint8_t tilemap_dt_layer = 0;
#endif /* DEBUG_TILES */

extern struct CLIENT* main_client;

#include <memory.h>
#include <string.h>

#include "callback.h"
#include "mobile.h"

static BOOL tilemap_layer_free_cb( bstring res, void* iter, void* arg ) {
   tilemap_layer_free( (struct TILEMAP_LAYER*)iter );
   return TRUE;
}

static BOOL tilemap_tileset_free_cb( bstring res, void* iter, void* arg ) {
   tilemap_tileset_free( (struct TILEMAP_TILESET*)iter );
   return TRUE;
}

static void tilemap_cleanup( const struct REF* ref ) {
   struct TILEMAP* t;

   t = scaffold_container_of( ref, struct TILEMAP, refcount );

   hashmap_remove_cb( &(t->layers), tilemap_layer_free_cb, NULL );
   hashmap_cleanup( &(t->layers) );
   hashmap_remove_cb( &(t->player_spawns), callback_free_generic, NULL );
   hashmap_cleanup( &(t->player_spawns) );
   hashmap_remove_cb( &(t->tilesets), tilemap_tileset_free_cb, NULL );
   hashmap_cleanup( &(t->tilesets) );

   /* TODO: Free tilemap. */
}

void tilemap_init( struct TILEMAP* t, BOOL local_images ) {
   ref_init( &(t->refcount), tilemap_cleanup );

   hashmap_init( &(t->layers) );
   hashmap_init( &(t->tilesets) );
   hashmap_init( &(t->player_spawns) );

   bfromcstralloc( TILEMAP_NAME_ALLOC, "" );

   t->orientation = TILEMAP_ORIENTATION_ORTHO;
   t->lname = bfromcstr( "" );
}

void tilemap_free( struct TILEMAP* t ) {
   refcount_dec( t, "tilemap" );
}

void tilemap_layer_init( struct TILEMAP_LAYER* layer ) {
   vector_init( &(layer->tiles) );
}

void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer ) {
   vector_free( &(layer->tiles ) );
}

void tilemap_tileset_free( struct TILEMAP_TILESET* tileset ) {
   hashmap_remove_cb( &(tileset->images), callback_free_graphics, NULL );
}

void tilemap_iterate_screen_row(
   struct TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( struct TILEMAP* t, uint32_t x, uint32_t y )
) {

}

/** \brief Given a tile GID, get the tileset it belongs to.
 * \param[in] t   The tilemap on which the tile resides.
 * \param[in] gid The GID of the tile information to fetch.
 * \return The tileset containing the tile with the requested GID.
 */
SCAFFOLD_INLINE struct TILEMAP_TILESET* tilemap_get_tileset(
   struct TILEMAP* t, SCAFFOLD_SIZE gid
) {
   return hashmap_iterate( &(t->tilesets), callback_search_tilesets_gid, &gid );
}

/** \brief Get the tileset position for the given tile GID.
 * \param[in]  set   The tileset to fetch from.
 * \param[in]  g_set
 * \param[in]  gid
 * \param[out] x
 * \param[out] y
 * \return
 */
SCAFFOLD_INLINE void tilemap_get_tile_tileset_pos(
   struct TILEMAP_TILESET* set, GRAPHICS* g_set, SCAFFOLD_SIZE gid,
   SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
) {
   SCAFFOLD_SIZE tiles_wide = 0;
   SCAFFOLD_SIZE tiles_high = 0;

   scaffold_check_null( g_set );

   tiles_wide = g_set->w / set->tilewidth;
   tiles_high = g_set->h / set->tileheight;

   gid -= set->firstgid - 1;

   *y = ((gid - 1) / tiles_wide) * set->tileheight;
   *x = ((gid - 1) % tiles_wide) * set->tilewidth;

   /* scaffold_assert( *y < (set->tileheight * tiles_high) );
   scaffold_assert( *x < (set->tilewidth * tiles_wide) ); */
cleanup:
   return;
}

/** \brief Get the GID of the tile at the given position on the given layer.
 * \param[in] layer  Layer from which to fetch the tile.
 * \param[in] x      X coordinate of the tile to fetch.
 * \param[in] y      Y coordinate of the tile to fetch.
 * \return A GID that can be used to find the tile's image or terrain info.
 */
SCAFFOLD_INLINE uint32_t tilemap_get_tile(
   struct TILEMAP_LAYER* layer, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
) {
   SCAFFOLD_SIZE index = (y * layer->width) + x;
   return vector_get_scalar( &(layer->tiles), index );
}

static void* tilemap_layer_draw_tile(
   struct TILEMAP_LAYER* layer, struct GRAPHICS_TILE_WINDOW* twindow,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE gid
) {
   struct TILEMAP_TILESET* set = NULL;
   SCAFFOLD_SIZE
      tileset_x = 0,
      tileset_y = 0,
      pix_x = 0,
      pix_y = 0;
   struct TILEMAP* t = twindow->t;
   const struct MOBILE* o = twindow->c->puppet;
   GRAPHICS* g_tileset = NULL;
#ifdef DEBUG_TILES
   bstring bnum = NULL;
   struct TILEMAP_TILE_DATA* tile_info = NULL;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;
   SCAFFOLD_SIZE td_i;

   bnum = bfromcstralloc( 10, "" );
   scaffold_check_null( bnum );
#endif /* DEBUG_TILES */

   set = tilemap_get_tileset( t, gid );
   scaffold_check_null( set );
   scaffold_check_zero( set->tilewidth );
   scaffold_check_zero( set->tileheight );

   /* Figure out the window position to draw to. */
   pix_x = set->tilewidth * (x - twindow->x);
   pix_y = set->tileheight * (y - twindow->y);

   /* Figure out the graphical tile to draw from. */
   /* TODO: Support multiple images. */
   g_tileset = (GRAPHICS*)hashmap_get_first( &(set->images) );
   if( NULL == g_tileset ) {
      /* TODO: Use a built-in placeholder tileset. */
      goto cleanup;
   }
   tilemap_get_tile_tileset_pos( set, g_tileset, gid, &tileset_x, &tileset_y );

   if( tilemap_inside_inner_map_x( o->x, o->y, twindow ) ) {
      pix_x += mobile_get_steps_remaining_x( twindow->c->puppet, TRUE );
   }

   if( tilemap_inside_inner_map_y( o->x, o->y, twindow ) ) {
      pix_y += mobile_get_steps_remaining_y( twindow->c->puppet, TRUE );
   }

   graphics_blit_partial(
      twindow->g,
      pix_x, pix_y,
      tileset_x, tileset_y,
      set->tilewidth, set->tileheight,
      g_tileset
   );

#ifdef DEBUG_TILES
   if( hashmap_count( &(t->layers) ) <= tilemap_dt_layer ) {
      tilemap_dt_layer = 0;
   }

   if( layer->z != tilemap_dt_layer ) {
      /* Don't bother with the debug stuff for another layer. */
      goto cleanup;
   }

   tile_info = vector_get( &(set->tiles), gid - 1 );
   switch( tilemap_dt_state ) {
   case TILEMAP_DEBUG_TERRAIN_COORDS:
      if( hashmap_count( &(t->layers) ) - 1 == layer->z ) {
         graphics_set_color( twindow->g, GRAPHICS_COLOR_DARK_BLUE );
         bassignformat( bnum, "%d,", x );
         graphics_draw_text( twindow->g, pix_x + 16, pix_y + 10, bnum );
         bassignformat( bnum, "%d", y );
         graphics_draw_text( twindow->g, pix_x + 16, pix_y + 22, bnum );
         bdestroy( bnum );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_NAMES:
      if( NULL != tile_info && NULL != tile_info->terrain[0] ) {
         bassignformat(
            bnum, "%c%c:%d",
            bdata( tile_info->terrain[0]->name )[0],
            bdata( tile_info->terrain[0]->name )[1],
            tile_info->terrain[0]->movement
         );
         graphics_draw_text(
            twindow->g,
            pix_x + 16, pix_y + (10 * layer->z), bnum
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_QUARTERS:
      for( td_i = 0 ; 4 > td_i ; td_i++ ) {
         if( NULL == tile_info || NULL == tile_info->terrain[td_i] ) {
            bassignformat( bnum, "x" );
         } else {
            bassignformat(
               bnum, "%d",
               tile_info->terrain[td_i]->id
            );
         }
         graphics_set_color( twindow->g, td_i + 4 );
         graphics_draw_text(
            twindow->g,
            pix_x + ((td_i % 2) * 12),
            pix_y + ((td_i / 2) * 16),
            bnum
         );
      }
      break;
   }
#endif /* DEBUG_TILES */

cleanup:
#ifdef DEBUG_TILES
   bdestroy( bnum );
#endif /* DEBUG_TILES */
   return NULL;
}

static void* tilemap_layer_draw_cb( bstring key, void* iter, void* arg ) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP* t = twindow->t;
   SCAFFOLD_SIZE_SIGNED
      x = 0,
      y = 0;
   uint32_t tile;
   struct VECTOR* tiles = NULL;

   tiles = &(layer->tiles);

   if( NULL == tiles || 0 == vector_count( tiles ) ) {
      goto cleanup;
   }

   for( x = twindow->min_x ; twindow->max_x > x ; x++ ) {
      for( y = twindow->min_y ; twindow->max_y > y ; y++ ) {
         tile = tilemap_get_tile( layer, x, y );
         if( 0 == tile ) {
            continue;
         }

         tilemap_layer_draw_tile( layer,twindow, x, y, tile );
      }
   }

cleanup:
   return NULL;
}

void tilemap_draw_ortho( struct GRAPHICS_TILE_WINDOW* twindow ) {
   hashmap_iterate( &(twindow->t->layers), tilemap_layer_draw_cb, twindow );
}

/** \brief
 *
 * \param x X coordinate in tiles.
 * \param y Y coordinate in tiles.
 * \return
 */
SCAFFOLD_INLINE BOOL tilemap_inside_inner_map_x(
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, struct GRAPHICS_TILE_WINDOW* twindow
) {
   const SCAFFOLD_SIZE window_half_width_tiles = twindow->width / 2;
   return
      x > window_half_width_tiles &&
      x < (twindow->t->width - window_half_width_tiles) + 2;
}

/** \brief
 *
 * \param x X coordinate in tiles.
 * \param y Y coordinate in tiles.
 * \return
 */
SCAFFOLD_INLINE BOOL tilemap_inside_inner_map_y(
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, struct GRAPHICS_TILE_WINDOW* twindow
) {
   const SCAFFOLD_SIZE window_half_height_tiles = twindow->height / 2;
   return
      y > window_half_height_tiles &&
      y < (twindow->t->height - window_half_height_tiles) + 2;
}

void tilemap_update_window_ortho( struct GRAPHICS_TILE_WINDOW* twindow, struct CLIENT* c ) {
   struct MOBILE* puppet = c->puppet;
   SCAFFOLD_SIZE_SIGNED
      border_x = twindow->x == 0 ? 0 : TILEMAP_BORDER,
      border_y = twindow->y == 0 ? 0 : TILEMAP_BORDER;
   struct TILEMAP* t = twindow->t;

   /* TODO: Only calculate these when window moves and store them. */
   twindow->max_x = twindow->x + twindow->width + border_x < t->width ?
      twindow->x + twindow->width + border_x : twindow->t->width;
   twindow->max_y = twindow->y + twindow->height + border_y < t->height ?
      twindow->y + twindow->height + border_y : t->height;

   twindow->min_x =
      twindow->x - border_x >= 0 &&
      twindow->x + twindow->width <= t->width
      ? twindow->x - border_x : 0;
   twindow->min_y =
      twindow->y - border_y >= 0 &&
      twindow->y + twindow->height <= t->height
      ? twindow->y - border_y : 0;

   if( tilemap_inside_inner_map_x( puppet->x, puppet->y, twindow ) ) {
      twindow->x = puppet->x - (twindow->width / 2) - 1;
   }
   if( tilemap_inside_inner_map_y( puppet->x, puppet->y, twindow ) ) {
      twindow->y = puppet->y - (twindow->height / 2) - 1;
   }
}
