
#define GRAPHICS_C
#include "graphics.h"

#ifdef USE_RAYCASTING


#endif /* USE_RAYCASTING */

#ifdef USE_CLOCK
#include <time.h>
#endif /* USE_CLOCK */

static uint32_t graphics_time = 0;
static uint32_t graphics_fps_delay = 0;

#pragma pack(push,1)
struct GRAPHICS_BITMAP_FILE_HEADER {
    int8_t type[2];
    int32_t size;
    int16_t reserved1;
    int16_t reserved2;
    int32_t offset;
};

struct GRAPHICS_BITMAP_HEADER {
    int32_t header_sz;
    int32_t width;
    int32_t height;
    int16_t planes;
    int16_t bpp;
    int32_t compression;
    int32_t image_sz;
    int32_t ppi_x;
    int32_t ppi_y;
    int32_t colors_used;
    int32_t colors_sig;
};
#pragma pack(pop)

void graphics_free_bitmap( struct GRAPHICS_BITMAP* bitmap ) {
   if( NULL != bitmap ) {
      if( NULL != bitmap->pixels ) {
         mem_free( bitmap->pixels );
      }
      mem_free( bitmap );
   }
}

void graphics_bitmap_load(
   const BYTE* data, SCAFFOLD_SIZE_SIGNED data_sz, struct GRAPHICS_BITMAP** bitmap_out
) {
#ifdef USE_BITMAP_DISCREET
   uint32_t* header_data_sz_ptr = (uint32_t*)&(data[2]);
   uint32_t* header_offset_ptr = (uint32_t*)&(data[10]);
#else
   struct GRAPHICS_BITMAP_FILE_HEADER* file_header = NULL;
   struct GRAPHICS_BITMAP_HEADER* header = NULL;
#endif /* USE_BITMAP_DISCREET */
   SCAFFOLD_SIZE_SIGNED pixels_sz = 0,
      i,
      output_i = 0;

   file_header = (struct GRAPHICS_BITMAP_FILE_HEADER*)&(data[0]);
   header = (struct GRAPHICS_BITMAP_HEADER*)&(data[14]);

   scaffold_print_debug(
      &module, "Bitmap: %d x %d\n", header->width, header->height
   );

   pixels_sz = ((header->width * header->height) / 2);

   scaffold_assert( 'B' == data[0] && 'M' == data[1] );
   scaffold_assert( file_header->size == data_sz );
   scaffold_assert( 4 == header->bpp );
   scaffold_assert( 0 == header->compression );
   scaffold_assert( (file_header->offset + pixels_sz) <= data_sz );

   *bitmap_out = mem_alloc( 1, struct GRAPHICS_BITMAP );
   scaffold_check_null( *bitmap_out );
   (*bitmap_out)->w = header->width;
   (*bitmap_out)->h = header->height;
   (*bitmap_out)->pixels_sz = (header->width * header->height);
   (*bitmap_out)->pixels =
      mem_alloc( (*bitmap_out)->pixels_sz, GRAPHICS_COLOR );
   scaffold_check_null( (*bitmap_out)->pixels );

   for( i = file_header->offset ; (file_header->offset + pixels_sz) > i ; i++ ) {
      (*bitmap_out)->pixels[output_i++] = (data[i] | 0x00) >> 4;
      (*bitmap_out)->pixels[output_i++] = (data[i] | 0xf0) & 0x0f;
   }

cleanup:
   return;
}

void graphics_setup() {
   graphics_fps_delay = 1000 / GRAPHICS_TIMER_FPS;
}

void graphics_start_fps_timer() {
   graphics_time = graphics_get_ticks();
}

int32_t graphics_sample_fps_timer() {
   int32_t ticks;
   ticks = graphics_get_ticks();
   return ticks - graphics_time;
}

void graphics_wait_for_fps_timer() {
#ifdef USE_POSIX_TIMER
   double elapsed;

   elapsed = ((double)(clock() - graphics_time)) / CLOCKS_PER_SEC;
   if( GRAPHICS_TIMER_FPS > elapsed ) {
      graphics_sleep( graphics_fps_delay - elapsed );
   }
#else
#ifdef DEBUG_FPS
   int bstr_ret;
#endif /* DEBUG_FPS */
   int32_t rest_time,
      difference;

   difference = graphics_sample_fps_timer();
   rest_time = graphics_fps_delay - difference;
   if( GRAPHICS_TIMER_FPS > difference ) {
      /* Subtract the time since graphics_Start_fps_timer() was last called
       * from the nominal delay required to maintain our FPS.
       */
      if( rest_time < 0 ) { goto cleanup; }
      graphics_sleep( rest_time );
   }
#endif /* USE_POSIX_TIMER */
cleanup:
   return;
}

