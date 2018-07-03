
#define GRAPHICS_C
#include "graphics.h"

#ifdef USE_CLOCK
#include <time.h>
#endif /* USE_CLOCK */

#include <math.h>

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
   scaffold_assert( 4 == header->bpp ); /* TODO: Polite error message. */
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

struct VECTOR* graphics_get_line(
   GFX_COORD_PIXEL x1, GFX_COORD_PIXEL y1,
   GFX_COORD_PIXEL x2, GFX_COORD_PIXEL y2
) {
   GFX_COORD_FPP fp_delta_x,
      fp_delta_y,
      delta_err,
      error = 0,
      tolerance = graphics_precise( 0.5 );
   GFX_COORD_PIXEL xi = 0,
      yi = y1,
      delta_x,
      delta_y,
      y_diff = 1;
   GRAPHICS_POINT* point_i = NULL;
   struct VECTOR* points = NULL;

   vector_new( points );

   delta_x = x2 - x1;
   delta_y = y2 - y1;

   fp_delta_x = graphics_precise( delta_x );
   fp_delta_y = graphics_precise( delta_y );

   /* Special Case: Vertical Line */
   if( 0 == fp_delta_x ) {
      xi = x1;
      for( yi = y1 ; y2 > yi ; yi++ ) {
         point_i = mem_alloc( 1, GRAPHICS_POINT );
         point_i->x = xi;
         point_i->y = yi;
         vector_add( points, point_i );
      }
      goto cleanup;
   }

   /* Special Case: Horizontal Line */
   if( 0 == fp_delta_y ) {
      yi = y1;
      for( xi = x1 ; x2 > xi ; xi++ ) {
         point_i = mem_alloc( 1, GRAPHICS_POINT );
         point_i->x = xi;
         point_i->y = yi;
         vector_add( points, point_i );
      }
      goto cleanup;
   }

   /* They're both in FPP mode, so division should work? */
   /* TODO: Vertical line? */
   delta_err = abs( graphics_divide_fp( fp_delta_y, fp_delta_x ) );

   if( 0 > fp_delta_y ) {
      y_diff = -1;
   }

   for( xi = x1 ; x2 > xi ; xi++ ) {
      point_i = mem_alloc( 1, GRAPHICS_POINT );
      point_i->x = xi;
      point_i->y = yi;

      vector_add( points, point_i );

      error += delta_err;
      while( error >= tolerance ) { /* 0.5 */
         yi += y_diff;
         error -= 100;
      }
   }

cleanup:
   return points;
}

