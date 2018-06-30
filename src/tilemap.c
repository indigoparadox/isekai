
#define TILEMAP_C
#include "tilemap.h"

#include "callback.h"
#include "mobile.h"
#include "channel.h"

static BOOL tilemap_layer_free_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   tilemap_layer_free( layer );
   return TRUE;
}

static BOOL tilemap_tileset_free_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   tilemap_tileset_free( (struct TILEMAP_TILESET*)iter );
   return TRUE;
}

static void tilemap_cleanup( const struct REF* ref ) {
   struct TILEMAP* t;
   struct CHANNEL* l;

   t = scaffold_container_of( ref, struct TILEMAP, refcount );
   l = scaffold_container_of( t, struct CHANNEL, tilemap );

   scaffold_print_debug(
      &module, "Destroying tilemap for channel: %b\n", l->name
   );

   vector_remove_cb( &(t->layers), tilemap_layer_free_cb, NULL );
   vector_cleanup( &(t->layers) );
   vector_remove_cb( &(t->item_caches), callback_free_item_caches, NULL );
   vector_cleanup( &(t->item_caches) );
   vector_remove_cb( &(t->spawners), callback_free_spawners, NULL );
   vector_cleanup( &(t->spawners) );
   vector_remove_cb( &(t->tilesets), tilemap_tileset_free_cb, NULL );
   vector_cleanup( &(t->tilesets) );

   /* TODO: Free tilemap. */
}

void tilemap_init(
   struct TILEMAP* t, BOOL local_images, struct CLIENT* server
) {
   ref_init( &(t->refcount), tilemap_cleanup );

   vector_init( &(t->layers) );
   vector_init( &(t->item_caches) );
   vector_init( &(t->tilesets) );
   vector_init( &(t->spawners) );

   vector_init( &(t->dirty_tiles) );

   tilemap_set_redraw_state( t, TILEMAP_REDRAW_DIRTY );

   t->orientation = TILEMAP_ORIENTATION_ORTHO;
   t->lname = bfromcstr( "" );
}

void tilemap_free( struct TILEMAP* t ) {
   refcount_dec( t, "tilemap" );
}

void tilemap_spawner_init(
   struct TILEMAP_SPAWNER* ts, struct TILEMAP* t, TILEMAP_SPAWNER_TYPE type
) {
   scaffold_assert( NULL != t );
   scaffold_assert( NULL != ts );
   ts->tilemap = t;
   ts->last_spawned = 0;
   ts->countdown_remaining = 0;
   ts->active = TRUE;
   ts->pos.x = 0;
   ts->pos.y = 0;
   ts->type = type;
   ts->respawn_countdown = 0;
   ts->id = NULL;
}

void tilemap_spawner_free( struct TILEMAP_SPAWNER* ts ) {
   bdestroy( ts->id );
   mem_free( ts );
}

void tilemap_item_cache_init(
   struct TILEMAP_ITEM_CACHE* cache,
   struct TILEMAP* t,
   GFX_COORD_TILE x,
   GFX_COORD_TILE y
) {
   vector_init( &(cache->items) );
   cache->position.x = x;
   cache->position.y = y;
   cache->tilemap = t;
}

void tilemap_item_cache_free( struct TILEMAP_ITEM_CACHE* cache ) {
   vector_remove_cb( &(cache->items), callback_free_item_cache_items, NULL );
   vector_cleanup( &(cache->items) );
   mem_free( cache );
}

void tilemap_layer_init( struct TILEMAP_LAYER* layer ) {
   vector_init( &(layer->tiles) );
}

void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer ) {
   vector_cleanup( &(layer->tiles ) );
   bdestroy( layer->name );
}

void tilemap_tileset_cleanup( struct TILEMAP_TILESET* set ) {
#ifdef ENABLE_LOCAL_CLIENT
   hashmap_remove_cb( &(set->images), callback_free_graphics, NULL );
   bdestroy( set->def_path );
#endif /* ENABLE_LOCAL_CLIENT */
}

static void tilemap_tileset_free_final( const struct REF* ref ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)scaffold_container_of(
      ref, struct TILEMAP_TILESET, refcount );

   tilemap_tileset_cleanup( set );

   mem_free( set );
}

