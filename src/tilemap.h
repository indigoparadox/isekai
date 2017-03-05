#ifndef TILEMAP_H
#define TILEMAP_H

#include "bstrlib/bstrlib.h"
#include "ezxml/ezxml.h"
#include "vector.h"
#include "hashmap.h"
#include "graphics.h"

/* All x/y/height/width dimensions for these structs are in terms of tiles. */

typedef struct _CLIENT CLIENT;
typedef struct _TILEMAP TILEMAP;

typedef enum {
   TILEMAP_ORIENTATION_ORTHO,
   TILEMAP_ORIENTATION_ISO,
} TILEMAP_ORIENTATION;

typedef struct {
   GRAPHICS* g;
   TILEMAP* t;
   size_t x; /* In tiles. */
   size_t y;
   size_t width;
   size_t height;
} TILEMAP_WINDOW;

typedef struct _TILEMAP_TERRAIN_DATA {
   bstring name;
   size_t tile;
} TILEMAP_TERRAIN_DATA;

typedef struct _TILEMAP_TILE_DATA {
   size_t id;
   size_t terrain[4];
} TILEMAP_TILE_DATA;

typedef struct _TILEMAP_TILESET {
   size_t firstgid;
   size_t tileheight;
   size_t tilewidth;
   HASHMAP images;
   HASHMAP terrain;
   VECTOR tiles;
} TILEMAP_TILESET;

typedef struct _TILEMAP_POSITION {
   size_t x_previous;
   size_t y_previous;
   size_t z_previous;
   size_t x;
   size_t y;
   size_t z;
   CLIENT* entity;
} TILEMAP_POSITION;

typedef struct _TILEMAP_LAYER {
   size_t x;
   size_t y;
   size_t width;
   size_t height;
   VECTOR tiles;
} TILEMAP_LAYER;

typedef struct _TILEMAP {
   REF refcount;
   size_t width;
   size_t height;
   HASHMAP layers;
   VECTOR positions;
   HASHMAP tilesets;
   size_t starting_x;
   size_t starting_y;
   TILEMAP_ORIENTATION orientation;
   bstring serialize_buffer;
   bstring serialize_filename;
   size_t window_step_width; /* For dungeons. */
   size_t window_step_height;
   BOOL local_images;
   uint16_t sentinal;

#ifdef EZXML_EMBEDDED_IMAGES
   /* Miscellaneous catch-all for generic memory blocks that must be freed on *
    * cleanup. Useful for datafile parsers.                                   */
   VECTOR freeable_chunks;
#endif /* EZXML_EMBEDDED_IMAGES */
} TILEMAP;

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
    t = (TILEMAP*)calloc( 1, sizeof( TILEMAP ) ); \
    scaffold_check_null( t ); \
    tilemap_init( t, local_images );

#define tilemap_layer_new( t ) \
    t = (TILEMAP_LAYER*)calloc( 1, sizeof( TILEMAP_LAYER ) ); \
    scaffold_check_null( t ); \
    tilemap_layer_init( t );

#define tilemap_position_new( t ) \
    t = (TILEMAP_POSITION*)calloc( 1, sizeof( TILEMAP_POSITION ) ); \
    scaffold_check_null( t ); \
    tilemap_position_init( t );

#define tilemap_layer_free( layer ) \
    tilemap_layer_cleanup( layer ); \
    free( layer );

#define tilemap_position_free( position ) \
    tilemap_position_cleanup( position ); \
    free( position );

/*
#define tilemap_tileset_free( tileset ) \
    tilemap_tileset_cleanup( tileset ); \
    free( tileset );
*/

void tilemap_init( TILEMAP* t, BOOL local_images );
void tilemap_free( TILEMAP* t );
void tilemap_layer_init( TILEMAP_LAYER* layer );
void tilemap_layer_cleanup( TILEMAP_LAYER* layer );
void tilemap_position_init( TILEMAP_POSITION* position );
void tilemap_position_cleanup( TILEMAP_POSITION* position );
void tilemap_tileset_cleanup( TILEMAP_TILESET* tileset );
void tilemap_tileset_free( TILEMAP_TILESET* tileset );
void tilemap_iterate_screen_row(
   TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( TILEMAP* t, uint32_t x, uint32_t y )
);
TILEMAP_TILESET* tilemap_get_tileset( TILEMAP* t, size_t gid );
inline void tilemap_get_tile_tileset_pos( TILEMAP_TILESET* set, size_t gid, size_t* x, size_t* y );
void tilemap_draw_ortho( TILEMAP* t, GRAPHICS* g, TILEMAP_WINDOW* window );

#endif /* TILEMAP_H */
