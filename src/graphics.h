#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"
#include "ref.h"

#define GRAPHICS_RASTER_EXTENSION ".bmp"
#define GRAPHICS_SCREEN_WIDTH 640
#define GRAPHICS_SCREEN_HEIGHT 480
#define GRAPHICS_SPRITE_WIDTH 32
#define GRAPHICS_SPRITE_HEIGHT 32
#define GRAPHICS_VIRTUAL_SCREEN_WIDTH 768
#define GRAPHICS_VIRTUAL_SCREEN_HEIGHT 608

#include "colors.h"

COLOR_TABLE( GRAPHICS )

typedef int GFX_COORD_TILE;
typedef long GFX_COORD_PIXEL;

typedef enum GRAPHICS_TIMER {
   GRAPHICS_TIMER_FPS = 15
} GRAPHICS_TIMER;

typedef enum GRAPHICS_TEXT_ALIGN {
   GRAPHICS_TEXT_ALIGN_LEFT,
   GRAPHICS_TEXT_ALIGN_CENTER,
   GRAPHICS_TEXT_ALIGN_RIGHT
} GRAPHICS_TEXT_ALIGN;

typedef enum {
   GRAPHICS_TRANSIT_FX_NONE,
   GRAPHICS_TRANSIT_FX_FADEIN,
   GRAPHICS_TRANSIT_FX_FADEOUT
} GRAPHICS_TRANSIT_FX;

typedef enum GRAPHICS_FONT_SIZE {
   GRAPHICS_FONT_SIZE_8  = 8,
   GRAPHICS_FONT_SIZE_10 = 10,
   GRAPHICS_FONT_SIZE_12 = 12,
   GRAPHICS_FONT_SIZE_16 = 16,
   GRAPHICS_FONT_SIZE_32 = 32
} GRAPHICS_FONT_SIZE;

struct GRAPHICS_BITMAP {
   GFX_COORD_PIXEL w;
   GFX_COORD_PIXEL h;
   SCAFFOLD_SIZE pixels_sz;
   GRAPHICS_COLOR* pixels;
};

typedef struct GRAPHICS {
   GFX_COORD_PIXEL w;
   GFX_COORD_PIXEL h;
   void* surface;
   void* palette;
   void* font;
   GFX_COORD_PIXEL virtual_x;
   GFX_COORD_PIXEL virtual_y;
} GRAPHICS;

typedef struct {
   GFX_COORD_PIXEL x;
   GFX_COORD_PIXEL y;
   GFX_COORD_PIXEL w;
   GFX_COORD_PIXEL h;
} GRAPHICS_RECT;

struct GRAPHICS_TILE_WINDOW {
   struct CLIENT* local_client;
   GRAPHICS* g;            /*!< Graphics element to draw on. */
   struct TILEMAP* t;
   GFX_COORD_TILE x;        /*!< Window left in tiles. */
   GFX_COORD_TILE y;        /*!< Window top in tiles. */
   GFX_COORD_TILE width;    /*!< Window width in tiles. */
   GFX_COORD_TILE height;   /*!< Window height in tiles. */
   GFX_COORD_TILE max_x;
   GFX_COORD_TILE max_y;
   GFX_COORD_TILE min_x;
   GFX_COORD_TILE min_y;
   uint8_t grid_w;
   uint8_t grid_h;
};

#define graphics_clear_screen( g, color ) \
   graphics_draw_rect( g, 0, 0, g->w, g->h, color, TRUE )

#define graphics_surface_new( g, x, y, w, h ) \
    g = (GRAPHICS*)calloc( 1, sizeof( GRAPHICS ) ); \
    scaffold_check_null( g ); \
    graphics_surface_init( g, w, h );

