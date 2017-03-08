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

typedef enum {
   TILEMAP_ORIENTATION_ORTHO,
   TILEMAP_ORIENTATION_ISO
} TILEMAP_ORIENTATION;

struct TILEMAP_TERRAIN_DATA {
   bstring name;
   size_t tile;
};

struct TILEMAP_TILE_DATA {
   size_t id;
   size_t terrain[4];
};

struct TILEMAP_TILESET {
   size_t firstgid;
   size_t tileheight;
   size_t tilewidth;
   struct HASHMAP images;
   struct HASHMAP terrain;
   struct VECTOR tiles;
};

struct TILEMAP_POSITION {
   size_t x_previous;
   size_t y_previous;
   size_t z_previous;
   size_t x;
   size_t y;
   size_t z;
   struct CLIENT* entity;
};

struct TILEMAP_LAYER {
   size_t x;
   size_t y;
   size_t width;
   size_t height;
   struct VECTOR tiles;
};

struct TILEMAP {
   struct REF refcount;
   size_t width;
   size_t height;
   struct HASHMAP layers;
   struct VECTOR positions;
   struct HASHMAP tilesets;
   size_t starting_x;
   size_t starting_y;
   TILEMAP_ORIENTATION orientation;
   bstring serialize_buffer;
   bstring serialize_filename;
   size_t window_step_width; /* For dungeons. */
   size_t window_step_height;
   BOOL local_images;
   uint16_t sentinal;
};

#define TILEMAP_SERIALIZE_RESERVED (128 * 1024)
#define TILEMAP_SERIALIZE_CHUNKSIZE 80

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
struct TILEMAP_TILESET* tilemap_get_tileset( struct TILEMAP* t, size_t gid );
inline void tilemap_get_tile_tileset_pos(
   struct TILEMAP_TILESET* set, GRAPHICS* g_set, size_t gid, size_t* x, size_t* y
);
inline uint32_t tilemap_get_tile( struct TILEMAP_LAYER* layer, size_t x, size_t y );
void tilemap_draw_ortho( struct TILEMAP* t, GRAPHICS* g, struct GRAPHICS_TILE_WINDOW* window );

#endif /* TILEMAP_H */
