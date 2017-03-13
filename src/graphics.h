#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#include "bstrlib/bstrlib.h"
#include "scaffold.h"
#include "ref.h"

#define GRAPHICS_RASTER_EXTENSION ".bmp"
#define GRAPHICS_SCREEN_WIDTH 640
#define GRAPHICS_SCREEN_HEIGHT 480
#define GRAPHICS_SPRITE_WIDTH 32
#define GRAPHICS_SPRITE_HEIGHT 32

typedef enum GRAPHICS_TIMER {
   GRAPHICS_TIMER_FPS = 30
} GRAPHICS_TIMER;

typedef enum {
   GRAPHICS_TRANSIT_FX_NONE,
   GRAPHICS_TRANSIT_FX_FADEIN,
   GRAPHICS_TRANSIT_FX_FADEOUT
} GRAPHICS_TRANSIT_FX;

typedef enum GRAPHICS_COLOR {
   GRAPHICS_COLOR_TRANSPARENT,
   GRAPHICS_COLOR_DARK_BLUE,
   GRAPHICS_COLOR_DARK_GREEN,
   GRAPHICS_COLOR_DARK_CYAN,
   GRAPHICS_COLOR_DARK_RED,
   GRAPHICS_COLOR_BROWN,
   GRAPHICS_COLOR_GRAY,
   GRAPHICS_COLOR_DARK_GRAY,
   GRAPHICS_COLOR_BLUE,
   GRAPHICS_COLOR_GREEN,
   GRAPHICS_COLOR_CYAN,
   GRAPHICS_COLOR_RED,
   GRAPHICS_COLOR_MAGENTA,
   GRAPHICS_COLOR_YELLOW,
   GRAPHICS_COLOR_WHITE
} GRAPHICS_COLOR;

struct GRAPHICS_BITMAP {
   SCAFFOLD_SIZE w;
   SCAFFOLD_SIZE h;
   SCAFFOLD_SIZE pixels_sz;
   GRAPHICS_COLOR* pixels;
};

typedef struct _GRAPHICS {
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
   SCAFFOLD_SIZE w;
   SCAFFOLD_SIZE h;
   void* surface;
   void* palette;
   void* font;
   GRAPHICS_COLOR color;
   struct REF refcount;
} GRAPHICS;

typedef struct {
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
   SCAFFOLD_SIZE w;
   SCAFFOLD_SIZE h;
} GRAPHICS_RECT;

struct GRAPHICS_TILE_WINDOW {
   struct CLIENT* c;
   GRAPHICS* g;            /*!< Graphics element to draw on. */
   struct TILEMAP* t;
   SCAFFOLD_SIZE x;        /*!< Window left in tiles. */
   SCAFFOLD_SIZE y;        /*!< Window top in tiles. */
   SCAFFOLD_SIZE width;    /*!< Window width in tiles. */
   SCAFFOLD_SIZE height;   /*!< Window height in tiles. */
   SCAFFOLD_SIZE_SIGNED max_x;
   SCAFFOLD_SIZE_SIGNED max_y;
   SCAFFOLD_SIZE_SIGNED min_x;
   SCAFFOLD_SIZE_SIGNED min_y;
};

#define graphics_surface_new( g, x, y, w, h ) \
    g = (GRAPHICS*)calloc( 1, sizeof( GRAPHICS ) ); \
    scaffold_check_null( g ); \
    graphics_surface_init( g, x, y, w, h );

void graphics_screen_init( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h, int32_t arg1, void* arg2 );
void graphics_surface_init( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h );
void graphics_surface_free( GRAPHICS* g );
void graphics_flip_screen( GRAPHICS* g );
void graphics_shutdown( GRAPHICS* g );
void graphics_set_font( GRAPHICS* g, bstring name );
void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR color );
void graphics_set_color_ex( GRAPHICS* gr, uint8_t r, uint8_t g, uint8_t b, uint8_t a );
void graphics_set_image_path( GRAPHICS* g, const bstring path );
void graphics_set_image_data( GRAPHICS* g, const BYTE* data,
                              SCAFFOLD_SIZE length );
BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
void graphics_draw_text( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, const bstring text );
void graphics_draw_rect( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h );
void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r, const bstring text );
void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx );
void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h );
void graphics_blit( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h,
                    const GRAPHICS* src );
void graphics_blit_partial(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_x,
   SCAFFOLD_SIZE s_y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h, const GRAPHICS* src
);
void graphics_sleep( uint16_t milliseconds );
void graphics_colors_to_surface(
   GRAPHICS* g, GRAPHICS_COLOR* colors, SCAFFOLD_SIZE colors_sz
);
void graphics_wait_for_fps_timer();

void graphics_free_bitmap( struct GRAPHICS_BITMAP* bitmap );
void graphics_bitmap_load(
   const BYTE* data, SCAFFOLD_SIZE data_sz, struct GRAPHICS_BITMAP** bitmap_out
);

#endif /* GRAPHICS_H */
