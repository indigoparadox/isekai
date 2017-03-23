#ifndef TILEMAP_H
#define TILEMAP_H

#include "bstrlib/bstrlib.h"
#include "ezxml.h"
#include "vector.h"
#include "hashmap.h"
#include "graphics.h"

/* All x/y/height/width dimensions for these structs are in terms of tiles. */

struct CLIENT;
struct TILEMAP;

typedef enum TILEMAP_SPAWNER_TYPE {
   TILEMAP_SPAWNER_TYPE_MOBILE
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
   TILEMAP_ORIENTATION_ORTHO,
   TILEMAP_ORIENTATION_ISO
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

struct TILEMAP_TILESET {
   struct REF refcount;
   SCAFFOLD_SIZE tileheight;  /*!< Height of tiles in pixels. */
   SCAFFOLD_SIZE tilewidth;   /*!< Width of tiles in pixels. */
   struct HASHMAP images;     /*!< Graphics indexed by filename. */
   struct VECTOR terrain;     /*!< Terrains in file order. */
   struct VECTOR tiles;       /*!< Tile data in file order. */
   bstring def_path;
};

struct TILEMAP_POSITION {
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
};

struct TILEMAP_SPAWNER {
   struct TILEMAP_POSITION pos;
   struct TILEMAP* tilemap;
   bstring id;
   TILEMAP_SPAWNER_TYPE type;
   SERIAL last_spawned;
   SCAFFOLD_SIZE_SIGNED respawn_countdown;
   SCAFFOLD_SIZE_SIGNED countdown_remaining;
   BOOL active;
};

struct TILEMAP_LAYER {
   SCAFFOLD_SIZE x;        /*!< Layer left in tiles. */
   SCAFFOLD_SIZE y;        /*!< Layer top in tiles. */
   SCAFFOLD_SIZE z;
   SCAFFOLD_SIZE width;    /*!< Layer width in tiles. */
   SCAFFOLD_SIZE height;   /*!< Layer height in tiles. */
   struct VECTOR tiles;
   struct TILEMAP* tilemap;
   struct TILEMAP_LAYER* next_layer;
};

struct TILEMAP {
   struct REF refcount; /*!< Parent "class". */

   SCAFFOLD_SIZE width;
   SCAFFOLD_SIZE height;
   struct HASHMAP layers;
   struct VECTOR tilesets;
   struct HASHMAP* server_tilesets; /*!< All tilesets on server. */
   struct VECTOR spawners;
   TILEMAP_ORIENTATION orientation;
   SCAFFOLD_SIZE window_step_width;    /*!< For dungeons. */
   SCAFFOLD_SIZE window_step_height;   /*!< For dungeons. */
   bstring lname;
   struct VECTOR dirty_tiles; /*!< Stores TILEMAP_POSITIONS. */
   TILEMAP_REDRAW_STATE redraw_state;
   struct TILEMAP_LAYER* first_layer;
   SCAFFOLD_ERROR scaffold_error;
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

#define tilemap_tileset_new( set, def_path ) \
    set = scaffold_alloc( 1, struct TILEMAP_TILESET ); \
    scaffold_check_null( set ); \
    tilemap_tileset_init( set, def_path );

#define tilemap_spawner_new( ts, t, type ) \
    ts = scaffold_alloc( 1, struct TILEMAP_SPAWNER ); \
    scaffold_check_null( ts ); \
    tilemap_spawner_init( ts, t, type );

#define tilemap_new( t, local_images, server ) \
    t = (struct TILEMAP*)calloc( 1, sizeof( struct TILEMAP ) ); \
    scaffold_check_null( t ); \
    tilemap_init( t, local_images, server );

#define tilemap_layer_new( t ) \
    t = (struct TILEMAP_LAYER*)calloc( 1, sizeof( struct TILEMAP_LAYER ) ); \
    scaffold_check_null( t ); \
    tilemap_layer_init( t );

#define tilemap_position_new( t ) \
    t = (struct TILEMAP_POSITION*)calloc( 1, sizeof( struct TILEMAP_POSITION ) ); \
    scaffold_check_null( t ); \
    tilemap_position_init( t );

#define tilemap_layer_free( layer ) \
    tilemap_layer_cleanup( layer ); \
    scaffold_free( layer );

#define tilemap_position_free( position ) \
    tilemap_position_cleanup( position ); \
    scaffold_free( position );

void tilemap_init(
   struct TILEMAP* t, BOOL local_images, struct CLIENT* server
);
void tilemap_free( struct TILEMAP* t );
void tilemap_spawner_init(
   struct TILEMAP_SPAWNER* ts, struct TILEMAP* t, TILEMAP_SPAWNER_TYPE type
);
void tilemap_spawner_free( struct TILEMAP_SPAWNER* ts );
void tilemap_layer_init( struct TILEMAP_LAYER* layer );
void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer );
void tilemap_position_init( struct TILEMAP_POSITION* position );
void tilemap_position_cleanup( struct TILEMAP_POSITION* position );
void tilemap_tileset_cleanup( struct TILEMAP_TILESET* tileset );
void tilemap_tileset_free( struct TILEMAP_TILESET* tileset );
void tilemap_tileset_init( struct TILEMAP_TILESET* tileset, bstring def_path );
SCAFFOLD_INLINE struct TILEMAP_TILESET* tilemap_get_tileset(
   struct TILEMAP* t, SCAFFOLD_SIZE gid, SCAFFOLD_SIZE* set_firstgid
);
SCAFFOLD_INLINE void tilemap_get_tile_tileset_pos(
   struct TILEMAP_TILESET* set, SCAFFOLD_SIZE set_firstgid, GRAPHICS* g_set,
   SCAFFOLD_SIZE gid, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
);
SCAFFOLD_INLINE uint32_t tilemap_get_tile( struct TILEMAP_LAYER* layer, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y );
void tilemap_draw_ortho( struct GRAPHICS_TILE_WINDOW* window );
void tilemap_update_window_ortho(
   struct GRAPHICS_TILE_WINDOW* twindow,
   SCAFFOLD_SIZE focal_x, SCAFFOLD_SIZE focal_y
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_x(
   SCAFFOLD_SIZE x, struct GRAPHICS_TILE_WINDOW* twindow
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_inner_map_y(
   SCAFFOLD_SIZE y, struct GRAPHICS_TILE_WINDOW* twindow
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_x(
   SCAFFOLD_SIZE x, struct GRAPHICS_TILE_WINDOW* twindow
);
SCAFFOLD_INLINE TILEMAP_EXCLUSION tilemap_inside_window_deadzone_y(
   SCAFFOLD_SIZE y, struct GRAPHICS_TILE_WINDOW* twindow
);
void tilemap_add_dirty_tile(
   struct TILEMAP* t, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
);
void tilemap_set_redraw_state( struct TILEMAP* t, TILEMAP_REDRAW_STATE st );
void tilemap_toggle_debug_state();
void tilemap_add_tileset(
   struct TILEMAP* t, const bstring key, struct TILEMAP_TILESET* set
);

#ifdef TILEMAP_C

SCAFFOLD_MODULE( "tilemap.c" );

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
