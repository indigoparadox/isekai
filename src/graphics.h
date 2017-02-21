
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#include "bstrlib/bstrlib.h"

typedef uint32_t gu;

typedef enum {
    GRAPHICS_TRANSIT_FX_NONE,
    GRAPHICS_TRANSIT_FX_FADEIN,
    GRAPHICS_TRANSIT_FX_FADEOUT,
} GRAPHICS_TRANSIT_FX;

typedef struct _GRAPHICS {
    gu x;
    gu y;
    gu w;
    gu h;
    void* surface;
    void* font;
} GRAPHICS;

typedef enum {
    GRAPHICS_COLOR_BLACK,
    GRAPHICS_COLOR_WHITE,
} GRAPHICS_COLOR;

typedef struct {
    gu x;
    gu y;
    gu w;
    gu h;
} GRAPHICS_RECT;

#define graphics_surface_new( g, x, y, w, h ) \
    g = calloc( 1, sizeof( GRAPHICS ) ); \
    scaffold_check_null( g ); \
    graphics_surface_init( g, x, y, w, h );

void graphics_screen_init( GRAPHICS* g, gu w, gu h );
void graphics_surface_init( GRAPHICS* g, gu x, gu y, gu w, gu h );
void graphics_surface_cleanup( GRAPHICS* g );
void graphics_flip_screen( GRAPHICS* g );
void graphics_shutdown( GRAPHICS* g );
void graphics_set_font( GRAPHICS* g, bstring name );
void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR color );
void graphics_set_image_path( GRAPHICS* g, const bstring path );
void graphics_set_image_data( GRAPHICS* g, const uint8_t* data, uint32_t length );
void graphics_export_image_data( GRAPHICS* g, uint8_t* data, uint32_t* length );
void graphics_draw_text( GRAPHICS* g, gu x, gu y, const bstring text );
void graphics_draw_rect( GRAPHICS* g, gu x, gu y, gu w, gu h );
void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r, const bstring text );
void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx );
void graphics_scale( GRAPHICS* g, gu w, gu h );
void graphics_blit( GRAPHICS* g, gu x, gu y, gu s_w, gu s_h, const GRAPHICS* src );
void graphics_sleep( uint16_t milliseconds );

#endif /* GRAPHICS_H */
