#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#include "bstrlib/bstrlib.h"
#include "scaffold.h"
#include "ref.h"

#define GRAPHICS_RASTER_EXTENSION ".bmp"

typedef uint32_t gu;

typedef enum {
   GRAPHICS_TRANSIT_FX_NONE,
   GRAPHICS_TRANSIT_FX_FADEIN,
   GRAPHICS_TRANSIT_FX_FADEOUT
} GRAPHICS_TRANSIT_FX;

typedef struct {
   uint8_t r;
   uint8_t g;
   uint8_t b;
   uint8_t a;
} GRAPHICS_COLOR;

typedef struct _GRAPHICS {
   gu x;
   gu y;
   gu w;
   gu h;
   void* surface;
   void* palette;
   void* font;
   GRAPHICS_COLOR color;
   struct REF refcount;
} GRAPHICS;

typedef struct {
   gu x;
   gu y;
   gu w;
   gu h;
} GRAPHICS_RECT;

struct GRAPHICS_TILE_WINDOW {
   GRAPHICS* g;
   struct TILEMAP* t;
   SCAFFOLD_SIZE x; /* In tiles. */
   SCAFFOLD_SIZE y;
   SCAFFOLD_SIZE width;
   SCAFFOLD_SIZE height;
};

#define graphics_surface_new( g, x, y, w, h ) \
    g = (GRAPHICS*)calloc( 1, sizeof( GRAPHICS ) ); \
    scaffold_check_null( g ); \
    graphics_surface_init( g, x, y, w, h );

void graphics_screen_init( GRAPHICS* g, gu w, gu h );
void graphics_surface_init( GRAPHICS* g, gu x, gu y, gu w, gu h );
void graphics_surface_free( GRAPHICS* g );
void graphics_flip_screen( GRAPHICS* g );
void graphics_shutdown( GRAPHICS* g );
void graphics_set_font( GRAPHICS* g, bstring name );
void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR* color );
void graphics_set_color_ex( GRAPHICS* gr, uint8_t r, uint8_t g, uint8_t b, uint8_t a );
void graphics_set_image_path( GRAPHICS* g, const bstring path );
void graphics_set_image_data( GRAPHICS* g, const BYTE* data,
                              SCAFFOLD_SIZE length );
BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
void graphics_draw_text( GRAPHICS* g, gu x, gu y, const bstring text );
void graphics_draw_rect( GRAPHICS* g, gu x, gu y, gu w, gu h );
void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r, const bstring text );
void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx );
void graphics_scale( GRAPHICS* g, gu w, gu h );
void graphics_blit( GRAPHICS* g, gu x, gu y, gu s_w, gu s_h,
                    const GRAPHICS* src );
void graphics_blit_partial(
   GRAPHICS* g, gu x, gu y, gu s_x, gu s_y, gu s_w, gu s_h, const GRAPHICS* src
);
void graphics_sleep( uint16_t milliseconds );

#endif /* GRAPHICS_H */
