#ifndef TILEMAP_H
#define TILEMAP_H

typedef int TILEMAP_COORD_TILE;
typedef int TILEMAP_TILE;

#include <libvcol.h>
#include <goki.h>

#include "ref.h"
#include "ezxml.h"
#include "graphics.h"
#include "twindow.h"
#ifdef USE_ITEMS
#include "item.h"
#endif /* USE_ITEMS */

/* All x/y/height/width dimensions for these structs are in terms of tiles. */

struct CLIENT;
struct TILEMAP;
struct TWINDOW;

typedef enum TILEMAP_SPAWNER_TYPE {
   TILEMAP_SPAWNER_TYPE_MOBILE,
   TILEMAP_SPAWNER_TYPE_PLAYER,
   TILEMAP_SPAWNER_TYPE_ITEM
} TILEMAP_SPAWNER_TYPE;

#ifdef DEBUG_TILES
typedef enum TILEMAP_DEBUG_TERRAIN_STATE {
   TILEMAP_DEBUG_TERRAIN_OFF,
   TILEMAP_DEBUG_TERRAIN_COORDS,
   TILEMAP_DEBUG_TERRAIN_NAMES,
   TILEMAP_DEBUG_TERRAIN_QUARTERS,
   TILEMAP_DEBUG_TERRAIN_DEADZONE
} TILEMAP_DEBUG_TERRAIN_STATE;
#endif /* DEBUG_TILES */

typedef enum {
   TILEMAP_REDRAW_DIRTY,
   TILEMAP_REDRAW_ALL
} TILEMAP_REDRAW_STATE;

typedef enum {
   TILEMAP_ORIENTATION_UNDEFINED,
   TILEMAP_ORIENTATION_ORTHO,
#ifndef DISABLE_ISOMETRIC
   TILEMAP_ORIENTATION_ISO
#endif /* DISABLE_ISOMETRIC */
} TILEMAP_ORIENTATION;

typedef enum TILEMAP_MOVEMENT_MOD {
   TILEMAP_MOVEMENT_BLOCK = 0,
   TILEMAP_MOVEMENT_NORMAL = 1,
   TILEMAP_MOVEMENT_HALF = 2
} TILEMAP_MOVEMENT_MOD;

typedef enum TILEMAP_CUTOFF {
   TILEMAP_CUTOFF_NONE = 0,
   TILEMAP_CUTOFF_HALF = 2,
   TILEMAP_CUTOFF_2THIRDS = 3
} TILEMAP_CUTOFF;

typedef enum TILEMAP_EXCLUSION {
   TILEMAP_EXCLUSION_INSIDE,
   TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN,
   TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP
} TILEMAP_EXCLUSION;

struct TILEMAP_TERRAIN_DATA {
   bstring name;
   TILEMAP_MOVEMENT_MOD movement;   /*!< Movement modifier for this terrain. */
   TILEMAP_CUTOFF cutoff;
   SCAFFOLD_SIZE tile;
   SCAFFOLD_SIZE id;
};

struct TILEMAP_TILE_DATA {
   SCAFFOLD_SIZE id;
   struct TILEMAP_TERRAIN_DATA* terrain[4];
};

struct TILEMAP_POSITION {
   TILEMAP_COORD_TILE x;
   TILEMAP_COORD_TILE y;
   uint8_t facing;
};

#ifdef USE_ITEMS

struct TILEMAP_ITEM_CACHE {
   struct TILEMAP_POSITION position;
   struct VECTOR items;
   struct TILEMAP* tilemap;
};

#endif // USE_ITEMS

struct TILEMAP_SPAWNER {
   struct TILEMAP_POSITION pos;
   struct TILEMAP* tilemap;
   bstring id;
   bstring catalog;
   TILEMAP_SPAWNER_TYPE type;
   SERIAL last_spawned;
   SCAFFOLD_SIZE_SIGNED respawn_countdown;
   SCAFFOLD_SIZE_SIGNED countdown_remaining;
   VBOOL active;
};

struct TILEMAP_LAYER {
   TILEMAP_COORD_TILE x;        /*!< Layer left in tiles. */
   TILEMAP_COORD_TILE y;        /*!< Layer top in tiles. */
   TILEMAP_COORD_TILE z;
   TILEMAP_COORD_TILE width;    /*!< Layer width in tiles. */
   TILEMAP_COORD_TILE height;   /*!< Layer height in tiles. */
   //struct VECTOR tiles;
   TILEMAP_TILE* tile_gids;
   size_t tile_gids_len;
   struct TILEMAP* tilemap;
   bstring name;
};

