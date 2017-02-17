
#ifndef TILEMAP_H
#define TILEMAP_H

#include "bstrlib/bstrlib.h"
#include "vector.h"

/* All x/y/height/width dimensions for these structs are in terms of tiles. */

typedef struct _CLIENT CLIENT;

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
    char textrep;
} TILEMAP_TILE;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    VECTOR tiles;
} TILEMAP_LAYER;

typedef struct {
    uint32_t width;
    uint32_t height;
    VECTOR layers;
    VECTOR positions;
    uint32_t starting_x;
    uint32_t starting_y;
} TILEMAP;

/* y xxxxx
 * y xxxxx
 * y xxxxx
 *   xxx
 * (y * x) + x
 */

#define tilemap_get_tile( t, l, x, y ) \
    ((TILEMAP_TILE*)vector_get( \
        (TILEMAP_LATER*)vector_get( t->layers, l )->tiles, \
        (y * (TILEMAP_LATER*)vector_get( t->layers, l )->width) + (y % x), \
    ) \
)

void tilemap_init( TILEMAP* t );
void tilemap_iterate_screen_row(
    TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
    void (*callback)( TILEMAP* t, uint32_t x, uint32_t y, TILEMAP_TILE e )
);

#endif /* TILEMAP_H */
