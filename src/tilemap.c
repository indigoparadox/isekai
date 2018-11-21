
#define TILEMAP_C
#include "tilemap.h"

#include "callback.h"
#include "mobile.h"
#include "channel.h"
#include "twindow.h"

struct TILEMAP_TILESET {
   struct REF refcount;
   GFX_COORD_PIXEL tileheight;  /*!< Height of tiles in pixels. */
   GFX_COORD_PIXEL tilewidth;   /*!< Width of tiles in pixels. */
   struct HASHMAP* images;     /*!< Graphics indexed by filename. */
   struct VECTOR* terrain;     /*!< Terrains in file order. */
   struct VECTOR* tiles;       /*!< Tile data in file order. */
   bstring def_path;
};

static VBOOL tilemap_layer_free_cb(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   tilemap_layer_free( layer );
   return VTRUE;
}

static VBOOL tilemap_tileset_free_cb(
   size_t idx, void* iter, void* arg
) {
   tilemap_tileset_free( (struct TILEMAP_TILESET*)iter );
   return VTRUE;
}

static void tilemap_cleanup( const struct REF* ref ) {
   struct TILEMAP* t;
   struct CHANNEL* l;

   t = scaffold_container_of( ref, struct TILEMAP, refcount );
   l = tilemap_get_channel( t );

   lg_debug(
      __FILE__, "Destroying tilemap for channel: %b\n", l->name
   );

   vector_remove_cb( t->layers, tilemap_layer_free_cb, NULL );
   vector_free( &(t->layers) );
#ifdef USE_ITEMS
   vector_remove_cb( &(t->item_caches), callback_free_item_caches, NULL );
   vector_cleanup( &(t->item_caches) );
#endif // USE_ITEMS
   vector_remove_cb( t->spawners, callback_free_spawners, NULL );
   vector_free( &(t->spawners) );
   vector_remove_cb( t->tilesets, tilemap_tileset_free_cb, NULL );
   vector_free( &(t->tilesets) );

   /* TODO: Free tilemap. */
}