struct TILEMAP {
   struct REF refcount; /*!< Parent "class". */
   TILEMAP_COORD_TILE width;
   TILEMAP_COORD_TILE height;
   struct VECTOR* item_caches;
   struct VECTOR* layers;
   struct VECTOR* tilesets;
   struct VECTOR* spawners;
   TILEMAP_ORIENTATION orientation;
   GFX_COORD_PIXEL window_step_width;    /*!< For dungeons. */
   GFX_COORD_PIXEL window_step_height;   /*!< For dungeons. */
   bstring lname;
   struct VECTOR* dirty_tiles; /*!< Stores TILEMAP_POSITIONS. */
   TILEMAP_REDRAW_STATE redraw_state;
   enum LGC_ERROR scaffold_error;
   struct CHANNEL* channel;
#ifdef DEBUG
   uint16_t sentinal;
#endif /* DEBUG */
};

#define TILEMAP_SERIALIZE_RESERVED (128 * 1024)
#define TILEMAP_SERIALIZE_CHUNKSIZE 80
#define TILEMAP_OBJECT_SPAWN_DIVISOR 32
#define TILEMAP_NAME_ALLOC 30
#define TILEMAP_BORDER 1
#define TILEMAP_DEAD_ZONE_X 8
#define TILEMAP_DEAD_ZONE_Y 5

#define TILEMAP_SENTINAL 1234

/* y xxxxx
 * y xxxxx
 * y xxxxx
 *   xxx
 * (y * x) + x
 */

#define tilemap_spawner_new( ts, t, type ) \
    ts = mem_alloc( 1, struct TILEMAP_SPAWNER ); \
    lgc_null( ts ); \
    tilemap_spawner_init( ts, t, type );

#define tilemap_new( t, local_images, server, channel ) \
    t = mem_alloc( 1, struct TILEMAP ); \
    lgc_null( t ); \
    tilemap_init( t, local_images, server, channel );

    /*
#define tilemap_layer_new( t ) \
    t = mem_alloc( 1, struct TILEMAP_LAYER ); \
    lgc_null( t ); \
    tilemap_layer_init( t );
*/

#define tilemap_item_cache_new( cache, t, x, y ) \
    cache = mem_alloc( 1, struct TILEMAP_ITEM_CACHE ); \
    lgc_null( cache ); \
    tilemap_item_cache_init( cache, t, x, y );

#define tilemap_position_new( t ) \
    t = mem_alloc( 1, struct TILEMAP_POSITION ); \
    lgc_null( t ); \
    tilemap_position_init( t );

#define tilemap_layer_free( layer ) \
    tilemap_layer_cleanup( layer ); \
    mem_free( layer );

#define tilemap_position_free( position ) \
    tilemap_position_cleanup( position ); \
    mem_free( position );

void tilemap_init(
   struct TILEMAP* t, VBOOL local_images, struct CLIENT* server,
   struct CHANNEL* l
);
void tilemap_free( struct TILEMAP* t );
void tilemap_spawner_init(
   struct TILEMAP_SPAWNER* ts, struct TILEMAP* t, TILEMAP_SPAWNER_TYPE type
);
void tilemap_spawner_free( struct TILEMAP_SPAWNER* ts );
#ifdef USE_ITEMS
void tilemap_item_cache_init(
   struct TILEMAP_ITEM_CACHE* cache,
   struct TILEMAP* t,
   TILEMAP_COORD_TILE x,
   TILEMAP_COORD_TILE y
);
void tilemap_item_cache_free( struct TILEMAP_ITEM_CACHE* cache );
#endif // USE_ITEMS
void tilemap_layer_init( struct TILEMAP_LAYER* layer, size_t tiles_length );
void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer );
void tilemap_position_init( struct TILEMAP_POSITION* position );
void tilemap_position_cleanup( struct TILEMAP_POSITION* position );
struct TILEMAP_TILESET* tilemap_tileset_new( bstring def_path );
struct TILEMAP_TILE_DATA* tilemap_tileset_get_tile(
   const struct TILEMAP_TILESET* set, int gid );
size_t tilemap_tileset_set_tile(
   struct TILEMAP_TILESET* set, int gid, struct TILEMAP_TILE_DATA* tile_info );
GFX_COORD_PIXEL tilemap_tileset_get_tile_width(
   const struct TILEMAP_TILESET* set );
GFX_COORD_PIXEL tilemap_tileset_get_tile_height(
   const struct TILEMAP_TILESET* set );
void tilemap_tileset_set_tile_width(
   struct TILEMAP_TILESET* set, GFX_COORD_PIXEL width );
void tilemap_tileset_set_tile_height(
   struct TILEMAP_TILESET* set, GFX_COORD_PIXEL height );
VBOOL tilemap_tileset_has_image(
   const struct TILEMAP_TILESET* set, bstring filename );
VBOOL tilemap_tileset_set_image(
   struct TILEMAP_TILESET* set, bstring filename, struct GRAPHICS* g );
VBOOL tilemap_tileset_add_terrain(
   struct TILEMAP_TILESET* set, struct TILEMAP_TERRAIN_DATA* terrain_info );