void tilemap_tileset_free( struct TILEMAP_TILESET* set ) {
   refcount_dec( set, "tileset" );
}

void tilemap_tileset_init( struct TILEMAP_TILESET* set, bstring def_path ) {
   ref_init( &(set->refcount), tilemap_tileset_free_final );
   /* We exist only to be in lists. */
   /* TODO: Right now, this seems somewhat useless because firstgid can be
    *       different for every tilemap this set belongs to.
    */
   set->refcount.count = 0;
   set->def_path = bstrcpy( def_path );

   hashmap_init( &(set->images) );

   vector_init( &(set->terrain) );
   vector_init( &(set->tiles) );
}

#ifndef USE_CURSES

/** \brief Given a tile GID, get the tileset it belongs to.
 * \param[in]  t            The tilemap on which the tile resides.
 * \param[in]  gid          The GID of the tile information to fetch.
 * \param[out] set_firstgid The first GID contained in this set for the given
 *                          tilemap.
 * \return The tileset containing the tile with the requested GID.
 */
SCAFFOLD_INLINE struct TILEMAP_TILESET* tilemap_get_tileset(
   struct TILEMAP* t, SCAFFOLD_SIZE gid, SCAFFOLD_SIZE* set_firstgid
) {
   struct TILEMAP_TILESET* set = NULL;
   /* The gid variable does double-duty, here. It provides a GID to search for,
    * and a place to keep the first GID of the found tileset for this tilemap
    * when the tileset is found.
    */
   set = vector_iterate( &(t->tilesets), callback_search_tilesets_gid, &gid );
   if( NULL != set && NULL != set_firstgid ) {
      *set_firstgid = gid;
   }
   return set;
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
   struct TILEMAP_TILESET* set, SCAFFOLD_SIZE set_firstgid, GRAPHICS* g_set,
   SCAFFOLD_SIZE gid, GRAPHICS_RECT* tile_screen_rect
) {
   GFX_COORD_TILE tiles_wide = 0;

   scaffold_check_null( g_set );

   tiles_wide = g_set->w / set->tilewidth;

   gid -= set_firstgid - 1;

   tile_screen_rect->y = ((gid - 1) / tiles_wide) * set->tileheight;
   tile_screen_rect->x = ((gid - 1) % tiles_wide) * set->tilewidth;

   /* scaffold_assert( *y < (set->tileheight * tiles_high) );
   scaffold_assert( *x < (set->tilewidth * tiles_wide) ); */
cleanup:
   return;
}

#endif /* USE_CURSES */

/** \brief Get the GID of the tile at the given position on the given layer.
 * \param[in] layer  Layer from which to fetch the tile.
 * \param[in] x      X coordinate of the tile to fetch.
 * \param[in] y      Y coordinate of the tile to fetch.
 * \return A GID that can be used to find the tile's image or terrain info.
 */
SCAFFOLD_INLINE uint32_t tilemap_get_tile(
   const struct TILEMAP_LAYER* layer, GFX_COORD_TILE x, GFX_COORD_TILE y
) {
   SCAFFOLD_SIZE index = (y * layer->width) + x;
   if( vector_count( &(layer->tiles) ) <= index ) {
      return -1;
   }
   return vector_get_scalar( &(layer->tiles), index );
}

void tilemap_set_redraw_state( struct TILEMAP* t, TILEMAP_REDRAW_STATE st ) {
   t->redraw_state = st;

#ifdef ENABLE_LOCAL_CLIENT
   if( TILEMAP_REDRAW_ALL == st ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug( &module, "Initiating full tilemap redraw...\n" );
#endif /* DEBUG_TILES_VERBOSE */
   }

   /* Always reset dirty tiles. */
   vector_remove_cb( &(t->dirty_tiles), callback_free_generic, NULL );
   scaffold_assert( 0 == vector_count( &(t->dirty_tiles) ) );
#endif /* ENABLE_LOCAL_CLIENT */
}

#ifdef ENABLE_LOCAL_CLIENT

#ifdef DEBUG_TILES