void graphics_draw_text(
   GRAPHICS* g, SCAFFOLD_SIZE_SIGNED x_start, SCAFFOLD_SIZE_SIGNED y_start,
   GRAPHICS_TEXT_ALIGN align, GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size,
   const bstring text, BOOL cursor
) {
   SCAFFOLD_SIZE_SIGNED x = x_start,
      y = y_start;
   char c;
   int i;
   GRAPHICS_RECT text_size;

   scaffold_assert( NULL != g );
   scaffold_assert( NULL != text );

   switch( align ) {
   case GRAPHICS_TEXT_ALIGN_CENTER:
      graphics_measure_text( g, &text_size, size, text );
      x -= (text_size.w / 2);
      break;
   case GRAPHICS_TEXT_ALIGN_LEFT:
      break;
   case GRAPHICS_TEXT_ALIGN_RIGHT:
      /* TODO */
      break;
   }

   for( i = 0 ; text->slen > i ; i++ ) {
      c = text->data[i];
      graphics_draw_char( g, x + (size * i), y, color, size, c );
   }

   if( TRUE == cursor ) {
      graphics_draw_char( g, x + (size * i), y, color, size, '_' );
   }
}

void graphics_measure_text(
   GRAPHICS* g, GRAPHICS_RECT* r, GRAPHICS_FONT_SIZE size, const bstring text
) {
   scaffold_check_null( r );
   r->w = size;
   r->h = size;
   if( NULL != text ) {
      r->w *= blength( text );
   }
cleanup:
   return;
}

void graphics_surface_free( GRAPHICS* g ) {
   scaffold_check_null( g );
   graphics_surface_cleanup( g );
   scaffold_assert( NULL == g->palette );
   scaffold_assert( NULL == g->surface );
   mem_free( g );
cleanup:
   return;
}

void graphics_blit(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   const GRAPHICS* src
) {
   graphics_blit_partial( g, x, y, 0, 0, src->w, src->h, src );
}

GRAPHICS* graphics_copy( GRAPHICS* g ) {
   GRAPHICS* g_out = NULL;

   graphics_surface_new( g_out, g->virtual_x, g->virtual_y, g->w, g->h );

   graphics_blit( g_out, 0, 0, g );

cleanup:
   return g_out;
}

/** \brief Put the spritesheet position for gid on the spritesheet g_sprites in
 *         the rectangle sprite_frame. The rectangle must already have the
 *         correct sprite width and height set when passed.
 * \param
 * \param
 */
SCAFFOLD_INLINE void graphics_get_spritesheet_pos_ortho(
   GRAPHICS* g_sprites, GRAPHICS_RECT* sprite_frame, SCAFFOLD_SIZE gid
) {
   GFX_COORD_TILE tiles_wide = 0;

   scaffold_check_null( g_sprites );

   tiles_wide = g_sprites->w / sprite_frame->w;

   sprite_frame->y = ((gid) / tiles_wide) * sprite_frame->h;
   sprite_frame->x = ((gid) % tiles_wide) * sprite_frame->w;

cleanup:
   return;
}

void graphics_shrink_rect( GRAPHICS_RECT* rect, GFX_COORD_PIXEL shrink_by ) {
   rect->x += shrink_by;
   rect->y += shrink_by;
   rect->h -= (2 * shrink_by);
   rect->w -= (2 * shrink_by);
}

#ifdef USE_RAYCASTING