struct TILEMAP_TERRAIN_DATA* tilemap_tileset_get_terrain(
   struct TILEMAP_TILESET* set, size_t gid
);
struct GRAPHICS* tilemap_tileset_get_image_default(
   const struct TILEMAP_TILESET* set, struct CLIENT* c );
bstring tilemap_tileset_get_definition_path(
   const struct TILEMAP_TILESET* set );
void tilemap_tileset_cleanup( struct TILEMAP_TILESET* tileset );
void tilemap_tileset_free( struct TILEMAP_TILESET* tileset );
void tilemap_tileset_init( struct TILEMAP_TILESET* tileset, bstring def_path );
struct TILEMAP_TILESET* tilemap_get_tileset(
   const struct TILEMAP* t, size_t gid, size_t* set_firstgid
);
void tilemap_get_tile_tileset_pos(
   struct TILEMAP_TILESET* set, SCAFFOLD_SIZE set_firstgid, GRAPHICS* g_set,
   SCAFFOLD_SIZE gid, GRAPHICS_RECT* tile_screen_rect
);
TILEMAP_ORIENTATION tilemap_get_orientation( struct TILEMAP* t );
GFX_COORD_PIXEL tilemap_get_tile_width( struct TILEMAP* t );
GFX_COORD_PIXEL tilemap_get_tile_height( struct TILEMAP* t );
TILEMAP_TILE tilemap_layer_get_tile_gid(
   const struct TILEMAP_LAYER* layer, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
);
void tilemap_layer_set_tile_gid(
   struct TILEMAP_LAYER* layer, size_t index, TILEMAP_TILE gid
);
void tilemap_draw_tilemap( struct TWINDOW* window );
/* void tilemap_update_window_ortho(
   struct TWINDOW* twindow,
   TILEMAP_COORD_TILE focal_x, TILEMAP_COORD_TILE focal_y
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_x(
   TILEMAP_COORD_TILE x, struct TWINDOW* twindow
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_y(
   TILEMAP_COORD_TILE y, struct TWINDOW* twindow
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_x(
   TILEMAP_COORD_TILE x, struct TWINDOW* twindow
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_y(
   TILEMAP_COORD_TILE y, struct TWINDOW* twindow
);*/
void tilemap_add_dirty_tile(
   struct TILEMAP* t, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
);
void tilemap_tile_draw_ortho(
   const struct TILEMAP_LAYER* layer,
   TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y,
   GFX_COORD_PIXEL screen_x, GFX_COORD_PIXEL screen_y,
   struct TILEMAP_TILESET* set,
   const struct TWINDOW* twindow
);
void tilemap_set_redraw_state( struct TILEMAP* t, TILEMAP_REDRAW_STATE st );
void tilemap_toggle_debug_state();
void tilemap_add_tileset(
   struct TILEMAP* t, const bstring key, struct TILEMAP_TILESET* set
);
#ifdef USE_ITEMS
struct TILEMAP_ITEM_CACHE* tilemap_drop_item(
   struct TILEMAP* t, struct ITEM* e, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
);
void tilemap_drop_item_in_cache( struct TILEMAP_ITEM_CACHE* cache, struct ITEM* e );
struct TILEMAP_ITEM_CACHE* tilemap_get_item_cache(
   struct TILEMAP* t, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y, VBOOL force
);
#endif /* USE_ITEMS */
struct CHANNEL* tilemap_get_channel( const struct TILEMAP* t );
TILEMAP_EXCLUSION tilemap_inside_inner_map_x(
   TILEMAP_COORD_TILE x, struct TWINDOW* twindow );
TILEMAP_EXCLUSION tilemap_inside_inner_map_y(
   TILEMAP_COORD_TILE y, struct TWINDOW* twindow );
TILEMAP_EXCLUSION tilemap_inside_window_deadzone_x(
   TILEMAP_COORD_TILE x, struct TWINDOW* twindow );
TILEMAP_EXCLUSION tilemap_inside_window_deadzone_y(
   TILEMAP_COORD_TILE y, struct TWINDOW* twindow );
struct TWINDOW* tilemap_get_local_window( struct TILEMAP* t );

#ifdef TILEMAP_C

#ifdef DEBUG_TILES
volatile TILEMAP_DEBUG_TERRAIN_STATE tilemap_dt_state = TILEMAP_DEBUG_TERRAIN_OFF;
volatile uint8_t tilemap_dt_layer = 0;
#endif /* DEBUG_TILES */

#else

#ifdef DEBUG_TILES
extern volatile TILEMAP_DEBUG_TERRAIN_STATE tilemap_dt_state;
extern volatile uint8_t tilemap_dt_layer;
#endif /* DEBUG_TILES */

#endif /* TILEMAP_C */

#endif /* TILEMAP_H */