static void tilemap_layer_draw_tile_debug(
   struct TILEMAP_LAYER* layer, GRAPHICS* g, struct GRAPHICS_TILE_WINDOW* twin,
   SCAFFOLD_SIZE_SIGNED tile_x, SCAFFOLD_SIZE_SIGNED tile_y,
   SCAFFOLD_SIZE_SIGNED pix_x, SCAFFOLD_SIZE_SIGNED pix_y, uint32_t gid
) {
   struct TILEMAP_TILESET* set = NULL;
   bstring bnum = NULL;
   struct TILEMAP_TILE_DATA* tile_info = NULL;
   SCAFFOLD_SIZE td_i;
   int bstr_result;
   struct TILEMAP* t = layer->tilemap;
   SCAFFOLD_SIZE set_firstgid = 0;
   SCAFFOLD_SIZE layers_count;

   bnum = bfromcstralloc( 10, "" );
   scaffold_check_null( bnum );

   /* FIXME: How does gid resolve in a tileset that can have a variable firstgid? */
   set = tilemap_get_tileset( t, gid, &set_firstgid );
   scaffold_check_null( set );
   scaffold_check_zero_against(
      t->scaffold_error, set->tilewidth, "Tile width is zero." );
   scaffold_check_zero_against(
      t->scaffold_error, set->tileheight, "Tile height is zero." );

   layers_count = vector_count( &(t->layers) );
   if( layers_count <= tilemap_dt_layer ) {
      tilemap_dt_layer = 0;
   }

   if( layer->z != tilemap_dt_layer ) {
      /* Don't bother with the debug stuff for another layer. */
      goto cleanup;
   }

   tile_info = vector_get( &(set->tiles), gid - 1 );
   switch( tilemap_dt_state ) {
   case TILEMAP_DEBUG_TERRAIN_COORDS:
      if( layers_count - 1 == layer->z ) {
         bstr_result = bassignformat( bnum, "%d,", tile_x );
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 10, GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, FALSE
         );
         bstr_result = bassignformat( bnum, "%d", tile_y );
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 22, GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, FALSE
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
            GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, FALSE
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
         graphics_draw_text(
            g,
            pix_x + ((td_i % 2) * 12),
            pix_y + ((td_i / 2) * 16),
            GRAPHICS_TEXT_ALIGN_CENTER,
            td_i + 4, GRAPHICS_FONT_SIZE_8,
            bnum,
            FALSE
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_DEADZONE:
      if(
         !tilemap_inside_inner_map_x( tile_x, twin ) &&
         !tilemap_inside_inner_map_y( tile_y, twin )
      ) {
         graphics_draw_rect(
            g, pix_x, pix_y, 32, 32, GRAPHICS_COLOR_DARK_RED, TRUE
         );
      }
      if(
         !tilemap_inside_window_deadzone_x( tile_x, twin ) &&
         !tilemap_inside_window_deadzone_y( tile_y, twin )
      ) {
         graphics_draw_rect(
            g, pix_x, pix_y, 32, 32, GRAPHICS_COLOR_DARK_CYAN, TRUE
         );
      }
      break;
   }

cleanup:
   bdestroy( bnum );
   return;
}

#endif /* DEBUG_TILES */

static void* tilemap_layer_draw_tile_items_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   GRAPHICS_RECT* rect = (GRAPHICS_RECT*)arg;
   struct ITEM* e = (struct ITEM*)iter;
   struct GRAPHICS* g_screen = NULL;

   g_screen = client_get_screen( e->client_or_server );

   item_draw_ortho( e, rect->x, rect->y, g_screen );
}

