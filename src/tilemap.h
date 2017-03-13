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

#ifdef DEBUG_TERRAIN
typedef enum TILEMAP_DEBUG_TERRAIN_STATE {
   TILEMAP_DEBUG_TERRAIN_NAMES,
   TILEMAP_DEBUG_TERRAIN_QUARTERS
} TILEMAP_DEBUG_TERRAIN_STATE;
#endif /* DEBUG_TERRAIN */

typedef enum {
   TILEMAP_ORIENTATION_ORTHO,
   TILEMAP_ORIENTATION_ISO
} TILEMAP_ORIENTATION;

typedef enum TILEMAP_MOVEMENT_MOD {
   TILEMAP_MOVEMENT_BLOCK = 0,
   TILEMAP_MOVEMENT_NORMAL = 1,
   TILEMAP_MOVEMENT_HALF = 2
} TILEMAP_MOVEMENT_MOD;

struct TILEMAP_TERRAIN_DATA {
   bstring name;
   TILEMAP_MOVEMENT_MOD movement;   /*!< Movement modifier for this terrain. */
   SCAFFOLD_SIZE tile;
   SCAFFOLD_SIZE id;
};

struct TILEMAP_TILE_DATA {
   SCAFFOLD_SIZE id;
   struct TILEMAP_TERRAIN_DATA* terrain[4];
};

struct TILEMAP_TILESET {
   SCAFFOLD_SIZE firstgid;
   SCAFFOLD_SIZE tileheight;  /*!< Height of tiles in pixels. */
   SCAFFOLD_SIZE tilewidth;   /*!< Width of tiles in pixels. */
   struct HASHMAP images;     /*!< Graphics indexed by filename. */
   struct VECTOR terrain;     /*!< Terrains in file order. */
   struct VECTOR tiles;       /*!< Tile data in file order. */
};

struct TILEMAP_POSITION {
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
};

struct TILEMAP_LAYER {
   SCAFFOLD_SIZE x;        /*!< Layer left in tiles. */
   SCAFFOLD_SIZE y;        /*!< Layer top in tiles. */
   SCAFFOLD_SIZE z;
   SCAFFOLD_SIZE width;    /*!< Layer width in tiles. */
   SCAFFOLD_SIZE height;   /*!< Layer height in tiles. */
   struct VECTOR tiles;
   struct TILEMAP* tilemap;
};

struct TILEMAP {
   struct REF refcount; /*!< Parent "class". */

   SCAFFOLD_SIZE width;
   SCAFFOLD_SIZE height;
   struct HASHMAP layers;
   /*
   struct VECTOR positions;
   */
   struct HASHMAP tilesets;
   struct HASHMAP player_spawns;
   SCAFFOLD_SIZE starting_x;
   SCAFFOLD_SIZE starting_y;
   TILEMAP_ORIENTATION orientation;
   SCAFFOLD_SIZE window_step_width;    /*!< For dungeons. */
   SCAFFOLD_SIZE window_step_height;   /*!< For dungeons. */
   bstring lname;
#ifdef DEBUG
   uint16_t sentinal;
#endif /* DEBUG */
};

#define TILEMAP_SERIALIZE_RESERVED (128 * 1024)
#define TILEMAP_SERIALIZE_CHUNKSIZE 80
#define TILEMAP_OBJECT_SPAWN_DIVISOR 32
#define TILEMAP_NAME_ALLOC 30

#define TILEMAP_SENTINAL 1234

/* y xxxxx
 * y xxxxx
 * y xxxxx
 *   xxx
 * (y * x) + x
 */

#define tilemap_new( t, local_images ) \
    t = (struct TILEMAP*)calloc( 1, sizeof( struct TILEMAP ) ); \
    scaffold_check_null( t ); \
    tilemap_init( t, local_images );

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
    free( layer );

#define tilemap_position_free( position ) \
    tilemap_position_cleanup( position ); \
    free( position );

void tilemap_init( struct TILEMAP* t, BOOL local_images );
void tilemap_free( struct TILEMAP* t );
void tilemap_layer_init( struct TILEMAP_LAYER* layer );
void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer );
void tilemap_position_init( struct TILEMAP_POSITION* position );
void tilemap_position_cleanup( struct TILEMAP_POSITION* position );
void tilemap_tileset_cleanup( struct TILEMAP_TILESET* tileset );
void tilemap_tileset_free( struct TILEMAP_TILESET* tileset );
void tilemap_iterate_screen_row(
   struct TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( struct TILEMAP* t, uint32_t x, uint32_t y )
);
SCAFFOLD_INLINE struct TILEMAP_TILESET* tilemap_get_tileset( struct TILEMAP* t, SCAFFOLD_SIZE gid );
SCAFFOLD_INLINE void tilemap_get_tile_tileset_pos(
   struct TILEMAP_TILESET* set, GRAPHICS* g_set, SCAFFOLD_SIZE gid, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
);
SCAFFOLD_INLINE uint32_t tilemap_get_tile( struct TILEMAP_LAYER* layer, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y );
void tilemap_draw_ortho( struct GRAPHICS_TILE_WINDOW* window );

#endif /* TILEMAP_H */
