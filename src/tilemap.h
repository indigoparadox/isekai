#ifndef TILEMAP_H
#define TILEMAP_H

#include "bstrlib/bstrlib.h"
#include "ezxml/ezxml.h"
#include "vector.h"
#include "graphics.h"

/* All x/y/height/width dimensions for these structs are in terms of tiles. */

typedef struct _CLIENT CLIENT;

typedef enum {
   TILEMAP_ORIENTATION_ORTHO,
   TILEMAP_ORIENTATION_ISO,
} TILEMAP_ORIENTATION;

typedef struct {
   GRAPHICS* image;
} TILEMAP_TILESET_IMAGE;

typedef struct {
   bstring name;
   int32_t tile;
} TILEMAP_TERRAIN_DATA;

typedef struct {
   int32_t id;
   int32_t terrain[4];
} TILEMAP_TILE_DATA;

typedef struct {
   int32_t firstgid;
   uint32_t tileheight;
   uint32_t tilewidth;
   VECTOR images;
   VECTOR terrain;
   VECTOR tiles;
} TILEMAP_TILESET;

typedef struct {
   uint32_t x_previous;
   uint32_t y_previous;
   uint32_t z_previous;
   uint32_t x;
   uint32_t y;
   uint32_t z;
   CLIENT* entity;
} TILEMAP_POSITION;

typedef struct {
   uint32_t x;
   uint32_t y;
   uint32_t width;
   uint32_t height;
   uint16_t* tiles;
   uint32_t tiles_count;
   uint32_t tiles_alloc;
} TILEMAP_LAYER;

typedef struct {
   uint32_t width;
   uint32_t height;
   VECTOR layers;
   VECTOR positions;
   VECTOR tilesets;
   uint32_t starting_x;
   uint32_t starting_y;
   TILEMAP_ORIENTATION orientation;
   ezxml_t xml_data;
   struct bstrList* serialize_buffer;
} TILEMAP;

#define TILEMAP_SERIALIZE_RESERVED (128 * 1024)
#define TILEMAP_SERIALIZE_CHUNKSIZE 80

/* y xxxxx
 * y xxxxx
 * y xxxxx
 *   xxx
 * (y * x) + x
 */

#define tilemap_get_tile( t, l, x, y ) \
    ((TILEMAP_TILE*)vector_get( \
        (TILEMAP_LAYER*)vector_get( t->layers, l )->tiles, \
        (y * (TILEMAP_LAYER*)vector_get( t->layers, l )->width) + (y % x), \
    ) \
)

#define tilemap_new( t ) \
    t = (TILEMAP*)calloc( 1, sizeof( TILEMAP ) ); \
    scaffold_check_null( t ); \
    tilemap_init( t );

#define tilemap_layer_new( t ) \
    t = (TILEMAP_LAYER*)calloc( 1, sizeof( TILEMAP_LAYER ) ); \
    scaffold_check_null( t ); \
    tilemap_layer_init( t );

#define tilemap_position_new( t ) \
    t = (TILEMAP_POSITION*)calloc( 1, sizeof( TILEMAP_POSITION ) ); \
    scaffold_check_null( t ); \
    tilemap_position_init( t );

#define tilemap_free( t ) \
    tilemap_cleanup( t ); \
    free( t );

#define tilemap_layer_free( layer ) \
    tilemap_layer_cleanup( layer ); \
    free( layer );

#define tilemap_position_free( position ) \
    tilemap_position_cleanup( position ); \
    free( position );

void tilemap_init( TILEMAP* t );
void tilemap_cleanup( TILEMAP* t );
void tilemap_layer_init( TILEMAP_LAYER* layer );
void tilemap_layer_cleanup( TILEMAP_LAYER* layer );
void tilemap_position_init( TILEMAP_POSITION* position );
void tilemap_position_cleanup( TILEMAP_POSITION* position );
void tilemap_iterate_screen_row(
   TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( TILEMAP* t, uint32_t x, uint32_t y )
);
void tilemap_serialize( TILEMAP* t, bstring buffer );
void tilemap_load_data( TILEMAP* t, const uint8_t* tmdata, int datasize );
void tilemap_load_file( TILEMAP* t, bstring filename );

#endif /* TILEMAP_H */