static void tilemap_layer_draw_tile_ortho(
   struct TILEMAP_LAYER* layer, struct GRAPHICS_TILE_WINDOW* twindow,
   GFX_COORD_TILE x, GFX_COORD_TILE y, SCAFFOLD_SIZE gid
) {
   struct TILEMAP_TILESET* set = NULL;
   GRAPHICS_RECT tile_tilesheet_pos;
   GRAPHICS_RECT tile_screen_rect;
   struct CLIENT* local_client = twindow->local_client;
   struct TILEMAP* t = local_client->active_t;
   const struct MOBILE* o = local_client->puppet;
   GRAPHICS* g_tileset = NULL;
   SCAFFOLD_SIZE set_firstgid = 0;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   set = tilemap_get_tileset( t, gid, &set_firstgid );
   if( NULL == set ) {
      goto cleanup; /* Silently. */
   }

   scaffold_check_zero_against(
      t->scaffold_error, set->tilewidth, "Tile width is zero." );
   scaffold_check_zero_against(
      t->scaffold_error, set->tileheight, "Tile height is zero." );
   if( 0 == set->tilewidth || 0 == set->tileheight ) {
      goto cleanup;
   }

   /* Figure out the window position to draw to. */
   tile_screen_rect.x = set->tilewidth * (x - twindow->x);
   tile_screen_rect.y = set->tileheight * (y - twindow->y);

   if( 0 > tile_screen_rect.x || 0 > tile_screen_rect.y ) {
      goto cleanup; /* Silently. */
   }

   /* Figure out the graphical tile to draw from. */
   g_tileset = (GRAPHICS*)hashmap_get_first( &(set->images) );
   /* FIXME */
   /* If the current tileset doesn't exist, then load it. */
   g_tileset = hashmap_iterate( &(set->images), callback_search_tileset_img_gid, local_client );
   if( NULL == g_tileset ) {
      /* TODO: Use a built-in placeholder tileset. */
      goto cleanup;
   }

   tilemap_get_tile_tileset_pos(
      set, set_firstgid, g_tileset, gid, &tile_tilesheet_pos );

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
      tile_screen_rect.x += mobile_get_steps_remaining_x( o, TRUE );
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
      tile_screen_rect.y += mobile_get_steps_remaining_y( o, TRUE );
   }

   graphics_blit_partial(
      twindow->g,
      tile_screen_rect.x, tile_screen_rect.y,
      tile_tilesheet_pos.x, tile_tilesheet_pos.y,
      set->tilewidth, set->tileheight,
      g_tileset
   );

   cache = tilemap_get_item_cache( t, x, y, FALSE );
   if( NULL != cache ) {
      vector_iterate(
         &(cache->items), tilemap_layer_draw_tile_items_cb, &tile_screen_rect
      );
   }

#ifdef DEBUG_TILES
   tilemap_layer_draw_tile_debug(
      layer, twindow->g, twindow, x, y, tile_screen_rect.x, tile_screen_rect.y, gid
   );
#endif /* DEBUG_TILES */

cleanup:
   return;
}

#ifndef DISABLE_ISOMETRIC

static void tilemap_layer_draw_tile_iso(
   struct TILEMAP_LAYER* layer, struct GRAPHICS_TILE_WINDOW* twindow,
   GFX_COORD_TILE x, GFX_COORD_TILE y, SCAFFOLD_SIZE gid
) {

   /*const struct isomap_tile* tile,
   const OG_Rect* viewport,
   const GRAPHICS_ROTATE rotation
) {*/
#if 0
   int i = 0, screen_x, screen_y, tile_x, tile_y;
   OG_Texture* sprite_texture = NULL;
   struct isomap_render_texture texture_selection;

   tile_x = tile->x;
   tile_y = tile->y;

   /* Perform transformations. */

   isomap_render_select_terrain( tile, rotation, &texture_selection );

   graphics_isometric_tile_rotate(
      &tile_x, &tile_y,
      tile->map->width,
      tile->map->height,
      rotation
   );

   graphics_transform_isometric(
      tile_x,
      tile_y,
      &screen_x,
      &screen_y,
      viewport
   );

   sprite_texture = isomap_render_terrain_textures[texture_selection.texture_index];
   if( NULL == sprite_texture ) {
      /* TODO: Draw a placeholder. */
      goto cleanup;
   }

   /* Don't draw stuff off-screen. */
   if(
      -GRAPHICS_TILE_WIDTH > screen_x || -GRAPHICS_TILE_WIDTH > screen_y ||
      GRAPHICS_SCREEN_WIDTH < screen_x || GRAPHICS_SCREEN_HEIGHT < screen_y
   ) {
      goto cleanup;
   }

   graphics_draw_tile(
      sprite_texture,
      texture_selection.sprite_rect.x,
      texture_selection.sprite_rect.y,
      screen_x,
      screen_y
   );
#endif // 0
cleanup:

   return;
}

