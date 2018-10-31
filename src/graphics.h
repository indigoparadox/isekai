#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifndef DISABLE_MODE_POV
#include <math.h>
#endif /* !DISABLE_MODE_POV */

#include "scaffold.h"
#include "ref.h"

#define GRAPHICS_90DEG_RADS 1.5708

#define GRAPHICS_RASTER_EXTENSION ".bmp"
#define GRAPHICS_SCREEN_WIDTH 640
#define GRAPHICS_SCREEN_HEIGHT 480
#define GRAPHICS_RAY_FOV 0.66
#define GRAPHICS_RAY_FOV_FP 6600
#define GRAPHICS_RAY_ROTATE_INC (3 * GRAPHICS_90DEG_RADS)
#define GRAPHICS_RAY_ROTATE_INC_FP 47142
#define GRAPHICS_RAY_ROTATE_INC_FP_COS 18
#define GRAPHICS_RAY_ROTATE_INC_FP_SIN 9999
#define GRAPHICS_ISO_TILE_WIDTH 48
#define GRAPHICS_ISO_TILE_HEIGHT 48
#define GRAPHICS_ISO_TILE_OFFSET_X 24
#define GRAPHICS_ISO_TILE_OFFSET_Y 24
#define GRAPHICS_ISO_TILES_X_COUNT 4
#define GRAPHICS_ISO_TILES_Y_COUNT 4

#define GRAPHICS_FIXED_PRECISION 10000

#ifndef DISABLE_MODE_POV
#define GRAPHICS_RAY_INITIAL_STEP_X 1
#define GRAPHICS_RAY_INITIAL_STEP_Y 1
#endif /* !DISABLE_MODE_POV */

#include "colors.h"

COLOR_TABLE( GRAPHICS )

#ifndef DISABLE_MODE_POV
/* #define USE_HICOLOR */
#endif /* !DISABLE_MODE_POV */

#ifdef USE_HICOLOR
typedef uint32_t GRAPHICS_HICOLOR;
#endif /* USE_HICOLOR */

typedef long GFX_COORD_PIXEL;
typedef int64_t GFX_COORD_FPP;

typedef enum {
   GRAPHICS_ISO_ROTATE_90 = 90,
   GRAPHICS_ISO_ROTATE_180 = 180,
   GRAPHICS_ISO_ROTATE_270 = 270
} GRAPHICS_ISO_ROTATE;

typedef enum {
   RAY_SIDE_NORTH_SOUTH,
   RAY_SIDE_EAST_WEST
} GRAPHICS_RAY_SIDE;

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
   GFX_COORD_PIXEL fp_w;
   GFX_COORD_PIXEL fp_h;
   void* surface;
   void* palette;
   void* font;
   GFX_COORD_PIXEL virtual_x;
   GFX_COORD_PIXEL virtual_y;
   GFX_COORD_PIXEL half_h;
} GRAPHICS;

typedef struct {
   GFX_COORD_PIXEL x;
   GFX_COORD_PIXEL y;
} GRAPHICS_POINT;

typedef struct {
   double precise_x;
   double precise_y;
   double facing_x;
   double facing_y;
   uint8_t facing;
} GRAPHICS_PLANE;

typedef struct {
   GFX_COORD_PIXEL x;
   GFX_COORD_PIXEL y;
   GFX_COORD_PIXEL w;
   GFX_COORD_PIXEL h;
} GRAPHICS_RECT;

typedef struct {
   int map_x;
   int map_y;
   int map_w;
   int map_h;
   double perpen_dist;
   double stripe_x_hit;
   GRAPHICS_RAY_SIDE side;
   uint32_t data;
} GRAPHICS_DELTA;

typedef struct {
   double origin_x; /*!< Copied from camera pos at creation. */
   double origin_y; /*!< Copied from camera pos at creation. */
   double direction_x;
   double direction_y;
   /* Length of ray from one side to next x or y-side. */
   double delta_dist_x;
   double delta_dist_y;
   /* Length of ray to next x or y-side. */
   double side_dist_x;
   double side_dist_y;
   int step_x;
   int step_y;
   int map_x;
   int map_y;
   int map_w;
   int map_h;
} GRAPHICS_RAY;

typedef struct {
   double x;
   double y;
   int tex_x;
   int tex_y;
   int floor_height;
   int map_w;
   int map_h;
   int side;
   double weight;
   int line_height;
} GFX_RAY_FLOOR;

#define graphics_clear_screen( g, color ) \
   graphics_draw_rect( g, 0, 0, g->w, g->h, color, TRUE )

#define graphics_surface_new( g, x, y, w, h ) \
    g = mem_alloc( 1, GRAPHICS ); \
    lgc_null( g ); \
    graphics_surface_init( g, w, h );

#define graphics_precise( num ) \
   (num * (GRAPHICS_FIXED_PRECISION))

#define graphics_unprecise( num ) \
   (num / (GRAPHICS_FIXED_PRECISION))