GFX_RAY_FLOOR* graphics_floorcast_create(
   GFX_RAY_FLOOR* floor_pos, const GFX_RAY* ray, int x, const GFX_DELTA* cam_pos,
   const GFX_RAY_WALL* wall_map_pos, const GRAPHICS* g
) {
   double wall_x_hit; /* Where, exactly, the wall was hit. */

   if( 0 == wall_map_pos->side ) {
      wall_x_hit = cam_pos->y + wall_map_pos->perpen_dist * ray->direction_y;
   } else {
      wall_x_hit = cam_pos->x + wall_map_pos->perpen_dist * ray->direction_x;
   }
   wall_x_hit -= floor( wall_x_hit );

   if( wall_map_pos->side == 0 && 0 < ray->direction_x ) {
      floor_pos->wall_x = wall_map_pos->x;
      floor_pos->wall_y = wall_map_pos->y + wall_x_hit;
   } else if( 0 == wall_map_pos->side && 0 > ray->direction_x ) {
      floor_pos->wall_x = wall_map_pos->x + 1.0;
      floor_pos->wall_y = wall_map_pos->y + wall_x_hit;
   } else if( 1 == wall_map_pos->side && 0 < ray->direction_y ) {
      floor_pos->wall_x = wall_map_pos->x + wall_x_hit;
      floor_pos->wall_y = wall_map_pos->y;
   } else {
      floor_pos->wall_x = wall_map_pos->x + wall_x_hit;
      floor_pos->wall_y = wall_map_pos->y + 1.0;
   }

   return floor_pos;
}

/** \brief
 *
 * \param floor_pos
 * \param
 * \return
 *
 */
GFX_RAY_FLOOR* graphics_floorcast_throw(
   GFX_RAY_FLOOR* floor_pos, int x, int y, int line_height,
   const GFX_DELTA* cam_pos, const GFX_RAY_WALL* wall_map_pos,
   const GRAPHICS* g
) {
   double current_dist;

   current_dist = g->h / (2.0 * y - g->h);

   floor_pos->weight = current_dist / wall_map_pos->perpen_dist;

   floor_pos->x = floor_pos->weight * floor_pos->wall_x +
      (1.0 - floor_pos->weight) * cam_pos->x;
   floor_pos->y = floor_pos->weight * floor_pos->wall_y +
      (1.0 - floor_pos->weight) * cam_pos->y;

   floor_pos->tex_x =
      (int)(floor_pos->x * floor_pos->tex_w) % floor_pos->tex_w;
   floor_pos->tex_y =
      (int)(floor_pos->y * floor_pos->tex_h) % floor_pos->tex_h;

   return floor_pos;
}

GFX_RAY* graphics_raycast_wall_create(
   GFX_RAY* ray, int x, GFX_RAY_WALL* wall_pos, const GFX_DELTA* plane_pos,
   const GFX_DELTA* cam_pos, const GRAPHICS* g
) {
   double camera_x;
   int cam_map_pos_x = (int)(cam_pos->x);
   int cam_map_pos_y = (int)(cam_pos->y);

   memset( wall_pos, sizeof( GFX_RAY_WALL ), '\0' );

   camera_x = 2 * x / (double)(g->w) - 1;
   ray->direction_x = cam_pos->dx.facing + plane_pos->x * camera_x;
   ray->direction_y = cam_pos->dy.facing + plane_pos->y * camera_x;
   ray->delta_dist_x = fabs( 1 / ray->direction_x );
   ray->delta_dist_y = fabs( 1 / ray->direction_y );

   /* Assume distance is finite to start. */
   ray->infinite_dist = FALSE;

   /* Calculate step and initial sideDist. */
   if( 0 > ray->direction_x ) {
      ray->step_x = -GRAPHICS_RAY_INITIAL_STEP_X;
      ray->side_dist_x = (cam_pos->x - cam_map_pos_x) * ray->delta_dist_x;
   } else {
      ray->step_x = GRAPHICS_RAY_INITIAL_STEP_X;
      ray->side_dist_x = (cam_map_pos_x + 1.0 - cam_pos->x) * ray->delta_dist_x;
   }
   if( 0 > ray->direction_y ) {
      ray->step_y = -GRAPHICS_RAY_INITIAL_STEP_Y;
      ray->side_dist_y = (cam_pos->y - cam_map_pos_y) * ray->delta_dist_y;
   } else {
      ray->step_y = GRAPHICS_RAY_INITIAL_STEP_X;
      ray->side_dist_y = (cam_map_pos_y + 1.0 - cam_pos->y) * ray->delta_dist_y;
   }

   return ray;
}