#endif /* !DISABLE_ISOMETRIC */

/** \brief Callback: Draw iterated layer.
 */
static void* tilemap_draw_layer_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   SCAFFOLD_SIZE_SIGNED
      x = 0,
      y = 0;
   uint32_t tile;
   struct VECTOR* tiles = NULL;

   tiles = &(layer->tiles);

   if( NULL == tiles || 0 == vector_count( tiles ) ) {
      goto cleanup;
   }

   /* TODO: Do culling in iso-friendly way. */
   for( x = twindow->min_x ; twindow->max_x > x ; x++ ) {
      for( y = twindow->min_y ; twindow->max_y > y ; y++ ) {
         tile = tilemap_get_tile( layer, x, y );
         if( 0 == tile ) {
            continue;
         }

         switch( layer->tilemap->orientation ) {
         case TILEMAP_ORIENTATION_ORTHO:
            tilemap_layer_draw_tile_ortho(
               layer, twindow, x, y, tile );
            break;

#ifndef DISABLE_ISOMETRIC
         case TILEMAP_ORIENTATION_ISO:
            tilemap_layer_draw_tile_iso(
               layer, twindow, x, y, tile );
            break;
#endif /* DISABLE_ISOMETRIC */
         }
      }
   }

cleanup:
   return NULL;
}

/** \brief Callback: Draw all of the layers for the iterated individual
 *         tile/position (kind of the opposite of tilemap_draw_layer_cb.)
 */
static void* tilemap_draw_pos_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP_LAYER* layer = NULL;
   struct TILEMAP* t =  twindow->local_client->active_t;
   int layer_idx = 0;
   int layer_max;
   uint32_t tile;

   vector_lock( &(t->layers), TRUE );
   layer_max = vector_count( &(t->layers) );
   for( layer_idx = 0 ; layer_max > layer_idx ; layer_idx++ ) {
      layer = vector_get( &(t->layers), layer_idx );
      tile = tilemap_get_tile( layer, pos->x, pos->y );
      if( 0 < tile ) {
         switch( t->orientation ) {
         case TILEMAP_ORIENTATION_ORTHO:
            tilemap_layer_draw_tile_ortho(
               layer, twindow, pos->x, pos->y, tile );
            break;

#ifndef DISABLE_ISOMETRIC
         case TILEMAP_ORIENTATION_ISO:
            tilemap_layer_draw_tile_iso(
               layer, twindow, pos->x, pos->y, tile );
            break;
#endif /* DISABLE_ISOMETRIC */
         }
      }
   }
   vector_lock( &(t->layers), FALSE );
}