void tilemap_init(
   struct TILEMAP* t, VBOOL local_images, struct CLIENT* server,
   struct CHANNEL* l
) {
   ref_init( &(t->refcount), tilemap_cleanup );

   t->layers = vector_new();
   t->item_caches = vector_new();
   t->tilesets = vector_new();
   t->spawners = vector_new();

   t->dirty_tiles = vector_new();

   tilemap_set_redraw_state( t, TILEMAP_REDRAW_DIRTY );

   t->orientation = TILEMAP_ORIENTATION_ORTHO;
   t->lname = bfromcstr( "" );
   t->channel = l;
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
   ts->active = VTRUE;
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

#ifdef USE_ITEMS

void tilemap_item_cache_init(
   struct TILEMAP_ITEM_CACHE* cache,
   struct TILEMAP* t,
   TILEMAP_COORD_TILE x,
   TILEMAP_COORD_TILE y
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

#endif // USE_ITEMS

void tilemap_layer_init( struct TILEMAP_LAYER* layer, size_t tiles_length ) {
   //vector_init( &(layer->tiles) );
   layer->tile_gids = mem_alloc( tiles_length, TILEMAP_TILE );
   layer->tile_gids_len = tiles_length;
}

void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer ) {
   //vector_cleanup( &(layer->tiles) );
   if( NULL != layer->tile_gids ) {
      mem_free( layer->tile_gids );
   }
   bdestroy( layer->name );
}

struct TILEMAP_TILESET* tilemap_tileset_new( bstring def_path ) {
   struct TILEMAP_TILESET* set = NULL;
   set = mem_alloc( 1, struct TILEMAP_TILESET );
   lgc_null( set );
   tilemap_tileset_init( set, def_path );
cleanup:
   return set;
}

struct TILEMAP_TILE_DATA* tilemap_tileset_get_tile(
   const struct TILEMAP_TILESET* set, int gid
) {
   scaffold_assert( NULL != set );
   scaffold_assert( vector_is_valid( set->tiles ) );
   return vector_get( set->tiles, gid );
}

size_t tilemap_tileset_set_tile(
   struct TILEMAP_TILESET* set, int gid, struct TILEMAP_TILE_DATA* tile_info
) {
   scaffold_assert( NULL != set );
   scaffold_assert( vector_is_valid( set->tiles ) );
   return vector_set( set->tiles, gid, tile_info, VTRUE );
}

GFX_COORD_PIXEL tilemap_tileset_get_tile_width(
   const struct TILEMAP_TILESET* set
) {
   scaffold_assert( NULL != set );
   return set->tilewidth;
}

GFX_COORD_PIXEL tilemap_tileset_get_tile_height(
   const struct TILEMAP_TILESET* set
) {
   scaffold_assert( NULL != set );
   return set->tileheight;
}

void tilemap_tileset_set_tile_width(
   struct TILEMAP_TILESET* set, GFX_COORD_PIXEL width
) {
   scaffold_assert( NULL != set );
   set->tilewidth = width;
}

void tilemap_tileset_set_tile_height(
   struct TILEMAP_TILESET* set, GFX_COORD_PIXEL height
) {
   scaffold_assert( NULL != set );
   set->tileheight = height;
}

VBOOL tilemap_tileset_has_image(
   const struct TILEMAP_TILESET* set, bstring filename
) {
   if( NULL == set ) {
      return VFALSE;
   }
   if( hashmap_iterate( set->images, callback_search_graphics, filename ) ) {
      return VTRUE;
   }
   return VFALSE;
}

VBOOL tilemap_tileset_set_image(
   struct TILEMAP_TILESET* set, bstring filename, struct GRAPHICS* g
) {
   scaffold_assert( NULL != set );
   scaffold_assert( hashmap_is_valid( set->images ) );
   if( hashmap_put( set->images, filename, g, VFALSE ) ) {
      return VTRUE;
   }
   return VFALSE;
}

static void* cb_tilemap_tileset_img_get_or_dl( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;

   if(
      NULL == iter
#ifdef USE_CHUNKS
      && NULL == client_get_chunker( c, idx )
#endif /* USE_CHUNKS */
   ) {
      client_request_file_later( c, DATAFILE_TYPE_TILESET_TILES, idx );
   } else if( NULL != iter ) {
      return iter;
   }
   return NULL;
}

struct GRAPHICS* tilemap_tileset_get_image_default(
   const struct TILEMAP_TILESET* set, struct CLIENT* c
) {
   scaffold_assert( NULL != set );
   //return (GRAPHICS*)hashmap_get_first( &(set->images) );
   return hashmap_iterate(
      set->images, cb_tilemap_tileset_img_get_or_dl, c );
}

VBOOL tilemap_tileset_add_terrain(
   struct TILEMAP_TILESET* set, struct TILEMAP_TERRAIN_DATA* terrain_info
) {
   scaffold_assert( NULL != set );
   scaffold_assert( vector_is_valid( set->terrain ) );
   if( 0 > vector_add( set->terrain, terrain_info ) ) {
      return VFALSE;
   }
   return VTRUE;
}

struct TILEMAP_TERRAIN_DATA* tilemap_tileset_get_terrain(
   struct TILEMAP_TILESET* set, size_t gid
) {
   scaffold_assert( NULL != set );
   scaffold_assert( vector_is_valid( set->terrain ) );
   return vector_get( set->terrain, gid );
}

bstring tilemap_tileset_get_definition_path(
   const struct TILEMAP_TILESET* set
) {
   scaffold_assert( NULL != set );
   return set->def_path;
}

void tilemap_tileset_cleanup( struct TILEMAP_TILESET* set ) {
#ifdef ENABLE_LOCAL_CLIENT
   hashmap_remove_cb( set->images, callback_free_graphics, NULL );
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

   set->images = hashmap_new();

   set->terrain = vector_new();
   set->tiles = vector_new();
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
   const struct TILEMAP* t, SCAFFOLD_SIZE gid, SCAFFOLD_SIZE* set_firstgid
) {
   struct TILEMAP_TILESET* set = NULL;
   /* The gid variable does double-duty, here. It provides a GID to search for,
    * and a place to keep the first GID of the found tileset for this tilemap
    * when the tileset is found.
    */
   set = vector_iterate( t->tilesets, callback_search_tilesets_gid, &gid );
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
   TILEMAP_COORD_TILE tiles_wide = 0;

   lgc_null( g_set );

   tiles_wide = g_set->w / set->tilewidth;

   gid -= set_firstgid;

   tile_screen_rect->y = (gid / tiles_wide) * set->tileheight;
   tile_screen_rect->x = (gid % tiles_wide) * set->tilewidth;

   /* scaffold_assert( *y < (set->tileheight * tiles_high) );
   scaffold_assert( *x < (set->tilewidth * tiles_wide) ); */
cleanup:
   return;
}

#endif /* USE_CURSES */

#if 0
/** \brief Get the GID of the tile at the given position on the given layer.
 * \param[in] layer  Layer from which to fetch the tile.
 * \param[in] x      X coordinate of the tile to fetch.
 * \param[in] y      Y coordinate of the tile to fetch.
 * \return A GID that can be used to find the tile's image or terrain info.
 */
SCAFFOLD_INLINE TILEMAP_TILE tilemap_get_tile(
   const struct TILEMAP_LAYER* layer, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
) {
   SCAFFOLD_SIZE index = (y * layer->width) + x;
   if( vector_count( &(layer->tiles) ) <= index ) {
      return -1;
   }
   // XXX: Invalid tiles showing up later.
   return vector_get_scalar( &(layer->tiles), index );
}
#endif // 0

TILEMAP_TILE tilemap_layer_get_tile_gid(
   const struct TILEMAP_LAYER* layer, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
) {
   SCAFFOLD_SIZE index = (y * layer->width) + x;
   TILEMAP_TILE gid_out = 0;
   scaffold_assert( layer->tile_gids_len > index );
   gid_out = layer->tile_gids[index];
   scaffold_assert( gid_out >= 0 );
   return layer->tile_gids[index] - 1;
}

void tilemap_layer_set_tile_gid(
   struct TILEMAP_LAYER* layer, size_t index, TILEMAP_TILE gid
) {
   scaffold_assert( layer->tile_gids_len > index );
   layer->tile_gids[index] = gid;
}

TILEMAP_ORIENTATION tilemap_get_orientation( struct TILEMAP* t ) {
   if( NULL == t ) {
      return TILEMAP_ORIENTATION_UNDEFINED;
   }
   return t->orientation;
}

SCAFFOLD_INLINE GFX_COORD_PIXEL tilemap_get_tile_width( struct TILEMAP* t ) {
   struct TILEMAP_TILESET* set = NULL;
   SCAFFOLD_SIZE set_firstgid = 0;

   set = tilemap_get_tileset( t, 1, &set_firstgid );

   if( NULL != set ) {
      return set->tilewidth;
   } else {
      return 0;
   }
}

SCAFFOLD_INLINE GFX_COORD_PIXEL tilemap_get_tile_height( struct TILEMAP* t ) {
   struct TILEMAP_TILESET* set = NULL;
   SCAFFOLD_SIZE set_firstgid = 0;

   set = tilemap_get_tileset( t, 1, &set_firstgid );

   if( NULL != set ) {
      return set->tileheight;
   } else {
      return 0;
   }
}

void tilemap_set_redraw_state( struct TILEMAP* t, TILEMAP_REDRAW_STATE st ) {
   t->redraw_state = st;

#ifdef ENABLE_LOCAL_CLIENT
   if( TILEMAP_REDRAW_ALL == st ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug( __FILE__, "Initiating full tilemap redraw...\n" );
#endif /* DEBUG_TILES_VERBOSE */
   }

   /* Always reset dirty tiles. */
   vector_remove_cb( t->dirty_tiles, callback_free_generic, NULL );
   scaffold_assert( 0 == vector_count( t->dirty_tiles ) );
#endif /* ENABLE_LOCAL_CLIENT */
}

#ifdef ENABLE_LOCAL_CLIENT

/** \brief Determine if the following coordinate is inside the area that can be
 *         displayed by the tilemap window without going off the edge.
 * \param x X coordinate in tiles.
 * \return
 */
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_x(
   TILEMAP_COORD_TILE x, struct TWINDOW* twindow
) {
   struct TILEMAP* t = NULL;

   t = twindow_get_tilemap_active( twindow );

   if( x < ((twindow_get_width( twindow ) / 2) - TILEMAP_DEAD_ZONE_X) ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( x >= t->width - ((twindow_get_width( twindow ) / 2) - TILEMAP_DEAD_ZONE_X) ) {
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
   TILEMAP_COORD_TILE y, struct TWINDOW* twindow
) {
   struct TILEMAP* t = NULL;

   t = twindow_get_tilemap_active( twindow );

   if(
      y < (twindow_get_height( twindow ) / 2) - TILEMAP_DEAD_ZONE_Y
   ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( y >= t->height - ((twindow_get_height( twindow ) / 2) - TILEMAP_DEAD_ZONE_Y) ) {
      return TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN;
   } else {
      return TILEMAP_EXCLUSION_INSIDE;
   }
}

SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_x(
   TILEMAP_COORD_TILE x, struct TWINDOW* twindow
) {
   TILEMAP_COORD_TILE twindow_middle_x = 0;

   twindow_middle_x = (twindow_get_x( twindow ) + (twindow_get_width( twindow ) / 2));

   if( x < twindow_middle_x - TILEMAP_DEAD_ZONE_X ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( x > twindow_middle_x + TILEMAP_DEAD_ZONE_X ) {
      return TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN;
   } else {
      return TILEMAP_EXCLUSION_INSIDE;
   }
}

SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_y(
   TILEMAP_COORD_TILE y, struct TWINDOW* twindow
) {
   TILEMAP_COORD_TILE twindow_middle_y = 0;

   twindow_middle_y = (twindow_get_y( twindow ) + (twindow_get_height( twindow ) / 2));

   if( y < twindow_middle_y - TILEMAP_DEAD_ZONE_Y ) {
      return TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP;
   } else if( y >twindow_middle_y + TILEMAP_DEAD_ZONE_Y ) {
      return TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN;
   } else {
      return TILEMAP_EXCLUSION_INSIDE;
   }
}

void tilemap_add_dirty_tile(
   struct TILEMAP* t, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
) {
   struct TILEMAP_POSITION* pos = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   pos = mem_alloc( 1, struct TILEMAP_POSITION );
   lgc_null( pos );

   pos->x = x;
   pos->y = y;

   verr = vector_add( t->dirty_tiles, pos );
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
      lg_debug( __FILE__, "Terrain Debug: Coords\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_COORDS:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_NAMES;
      lg_debug( __FILE__, "Terrain Debug: Terrain Names\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_NAMES:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_QUARTERS;
      lg_debug( __FILE__, "Terrain Debug: Terrain Quarters\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_QUARTERS:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_DEADZONE;
      lg_debug( __FILE__, "Terrain Debug: Window Deadzone\n" );
      break;
   case TILEMAP_DEBUG_TERRAIN_DEADZONE:
      tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_OFF;
      lg_debug( __FILE__, "Terrain Debug: Off\n" );
      break;
   }
}

#endif /* DEBUG_TILES */

void tilemap_tile_draw_ortho(
   const struct TILEMAP_LAYER* layer,
   TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y,
   GFX_COORD_PIXEL screen_x, GFX_COORD_PIXEL screen_y,
   struct TILEMAP_TILESET* set,
   const struct TWINDOW* twindow
) {
   GRAPHICS_RECT tile_tilesheet_pos;
   struct CLIENT* local_client = NULL;
   struct TILEMAP* t = NULL;
   GRAPHICS* g_tileset = NULL;
   SCAFFOLD_SIZE set_firstgid = 0;
   TILEMAP_TILE gid = 0;

   local_client = twindow_get_local_client( twindow );
   lgc_null( local_client );
   lgc_null( layer );
   t = layer->tilemap;
   lgc_null( t );

   gid = tilemap_layer_get_tile_gid( layer, x, y );

   lgc_zero_against(
      t->scaffold_error,
      tilemap_tileset_get_tile_width( set ), "Tile width is zero." );
   lgc_zero_against(
      t->scaffold_error,
      tilemap_tileset_get_tile_height( set ), "Tile height is zero." );
   if(
      0 == tilemap_tileset_get_tile_width( set ) ||
      0 == tilemap_tileset_get_tile_height( set )
   ) {
      goto cleanup;
   }

   if( 0 > screen_x || 0 > screen_y ) {
      goto cleanup; /* Silently. */
   }

   /* Figure out the graphical tile to draw from. */
   g_tileset = tilemap_tileset_get_image_default( set, local_client );
   if( NULL == g_tileset ) {
      /* TODO: Use a built-in placeholder tileset. */
      goto cleanup;
   }

   tilemap_get_tile_tileset_pos(
      set, set_firstgid, g_tileset, gid, &tile_tilesheet_pos );

   graphics_blit_partial(
      twindow_get_screen( twindow ),
      screen_x, screen_y,
      tile_tilesheet_pos.x, tile_tilesheet_pos.y,
      tilemap_tileset_get_tile_width( set ),
      tilemap_tileset_get_tile_height( set ),
      g_tileset
   );

cleanup:
   return;
}

#ifdef USE_ITEMS

struct TILEMAP_ITEM_CACHE* tilemap_drop_item(
   struct TILEMAP* t, struct ITEM* e, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
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
      lgc_null( cache );
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
   struct TILEMAP* t, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y, VBOOL force
) {
   struct TILEMAP_POSITION tile_map_pos;
   struct TILEMAP_ITEM_CACHE* cache_out = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   tile_map_pos.x = x;
   tile_map_pos.y = y;

   cache_out = vector_iterate(
      &(t->item_caches), callback_search_item_caches, &tile_map_pos
   );

   if( NULL == cache_out && VFALSE != force ) {
      tilemap_item_cache_new( cache_out, t, x, y );
      lgc_null( cache_out );
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

#endif // USE_ITEMS

struct CHANNEL* tilemap_get_channel( const struct TILEMAP* t ) {
   return t->channel;
}

#endif /* ENABLE_LOCAL_CLIENT */