void graphics_raycast_wall_iter( GFX_RAY_WALL* wall_pos, GFX_RAY* ray ) {
   /* Jump to next map square, OR in x-direction, OR in y-direction. */
   if( ray->side_dist_x < ray->side_dist_y ) {
      ray->side_dist_x += ray->delta_dist_x;
      wall_pos->x += ray->step_x;
      wall_pos->side = 0;
   } else {
      ray->side_dist_y += ray->delta_dist_y;
      wall_pos->y += ray->step_y;
      wall_pos->side = 1;
   }

   /* Don't draw walls outside of the map. */
   if(
      wall_pos->x > wall_pos->map_w ||
      wall_pos->y > wall_pos->map_h ||
      0 > wall_pos->x ||
      0 > wall_pos->y
   ) {
      ray->infinite_dist = TRUE;
   }
}

double graphics_raycast_get_distance(
   const GFX_RAY_WALL* wall_pos, const GFX_DELTA* cam_pos, const GFX_RAY* ray
) {
   /* Calculate distance projected on camera direction
      (Euclidean distance will give fisheye effect!). */
   if( 0 == wall_pos->side ) {
      return
         (wall_pos->x - cam_pos->x + (1 - ray->step_x) / 2) / ray->direction_x;
   } else {
      return
         (wall_pos->y - cam_pos->y + (1 - ray->step_y) / 2) / ray->direction_y;
   }
}

#if 0
/** \brief
 *
 * \param
 * \param wall_pos A rectangle with W set to the map width. The wall collision
 * will be returned via X and Y. The wall side (NE/SW) will be returned via H.
 * X and Y are in map tiles.
 * \return
 *
 */
int graphics_raycast_wall_throw(
   GFX_RAY* ray, GFX_RAY_WALL* wall_pos,
   const GFX_DELTA* cam_pos, const GRAPHICS* g,
   BOOL (collision_check)( GFX_RAY_WALL*, void* ), void* data
) {
   BOOL wall_hit = FALSE;

   /* Do the actual casting. */
   while( FALSE == wall_hit ) {
      /* Jump to next map square, OR in x-direction, OR in y-direction. */
      if( ray->side_dist_x < ray->side_dist_y ) {
         ray->side_dist_x += ray->delta_dist_x;
         wall_pos->x += ray->step_x;
         wall_pos->side = 0;
      } else {
         ray->side_dist_y += ray->delta_dist_y;
         wall_pos->y += ray->step_y;
         wall_pos->side = 1;
      }

      /* Don't draw walls outside of the map. */
      if(
         wall_pos->x > wall_pos->map_w ||
         wall_pos->y > wall_pos->map_h ||
         0 > wall_pos->x ||
         0 > wall_pos->y
      ) {
         ray->infinite_dist = TRUE;
         break;
      }

      /* Check if ray has hit a wall. */
      if( collision_check( wall_pos, data ) ) {
         wall_hit = TRUE;
      }
   }

   /* Calculate distance projected on camera direction
      (Euclidean distance will give fisheye effect!). */
   if( 0 == wall_pos->side ) {
      wall_pos->perpen_dist =
         (wall_pos->x - cam_pos->x + (1 - ray->step_x) / 2) / ray->direction_x;
   } else {
      wall_pos->perpen_dist =
         (wall_pos->y - cam_pos->y + (1 - ray->step_y) / 2) / ray->direction_y;
   }

   /* Calculate height of line to draw on screen. */
   return (int)(g->h / wall_pos->perpen_dist);
}

#endif

/** \brief Calculate lowest and highest pixel to fill in current stripe.
 *
 * \param
 * \param
 * \return
 *
 */
int graphics_get_ray_stripe_end( int line_height, const GRAPHICS* g ) {
   int draw_end = line_height / 2 + g->h / 2;
   if( draw_end >= g->h ) {
      draw_end = g->h - 1;
   }
   return draw_end;
}

/** \brief Calculate lowest and highest pixel to fill in current stripe.
 *
 * \param
 * \param
 * \return
 *
 */
int graphics_get_ray_stripe_start( int line_height, const GRAPHICS* g ) {
   int draw_start = -line_height / 2 + g->h / 2;
   if( 0 > draw_start ) {
      draw_start = 0;
   }
   return draw_start;
}

#endif /* USE_RAYCASTING */