void tilemap_draw_tilemap( struct GRAPHICS_TILE_WINDOW* twindow ) {
   struct CLIENT* local_client = twindow->local_client;
   struct MOBILE* o = local_client->puppet;
   struct TILEMAP* t = local_client->active_t;

   if( NULL == t ) {
      return;
   }

   /* Redraw all tiles if requested. */
   if(
      TILEMAP_REDRAW_ALL == t->redraw_state
#ifdef DEBUG_TILES
      || TILEMAP_DEBUG_TERRAIN_OFF != tilemap_dt_state
#endif /* DEBUG_TILES */
   ) {
      vector_iterate( &(t->layers), tilemap_draw_layer_cb, twindow );
   } else if(
      TILEMAP_REDRAW_DIRTY == t->redraw_state &&
      0 < vector_count( &(t->dirty_tiles ) )
   ) {
      /* Just redraw dirty tiles. */
      vector_iterate(
         &(t->dirty_tiles), tilemap_draw_pos_cb, twindow
      );
   }

   /* If we've done a full redraw as requested then switch back to just dirty *
    * tiles.                                                                  */
   if(
      0 != o->steps_remaining &&
      TILEMAP_REDRAW_ALL != t->redraw_state
   ) {
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if(
      0 == o->steps_remaining &&
      TILEMAP_REDRAW_DIRTY != t->redraw_state
   ) {
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_DIRTY );
   }

   /* Draw masks to cover up garbage from mismatch between viewport and window.
    */
   if( twindow->width < (GRAPHICS_SCREEN_WIDTH / GRAPHICS_SPRITE_WIDTH) ) {
      graphics_draw_rect(
         twindow->g,
         twindow->width * GRAPHICS_SPRITE_WIDTH,
         0,
         ((GRAPHICS_SCREEN_WIDTH / GRAPHICS_SPRITE_WIDTH) - twindow->width)
            * GRAPHICS_SPRITE_WIDTH,
         GRAPHICS_SCREEN_HEIGHT,
         GRAPHICS_COLOR_CHARCOAL,
         TRUE
      );
   }
   if( twindow->height < (GRAPHICS_SCREEN_HEIGHT / GRAPHICS_SPRITE_HEIGHT) ) {
      graphics_draw_rect(
         twindow->g,
         0,
         twindow->height * GRAPHICS_SPRITE_HEIGHT,
         GRAPHICS_SCREEN_WIDTH,
         ((GRAPHICS_SCREEN_HEIGHT / GRAPHICS_SPRITE_HEIGHT) - twindow->height)
            * GRAPHICS_SPRITE_HEIGHT,
         GRAPHICS_COLOR_CHARCOAL,
         TRUE
      );
   }
}

/** \brief Determine if the following coordinate is inside the area that can be
 *         displayed by the tilemap window without going off the edge.
 * \param x X coordinate in tiles.
 * \return
 */
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_x(
   GFX_COORD_TILE x, struct GRAPHICS_TILE_WINDOW* twindow
) {
   struct TILEMAP* t = twindow->local_client->active_t;
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
   GFX_COORD_TILE y, struct GRAPHICS_TILE_WINDOW* twindow
) {
   struct TILEMAP* t = twindow->local_client->active_t;
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
   GFX_COORD_TILE x, struct GRAPHICS_TILE_WINDOW* twindow
) {
   GFX_COORD_TILE twindow_middle_x = 0;

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
   GFX_COORD_TILE y, struct GRAPHICS_TILE_WINDOW* twindow
) {
   GFX_COORD_TILE twindow_middle_y = 0;

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
   GFX_COORD_TILE focal_x, GFX_COORD_TILE focal_y
) {
   GFX_COORD_TILE
      border_x = twindow->x == 0 ? 0 : TILEMAP_BORDER,
      border_y = twindow->y == 0 ? 0 : TILEMAP_BORDER;
   struct TILEMAP* t = twindow->local_client->active_t;
   TILEMAP_EXCLUSION exclusion;
   struct TILEMAP_TILESET* smallest_tileset = NULL;
   struct TILEMAP_POSITION temp = { 0 };

   if( NULL == t ) {
      return;
   }

   /* TODO: Request item caches for tiles scrolling into view. */

   /* Find the focal point if we're not centered on it. */
   if( focal_x < twindow->x || focal_x > twindow->x + twindow->width ) {
      twindow->x = focal_x - (twindow->width / 2);
   }
   if( focal_y < twindow->y || focal_y > twindow->y + twindow->height ) {
      twindow->y = focal_y - (twindow->height / 2);
   }

   smallest_tileset =
      vector_iterate( &(t->tilesets), callback_search_tilesets_small, &temp );
   if( NULL != smallest_tileset ) {
      twindow->grid_w = smallest_tileset->tilewidth;
      twindow->grid_h = smallest_tileset->tileheight;
   } else {
      twindow->grid_w = 0;
      twindow->grid_h = 0;
   }

   /* Scroll the window to follow the focal point. */
   exclusion = tilemap_inside_window_deadzone_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point right of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x++;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point left of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x--;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   exclusion = tilemap_inside_window_deadzone_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point below window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y++;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point above window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y--;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   /* Clamp the window to the edge of the map. */
   exclusion = tilemap_inside_inner_map_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map left edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x = t->width - twindow->width;
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map right edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x = 0;
   }

   exclusion = tilemap_inside_inner_map_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map bottom edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y = t->height - twindow->height;
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map top edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y = 0;
   }

   /* Only calculate these when window moves and store them. */
   twindow->max_x = twindow->x + twindow->width + border_x < t->width ?
      twindow->x + twindow->width + border_x : t->width;
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
   struct TILEMAP* t, GFX_COORD_TILE x, GFX_COORD_TILE y
) {
   struct TILEMAP_POSITION* pos = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   pos = mem_alloc( 1, struct TILEMAP_POSITION );
   scaffold_check_null( pos );

   pos->x = x;
   pos->y = y;

   verr = vector_add( &(t->dirty_tiles), pos );
   if( 0 > verr ) {
      mem_free( pos );
      goto cleanup;
   }

cleanup:
   return;
}