void graphics_screen_new(
   GRAPHICS** g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh, int32_t arg1, void* arg2
);
void graphics_surface_init( GRAPHICS* g, GFX_COORD_PIXEL w, GFX_COORD_PIXEL h );
void graphics_surface_free( GRAPHICS* g );
void graphics_surface_cleanup( GRAPHICS* g );
void graphics_flip_screen( GRAPHICS* g );
void graphics_shutdown( GRAPHICS* g );
void graphics_set_window_title( GRAPHICS* g, bstring title, void* icon );
void graphics_screen_scroll(
   GRAPHICS* g, GFX_COORD_PIXEL offset_x, GFX_COORD_PIXEL offset_y
);
void graphics_set_image_path( GRAPHICS* g, const bstring path );
void graphics_set_image_data(
   GRAPHICS* g, const BYTE* data, SCAFFOLD_SIZE length
);
BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
void graphics_draw_text(
   GRAPHICS* g, SCAFFOLD_SIZE_SIGNED x_start, SCAFFOLD_SIZE_SIGNED y_start,
   GRAPHICS_TEXT_ALIGN align, GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size,
   const bstring text, BOOL cursor
);
void graphics_draw_rect(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL w, GFX_COORD_PIXEL h, GRAPHICS_COLOR color, BOOL filled
);
void graphics_draw_line(
   GRAPHICS* g, GFX_COORD_PIXEL x1, GFX_COORD_PIXEL y1,
   GFX_COORD_PIXEL x2, GFX_COORD_PIXEL y2, GRAPHICS_COLOR color
);
void graphics_draw_triangle(
   GRAPHICS* g,
   GFX_COORD_PIXEL x1, GFX_COORD_PIXEL y1,
   GFX_COORD_PIXEL x2, GFX_COORD_PIXEL y2,
   GFX_COORD_PIXEL x3, GFX_COORD_PIXEL y3,
   GRAPHICS_COLOR color, BOOL filled
);
void graphics_draw_circle(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL radius, GRAPHICS_COLOR color, BOOL filled
);
void graphics_measure_text(
   GRAPHICS* g, GRAPHICS_RECT* r, GRAPHICS_FONT_SIZE size, const bstring text
);
void graphics_draw_char(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size, char c
);
void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx );
void graphics_scale( GRAPHICS* g, GFX_COORD_PIXEL w, GFX_COORD_PIXEL h );
void graphics_blit(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   const GRAPHICS* src
);
void graphics_blit_partial(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y, GFX_COORD_PIXEL s_x,
   GFX_COORD_PIXEL s_y, GFX_COORD_PIXEL s_w, GFX_COORD_PIXEL s_h, const GRAPHICS* src
);
void graphics_sleep( uint16_t milliseconds );
uint32_t graphics_get_ticks();
void graphics_start_fps_timer();
int32_t graphics_sample_fps_timer();
void graphics_wait_for_fps_timer();

void graphics_free_bitmap( struct GRAPHICS_BITMAP* bitmap );
void graphics_bitmap_load(
   const BYTE* data, SCAFFOLD_SIZE data_sz, struct GRAPHICS_BITMAP** bitmap_out
);

#ifdef GRAPHICS_C
SCAFFOLD_MODULE( "graphics.c" );
void graphics_setup();
#ifdef GRAPHICS_SLOW_LINE

static SCAFFOLD_INLINE void graphics_draw_line_slow(
   GRAPHICS* g, SCAFFOLD_SIZE x1, SCAFFOLD_SIZE y1,
   SCAFFOLD_SIZE x2, SCAFFOLD_SIZE y2, GRAPHICS_COLOR color
) {

   /*
   if( x1 > x2 ) {
      x = x1;
      x1 = x2;

   }
   */

   /* FIXME */

   graphics_lock( g->surface );
   for( x = x1 ; x2 > x ; x++ ) {
     graphics_draw_pixel( g->surface, x, y, graphics_get_color( color ) );

   }
   graphics_unlock( g->surface );
}

#endif /* GRAPHICS_SLOW_LINE */
#endif /* GRAPHICS_C */

#endif /* GRAPHICS_H */