void graphics_screen_new(
   GRAPHICS** g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh, int32_t arg1, void* arg2
);
void graphics_surface_init( GRAPHICS* g, GFX_COORD_PIXEL w, GFX_COORD_PIXEL h );
void graphics_surface_free( GRAPHICS* g );
void graphics_surface_cleanup( GRAPHICS* g );
void graphics_surface_set_w( GRAPHICS* g, GFX_COORD_PIXEL w );
void graphics_surface_set_h( GRAPHICS* g, GFX_COORD_PIXEL h );
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
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
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
struct VECTOR* graphics_get_line(
   GFX_COORD_PIXEL x1, GFX_COORD_PIXEL y1,
   GFX_COORD_PIXEL x2, GFX_COORD_PIXEL y2
);
void graphics_draw_char(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size, char c
);
void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx );
GRAPHICS* graphics_copy( GRAPHICS* g );
void graphics_blit(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   const GRAPHICS* src
);
void graphics_blit_partial(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y, GFX_COORD_PIXEL s_x,
   GFX_COORD_PIXEL s_y, GFX_COORD_PIXEL s_w, GFX_COORD_PIXEL s_h, const GRAPHICS* src
);
void graphics_blit_pixel(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL x_src, GFX_COORD_PIXEL y_src,
   const GRAPHICS* g_src
);
void graphics_blit_stretch(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL w, GFX_COORD_PIXEL h, const GRAPHICS* src
);
void graphics_sleep( uint16_t milliseconds );
uint32_t graphics_get_ticks();
void graphics_start_fps_timer();
int32_t graphics_sample_fps_timer();
void graphics_wait_for_fps_timer();

void graphics_free_bitmap( struct GRAPHICS_BITMAP* bitmap );
void graphics_bitmap_load(
   const BYTE* data, SCAFFOLD_SIZE_SIGNED data_sz, struct GRAPHICS_BITMAP** bitmap_out
);
SCAFFOLD_INLINE void graphics_get_spritesheet_pos_ortho(
   GRAPHICS* g_sprites, GRAPHICS_RECT* sprite_frame, SCAFFOLD_SIZE gid
);
void graphics_shrink_rect( GRAPHICS_RECT* rect, GFX_COORD_PIXEL shrink_by );
GRAPHICS_COLOR graphics_get_pixel(
   const GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y
);
void graphics_set_pixel(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y, GRAPHICS_COLOR pixel
);
GFX_COORD_FPP graphics_multiply_fp( GFX_COORD_FPP value_a, GFX_COORD_FPP value_b );
GFX_COORD_FPP graphics_divide_fp( GFX_COORD_FPP value_a, GFX_COORD_FPP value_b );

#ifdef USE_HICOLOR
GRAPHICS_HICOLOR graphics_get_hipixel(
   GRAPHICS* surface, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y
);
#endif /* USE_HICOLOR */

#ifndef DISABLE_MODE_POV

BOOL graphics_raycast_point_is_infinite( const GRAPHICS_DELTA* point );
void graphics_raycast_wall_create(
   GRAPHICS_RAY* ray, int x, int map_w, int map_h,
   const GRAPHICS_PLANE* plane_pos,
   const GRAPHICS_PLANE* cam_pos, const GRAPHICS* g
);
void graphics_raycast_wall_iterate(
   GRAPHICS_DELTA* wall_pos, GRAPHICS_RAY* ray, int prev_wall_height,
   const GRAPHICS* g
);
double graphics_raycast_get_distance(
   const GRAPHICS_DELTA* wall_pos, const GRAPHICS_PLANE* cam_pos, const GRAPHICS_RAY* ray
);
void graphics_floorcast_create(
   GFX_RAY_FLOOR* floor_pos, const GRAPHICS_RAY* ray, int x, const GRAPHICS_PLANE* cam_pos,
   const GRAPHICS_DELTA* wall_map_pos, const GRAPHICS* g
);
void graphics_floorcast_throw(
   GFX_RAY_FLOOR* floor_pos, int x, int y, int below_wall_height,
   const GRAPHICS_PLANE* cam_pos, const GRAPHICS_DELTA* wall_map_pos,
   const GRAPHICS_RAY* ray,
   GFX_COORD_PIXEL tile_width, GFX_COORD_PIXEL tile_height,
   const GRAPHICS* g
);
int graphics_get_ray_stripe_end( int line_height, const GRAPHICS* g );
int graphics_get_ray_stripe_start( int line_height, const GRAPHICS* g );

#endif /* !DISABLE_MODE_POV */

#ifndef DISABLE_ISOMETRIC

void graphics_isometric_tile_rotate(
   int* x, int* y, int width, int height, GRAPHICS_ISO_ROTATE rotation
);
void graphics_transform_isometric(
   GRAPHICS* g, float tile_x, float tile_y, int* screen_x, int* screen_y
);

#endif /* DISABLE_ISOMETRIC */

#ifdef GRAPHICS_C
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