#ifdef DEBUG_TILES

void tilemap_toggle_debug_state() {
   switch( tilemap_dt_state ) {
   case TILEMAP_DEBUG_TERRAIN_OFF:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_COORDS;
      scaffold_print_debug( &module, "Terrain Debug: Coords\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_COORDS:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_NAMES;
      scaffold_print_debug( &module, "Terrain Debug: Terrain Names\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_NAMES:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_QUARTERS;
      scaffold_print_debug( &module, "Terrain Debug: Terrain Quarters\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_QUARTERS:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_DEADZONE;
      scaffold_print_debug( &module, "Terrain Debug: Window Deadzone\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_DEADZONE:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_OFF;
      scaffold_print_debug( &module, "Terrain Debug: Off\n" );
      break;
   }
}

#endif /* DEBUG_TILES */

struct TILEMAP_ITEM_CACHE* tilemap_drop_item(
   struct TILEMAP* t, struct ITEM* e, GFX_COORD_TILE x, GFX_COORD_TILE y
) {
   struct TILEMAP_ITEM_CACHE* cache = NULL;
   struct TILEMAP_POSITION pos;
   SCAFFOLD_SIZE_SIGNED verr = 0;

   pos.x = x;
   pos.y = y;

   cache =
      vector_iterate( &(t->item_caches), callback_search_item_caches, &pos );
   if( NULL == cache ) {
      tilemap_item_cache_new( cache, t, x, y );
      scaffold_check_null( cache );
      verr = vector_add( &(t->item_caches), cache );
      if( 0 > verr ) {
         tilemap_item_cache_free( cache );
         cache = NULL;
         goto cleanup;
      }
   }

   /* Prevent the item from being double-dropped. */
   if(
      NULL != vector_iterate( &(cache->items), callback_search_items, e )
   ) {
      goto cleanup;
   }

   verr = vector_add( &(cache->items), e );
   if( 0 > verr ) {
      item_free( e );
      goto cleanup;
   }

   tilemap_add_dirty_tile( t, x, y );

cleanup:
   return cache;
}

void tilemap_drop_item_in_cache( struct TILEMAP_ITEM_CACHE* cache, struct ITEM* e ) {
   tilemap_drop_item( cache->tilemap, e, cache->position.x, cache->position.y );
}

/** \brief
 * \param
 * \param
 * \return The cache for the given tile or a new, empty cache if none exists.
 */
struct TILEMAP_ITEM_CACHE* tilemap_get_item_cache(
   struct TILEMAP* t, GFX_COORD_TILE x, GFX_COORD_TILE y, BOOL force
) {
   struct TILEMAP_POSITION tile_map_pos;
   struct TILEMAP_ITEM_CACHE* cache_out = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   tile_map_pos.x = x;
   tile_map_pos.y = y;

   cache_out = vector_iterate(
      &(t->item_caches), callback_search_item_caches, &tile_map_pos
   );

   if( NULL == cache_out && FALSE != force ) {
      tilemap_item_cache_new( cache_out, t, x, y );
      scaffold_check_null( cache_out );
      verr = vector_add( &(t->item_caches), cache_out );
      if( 0 > verr ) {
         tilemap_item_cache_free( cache_out );
         cache_out = NULL;
         goto cleanup;
      }
   }

cleanup:
   return cache_out;
}

#endif /* ENABLE_LOCAL_CLIENT */