void graphics_draw_line(
   GRAPHICS* g, GFX_COORD_PIXEL x1, GFX_COORD_PIXEL y1,
   GFX_COORD_PIXEL x2, GFX_COORD_PIXEL y2, GRAPHICS_COLOR color
) {
   struct VECTOR* points = NULL;
   int i = 0;
   GRAPHICS_POINT* point_i = NULL;

   points = graphics_get_line( x1, y1, x2, y2 );
   point_i = vector_get( points, i );
   while( NULL != point_i ) {
      graphics_set_pixel( g, point_i->x, point_i->y, color );
      mem_free( point_i );
      i++;
      point_i = vector_get( points, i );
   }

   /* Individual points freed in the loop. */
   vector_free_force( &points );
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

void graphics_surface_set_h( GRAPHICS* g, GFX_COORD_PIXEL h ) {
   g->h = h;
   g->half_h = h / 2;
   g->fp_h = graphics_precise( h );
}

void graphics_surface_set_w( GRAPHICS* g, GFX_COORD_PIXEL w ) {
   g->w = w;
   g->fp_w = graphics_precise( w );
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
   int tiles_wide = 0;

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

GFX_COORD_FPP graphics_multiply_fp( GFX_COORD_FPP value_a, GFX_COORD_FPP value_b ) {
   GFX_COORD_FPP value_out;

   /* Additional layer of precision since we're multiplying. */
   value_out = graphics_precise( value_a );
   value_out *= value_b;

   value_out = graphics_unprecise( value_out );
   value_out = graphics_unprecise( value_out );

   return value_out;
}

GFX_COORD_FPP graphics_divide_fp( GFX_COORD_FPP value_a, GFX_COORD_FPP value_b ) {
   GFX_COORD_FPP value_out;

   /* Additional layer of precision since we're multiplying. */
   value_out = graphics_precise( value_a );
   value_out /= value_b;

   //value_out = graphics_unprecise( value_out );
   //value_out = graphics_unprecise( value_out );

   return value_out;
}

#ifndef DISABLE_MODE_POV

#ifdef RAYCAST_OLD_DOUBLE

/** \brief
 *
 * \param floor_pos
 * \param
 * \return
 *
 */
void graphics_floorcast_throw(
   GFX_RAY_FLOOR* floor_pos, int x, int y, int below_wall_height,
   const GRAPHICS_PLANE* cam_pos, const GRAPHICS_DELTA* wall_map_pos,
   const GRAPHICS_RAY* ray,
   GFX_COORD_PIXEL tile_width, GFX_COORD_PIXEL tile_height,
   const GRAPHICS* g
) {
   double current_dist,
      wall_x,
      wall_y;

   /* Grab the distance and a weight factor to help below. */
   current_dist = g->h / (2.0 * y - g->h); // + below_wall_height;
   floor_pos->weight = current_dist / wall_map_pos->perpen_dist;

   /* Figure out the precise spot on the texture to copy. */
   if( wall_map_pos->side == 0 && 0 < ray->direction_x ) {
      wall_x = wall_map_pos->map_x;
      wall_y = wall_map_pos->map_y + wall_map_pos->stripe_x_hit;
   } else if( 0 == wall_map_pos->side && 0 > ray->direction_x ) {
      wall_x = wall_map_pos->map_x + 1.0;
      wall_y = wall_map_pos->map_y + wall_map_pos->stripe_x_hit;
   } else if( 1 == wall_map_pos->side && 0 < ray->direction_y ) {
      wall_x = wall_map_pos->map_x + wall_map_pos->stripe_x_hit;
      wall_y = wall_map_pos->map_y;
   } else {
      wall_x = wall_map_pos->map_x + wall_map_pos->stripe_x_hit;
      wall_y = wall_map_pos->map_y + 1.0;
   }

   floor_pos->x = floor_pos->weight * wall_x +
      (1.0 - floor_pos->weight) * cam_pos->precise_x;
   floor_pos->y = floor_pos->weight * wall_y +
      (1.0 - floor_pos->weight) * cam_pos->precise_y;

   floor_pos->tex_x =
      (int)(floor_pos->x * tile_width) % tile_width;
   floor_pos->tex_y =
      (int)(floor_pos->y * tile_height) % tile_height;

   //return floor_pos;
}

#endif /* RAYCAST_OLD_DOUBLE */

#ifdef RAYCAST_OLD_DOUBLE

void graphics_raycast_wall_create(
   GRAPHICS_RAY* ray, int x, int map_w, int map_h,
   const GRAPHICS_PLANE* plane_pos,
   const GRAPHICS_PLANE* cam_pos, const GRAPHICS* g
) {
   double camera_x;
   int cam_map_pos_x = (int)(cam_pos->precise_x);
   int cam_map_pos_y = (int)(cam_pos->precise_y);

   camera_x = 2 * x / (double)(g->w) - 1;
   ray->direction_x = cam_pos->facing_x + plane_pos->precise_x * camera_x;
   ray->direction_y = cam_pos->facing_y + plane_pos->precise_y * camera_x;
   ray->delta_dist_x = fabs( 1 / ray->direction_x );
   ray->delta_dist_y = fabs( 1 / ray->direction_y );
   ray->map_x = cam_map_pos_x;
   ray->map_y = cam_map_pos_y;
   ray->map_w = map_w;
   ray->map_h = map_h;
   ray->origin_x = cam_pos->precise_x;
   ray->origin_y = cam_pos->precise_y;

   /* Calculate step and initial sideDist. */
   if( 0 > ray->direction_x ) {
      ray->step_x = -GRAPHICS_RAY_INITIAL_STEP_X;
      ray->side_dist_x =
         (cam_pos->precise_x - cam_map_pos_x) * ray->delta_dist_x;
   } else {
      ray->step_x = GRAPHICS_RAY_INITIAL_STEP_X;
      ray->side_dist_x =
         (cam_map_pos_x + 1.0 - cam_pos->precise_x) * ray->delta_dist_x;
   }
   if( 0 > ray->direction_y ) {
      ray->step_y = -GRAPHICS_RAY_INITIAL_STEP_Y;
      ray->side_dist_y =
         (cam_pos->precise_y - cam_map_pos_y) * ray->delta_dist_y;
   } else {
      ray->step_y = GRAPHICS_RAY_INITIAL_STEP_Y;
      ray->side_dist_y =
         (cam_map_pos_y + 1.0 - cam_pos->precise_y) * ray->delta_dist_y;
   }
}

BOOL graphics_raycast_point_is_infinite( const GRAPHICS_DELTA* point ) {
   if(
      point->map_x >= point->map_w ||
      point->map_y >= point->map_h ||
      0 > point->map_x ||
      0 > point->map_y
   ) {
      return TRUE;
   }
   return FALSE;
}

void graphics_raycast_wall_iterate(
   GRAPHICS_DELTA* point, GRAPHICS_RAY* ray, int prev_wall_height,
   const GRAPHICS* g
) {
   double dist_tmp;

   /* Jump to next map square, OR in x-direction, OR in y-direction. */
   if( ray->side_dist_x < ray->side_dist_y ) {
      ray->side_dist_x += ray->delta_dist_x;
      ray->map_x += ray->step_x;
      point->side = RAY_SIDE_NORTH_SOUTH;
   } else {
      ray->side_dist_y += ray->delta_dist_y;
      ray->map_y += ray->step_y;
      point->side = RAY_SIDE_EAST_WEST;
   }
   point->map_x = ray->map_x;
   point->map_y = ray->map_y;
   point->map_w = ray->map_w;
   point->map_h = ray->map_h;

   /* Calculate distance projected on camera direction
      (Euclidean distance will give fisheye effect!). */
   if( RAY_SIDE_NORTH_SOUTH == point->side ) {
      dist_tmp = point->map_x - ray->origin_x + (-1 - ray->step_x) / 2;
      point->perpen_dist = dist_tmp / ray->direction_x;
   } else {
      //wall_pos->perpen_dist =
      //   (wall_pos->y - cam_pos->y + (-1 - ray->step_y) / 2) / ray->direction_y;
      dist_tmp = point->map_y - ray->origin_y + (-1 - ray->step_y) / 2;
      point->perpen_dist = dist_tmp / ray->direction_y;
   }
   //point->cell_height = (int)(g->h / point->perpen_dist);

   /* Figure out the precise pixel on the wall hit by this stripe, for
    * texture-mapping purposes. */
   if( 0 == point->side ) {
      point->stripe_x_hit =
         ray->origin_x + point->perpen_dist * ray->direction_y;
   } else {
      point->stripe_x_hit =
         ray->origin_x + point->perpen_dist * ray->direction_x;
   }
   point->stripe_x_hit -= floor( point->stripe_x_hit );
}

#endif /* RAYCAST_OLD_DOUBLE */

#endif /* !DISABLE_MODE_POV */

#ifndef DISABLE_ISOMETRIC

void graphics_isometric_tile_rotate(
   int* x, int* y, int width, int height, GRAPHICS_ISO_ROTATE rotation
) {
   switch( rotation ) {
      case GRAPHICS_ISO_ROTATE_90:
         *x = *x ^ *y;
         *y = *x ^ *y;
         *x = *x ^ *y;
         *y = height - *y;
         break;
      case GRAPHICS_ISO_ROTATE_180:
         *x = width - *x;
         *y = height - *y;
         break;
      case GRAPHICS_ISO_ROTATE_270:
         *x = *x ^ *y;
         *y = *x ^ *y;
         *x = *x ^ *y;
         *x = width - *x;
         break;
   }
}

void graphics_transform_isometric(
   GRAPHICS* g, float tile_x, float tile_y, int* screen_x, int* screen_y
) {
   SCAFFOLD_SIZE vp_x = g->virtual_x;
   SCAFFOLD_SIZE vp_y = g->virtual_y;

   *screen_x = vp_x + (tile_x * GRAPHICS_ISO_TILE_WIDTH / 2) + \
      (tile_y * GRAPHICS_ISO_TILE_WIDTH / 2); \
   *screen_y = vp_y + ((tile_y * GRAPHICS_ISO_TILE_OFFSET_X / 2) - \
         (tile_x * GRAPHICS_ISO_TILE_OFFSET_X / 2));
}

#endif // DISABLE_ISOMETRIC
