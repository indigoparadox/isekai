
#define TILEMAP_C
#include "tilemap.h"

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

   vector_init( &(t->dirty_tiles) );

   tilemap_set_redraw_state( t, TILEMAP_REDRAW_DIRTY );

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

#ifdef DEBUG_TILES

static void tilemap_layer_draw_tile_debug(
   struct TILEMAP_LAYER* layer, GRAPHICS* g, struct GRAPHICS_TILE_WINDOW* twin,
   SCAFFOLD_SIZE_SIGNED tile_x, SCAFFOLD_SIZE_SIGNED tile_y,
   SCAFFOLD_SIZE_SIGNED pix_x, SCAFFOLD_SIZE_SIGNED pix_y, uint32_t gid
) {
   struct TILEMAP_TILESET* set = NULL;
   bstring bnum = NULL;
   struct TILEMAP_TILE_DATA* tile_info = NULL;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;
   SCAFFOLD_SIZE td_i;
   int bstr_result;
   struct TILEMAP* t = layer->tilemap;

   bnum = bfromcstralloc( 10, "" );
   scaffold_check_null( bnum );

   set = tilemap_get_tileset( t, gid );
   scaffold_check_null( set );
   scaffold_check_zero( set->tilewidth );
   scaffold_check_zero( set->tileheight );

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
         graphics_set_color( g, GRAPHICS_COLOR_DARK_BLUE );
         bstr_result = bassignformat( bnum, "%d,", tile_x );
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 10, GRAPHICS_TEXT_ALIGN_CENTER, bnum
         );
         bstr_result = bassignformat( bnum, "%d", tile_y );
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 22, GRAPHICS_TEXT_ALIGN_CENTER, bnum
         );
         bdestroy( bnum );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_NAMES:
      if( NULL != tile_info && NULL != tile_info->terrain[0] ) {
         bstr_result = bassignformat(
            bnum, "%c%c:%d",
            bdata( tile_info->terrain[0]->name )[0],
            bdata( tile_info->terrain[0]->name )[1],
            tile_info->terrain[0]->movement
         );
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + (10 * layer->z),
            GRAPHICS_TEXT_ALIGN_CENTER, bnum
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_QUARTERS:
      for( td_i = 0 ; 4 > td_i ; td_i++ ) {
         if( NULL == tile_info || NULL == tile_info->terrain[td_i] ) {
            bstr_result = bassignformat( bnum, "x" );
            scaffold_check_nonzero( bstr_result );
         } else {
            bstr_result = bassignformat(
               bnum, "%d",
               tile_info->terrain[td_i]->id
            );
            scaffold_check_nonzero( bstr_result );
         }
         graphics_set_color( g, td_i + 4 );
         graphics_draw_text(
            g,
            pix_x + ((td_i % 2) * 12),
            pix_y + ((td_i / 2) * 16),
            GRAPHICS_TEXT_ALIGN_CENTER,
            bnum
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_DEADZONE:
      if(
         !tilemap_inside_inner_map_x( tile_x, twin ) &&
         !tilemap_inside_inner_map_y( tile_y, twin )
      ) {
         graphics_set_color( g, GRAPHICS_COLOR_DARK_RED );
         graphics_draw_rect( g, pix_x, pix_y, 32, 32 );
      }
      if(
         !tilemap_inside_window_deadzone_x( tile_x, twin ) &&
         !tilemap_inside_window_deadzone_y( tile_y, twin )
      ) {
         graphics_set_color( g, GRAPHICS_COLOR_DARK_CYAN );
         graphics_draw_rect( g, pix_x, pix_y, 32, 32 );
      }
      break;
   }

cleanup:
   bdestroy( bnum );
   return;
}

#endif /* DEBUG_TILES */

static void* tilemap_layer_draw_tile(
   struct TILEMAP_LAYER* layer, struct GRAPHICS_TILE_WINDOW* twindow,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE gid
) {
   struct TILEMAP_TILESET* set = NULL;
   SCAFFOLD_SIZE
      tileset_x = 0,
      tileset_y = 0;
   SCAFFOLD_SIZE_SIGNED
      pix_x = 0,
      pix_y = 0;
   struct TILEMAP* t = twindow->t;
   const struct MOBILE* o = twindow->c->puppet;
   GRAPHICS* g_tileset = NULL;

   set = tilemap_get_tileset( t, gid );
   if( NULL == set ) {
      goto cleanup; /* Silently. */
   }

   scaffold_check_zero( set->tilewidth );
   scaffold_check_zero( set->tileheight );

   /* Figure out the window position to draw to. */
   pix_x = set->tilewidth * (x - twindow->x);
   pix_y = set->tileheight * (y - twindow->y);

   if( 0 > pix_x || 0 > pix_y ) {
      goto cleanup; /* Silently. */
   }

   /* Figure out the graphical tile to draw from. */
   /* TODO: Support multiple images. */
   g_tileset = (GRAPHICS*)hashmap_get_first( &(set->images) );
   if( NULL == g_tileset ) {
      /* TODO: Use a built-in placeholder tileset. */
      goto cleanup;
   }
   tilemap_get_tile_tileset_pos( set, g_tileset, gid, &tileset_x, &tileset_y );

   if(
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_x( o->x + 1, twindow ) &&
       TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_inner_map_x( o->x, twindow )
      ) || (
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_x( o->x - 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_x( o->x, twindow )
      )
   ) {
      pix_x += mobile_get_steps_remaining_x( twindow->c->puppet, TRUE );
   }

   if(
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_y( o->y + 1, twindow )  &&
       TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_inner_map_y( o->y, twindow )
      ) || (
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_y( o->y - 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_y( o->y, twindow )
      )
   ) {
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
   tilemap_layer_draw_tile_debug(
      layer, twindow->g, twindow, x, y, pix_x, pix_y, gid
   );
#endif /* DEBUG_TILES */

cleanup:
   return NULL;
}

static void* tilemap_layer_draw_dirty_cb( bstring key, void* iter, void* arg ) {
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct VECTOR* v = NULL;
   struct TILEMAP* t = twindow->t;
   int i;
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   uint32_t tile;

   scaffold_check_null( t );

   layer = t->first_layer;
   while( NULL != layer ) {
      tile = tilemap_get_tile( layer, pos->x, pos->y );
      tilemap_layer_draw_tile( layer, twindow, pos->x, pos->y, tile );
      layer = layer->next_layer;
   }

cleanup:
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
   if( NULL == twindow->t ) {
      return;
   }

   if(
      TILEMAP_REDRAW_ALL == twindow->t->redraw_state
#ifdef DEBUG_TILES
      || TILEMAP_DEBUG_TERRAIN_OFF != tilemap_dt_state
#endif /* DEBUG_TILES */
   ) {
      hashmap_iterate( &(twindow->t->layers), tilemap_layer_draw_cb, twindow );
   } else if(
      TILEMAP_REDRAW_DIRTY == twindow->t->redraw_state &&
      0 < vector_count( &(twindow->t->dirty_tiles ) )
   ) {
      vector_iterate(
         &(twindow->t->dirty_tiles), tilemap_layer_draw_dirty_cb, twindow
      );
   }

   /* If we've done a full redraw as requested then switch back to just dirty *
    * tiles.                                                                  */
   if(
      0 != twindow->c->puppet->steps_remaining &&
      TILEMAP_REDRAW_ALL != twindow->t->redraw_state
   ) {
      tilemap_set_redraw_state( twindow->t, TILEMAP_REDRAW_ALL );
   } else if(
      0 == twindow->c->puppet->steps_remaining &&
      TILEMAP_REDRAW_DIRTY != twindow->t->redraw_state
   ) {
      tilemap_set_redraw_state( twindow->t, TILEMAP_REDRAW_DIRTY );
   }
}

/** \brief Determine if the following coordinate is inside the area that can be
 *         displayed by the tilemap window without going off the edge.
 * \param x X coordinate in tiles.
 * \return
 */
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_x(
   SCAFFOLD_SIZE x, struct GRAPHICS_TILE_WINDOW* twindow
) {
   struct TILEMAP* t = twindow->t;
   if( x < ((twindow->width / 2) - TILEMAP_DEAD_ZONE_X) ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( x >= t->width - ((twindow->width / 2) - TILEMAP_DEAD_ZONE_X) ) {
      return TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN;
   } else {
      return TILEMAP_EXCLUSION_INSIDE;
   }
}

/** \brief Determine if the following coordinate is inside the area that can be
 *         displayed by the tilemap window without going off the edge.
 * \param y Y coordinate in tiles.
 * \return
 */
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_y(
   SCAFFOLD_SIZE y, struct GRAPHICS_TILE_WINDOW* twindow
) {
   struct TILEMAP* t = twindow->t;
   if(
      y < ((twindow->height / 2) - TILEMAP_DEAD_ZONE_Y)
   ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( y >= t->height - ((twindow->height / 2) - TILEMAP_DEAD_ZONE_Y) ) {
      return TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN;
   } else {
      return TILEMAP_EXCLUSION_INSIDE;
   }
}

SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_x(
   SCAFFOLD_SIZE x, struct GRAPHICS_TILE_WINDOW* twindow
) {
   SCAFFOLD_SIZE twindow_middle_x = 0;

   twindow_middle_x = (twindow->x + (twindow->width / 2));

   if( x < twindow_middle_x - TILEMAP_DEAD_ZONE_X ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( x > twindow_middle_x + TILEMAP_DEAD_ZONE_X ) {
      return TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN;
   } else {
      return TILEMAP_EXCLUSION_INSIDE;
   }
}

SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_y(
   SCAFFOLD_SIZE y, struct GRAPHICS_TILE_WINDOW* twindow
) {
   SCAFFOLD_SIZE twindow_middle_y = 0;

   twindow_middle_y = (twindow->y + (twindow->height / 2));

   if( y < twindow_middle_y - TILEMAP_DEAD_ZONE_Y ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( y >twindow_middle_y + TILEMAP_DEAD_ZONE_Y ) {
      return TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN;
   } else {
      return TILEMAP_EXCLUSION_INSIDE;
   }
}

void tilemap_update_window_ortho(
   struct GRAPHICS_TILE_WINDOW* twindow,
   SCAFFOLD_SIZE focal_x, SCAFFOLD_SIZE focal_y
) {
   SCAFFOLD_SIZE_SIGNED
      border_x = twindow->x == 0 ? 0 : TILEMAP_BORDER,
      border_y = twindow->y == 0 ? 0 : TILEMAP_BORDER;
   struct TILEMAP* t = twindow->t;
   SCAFFOLD_SIZE twindow_middle_x, twindow_middle_y;
   TILEMAP_EXCLUSION exclusion;

   if( NULL == t ) {
      return;
   }

   twindow_middle_x = (twindow->x + (twindow->width / 2));
   twindow_middle_y = (twindow->y + (twindow->height / 2));

   /* Find the focal point if we're not centered on it. */
   if( focal_x < twindow->x || focal_x > twindow->x + twindow->width ) {
      twindow->x = focal_x - (twindow->width / 2);
   }
   if( focal_y < twindow->y || focal_y > twindow->y + twindow->height ) {
      twindow->y = focal_y - (twindow->height / 2);
   }

   /* Scroll the window to follow the focal point. */
   exclusion = tilemap_inside_window_deadzone_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point right of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x++;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point left of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x--;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   exclusion = tilemap_inside_window_deadzone_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point below window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y++;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point above window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y--;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   /* Clamp the window to the edge of the map. */
   exclusion = tilemap_inside_inner_map_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point too close to map left edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x = t->width - twindow->width;
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point too close to map right edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x = 0;
   }

   exclusion = tilemap_inside_inner_map_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point too close to map bottom edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y = t->height - twindow->height;
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Focal point too close to map top edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y = 0;
   }

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
}

void tilemap_add_dirty_tile(
   struct TILEMAP* t, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
) {
   struct TILEMAP_POSITION* pos = NULL;

   pos =
      (struct TILEMAP_POSITION*)calloc( 1, sizeof( struct TILEMAP_POSITION ) );
   scaffold_check_null( pos );

   pos->x = x;
   pos->y = y;

   vector_add( &(t->dirty_tiles), pos );

cleanup:
   return;
}

void tilemap_set_redraw_state( struct TILEMAP* t, TILEMAP_REDRAW_STATE st ) {
   t->redraw_state = st;

   if( TILEMAP_REDRAW_ALL == st ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( "Initiating full tilemap redraw...\n" );
#endif /* DEBUG_TILES_VERBOSE */
   }

   /* Always reset dirty tiles. */
   vector_remove_cb( &(t->dirty_tiles), callback_free_generic, NULL );
   scaffold_assert( 0 == vector_count( &(t->dirty_tiles) ) );
}
