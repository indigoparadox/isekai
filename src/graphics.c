
#define GRAPHICS_C
#include "graphics.h"

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

#ifndef DISABLE_MODE_POV

#ifdef RAYCAST_OLD_DOUBLE

#if 0

void graphics_floorcast_create(
   GFX_RAY_FLOOR* floor_pos, const GRAPHICS_RAY* ray, int x, const GRAPHICS_PLANE* cam_pos,
   const GRAPHICS_DELTA* wall_map_pos, const GRAPHICS* g
) {
   //double wall_x_hit; /* Where, exactly, the wall was hit. */

   if( 0 == wall_map_pos->side ) {
      wall_map_pos->stripe_x_hit = cam_pos->precise_y + wall_map_pos->perpen_dist * ray->direction_y;
   } else {
      wall_map_pos->stripe_x_hit = cam_pos->precise_x + wall_map_pos->perpen_dist * ray->direction_x;
   }
   wall_map_pos->stripe_x_hit -= floor( wall_map_pos->stripe_x_hit );

   /*
   if( wall_map_pos->side == 0 && 0 < ray->direction_x ) {
      floor_pos->wall_x = wall_map_pos->map_x;
      floor_pos->wall_y = wall_map_pos->map_y + wall_x_hit;
   } else if( 0 == wall_map_pos->side && 0 > ray->direction_x ) {
      floor_pos->wall_x = wall_map_pos->map_x + 1.0;
      floor_pos->wall_y = wall_map_pos->map_y + wall_x_hit;
   } else if( 1 == wall_map_pos->side && 0 < ray->direction_y ) {
      floor_pos->wall_x = wall_map_pos->map_x + wall_x_hit;
      floor_pos->wall_y = wall_map_pos->map_y;
   } else {
      floor_pos->wall_x = wall_map_pos->map_x + wall_x_hit;
      floor_pos->wall_y = wall_map_pos->map_y + 1.0;
   }
   */

   //return floor_pos;
}

#endif // 0

/** \brief
 *
 * \param floor_pos
 * \param
 * \return
 *
 */
void graphics_floorcast_throw(
   GFX_RAY_FLOOR* floor_pos, int x, int y,
   const GRAPHICS_PLANE* cam_pos, const GRAPHICS_DELTA* wall_map_pos,
   const GRAPHICS_RAY* ray, const GRAPHICS* g
) {
   double current_dist,
      wall_x,
      wall_y;

   /* Grab the distance and a weight factor to help below. */
   current_dist = g->h / (2.0 * y - g->h);
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
      (int)(floor_pos->x * GRAPHICS_SPRITE_WIDTH) % GRAPHICS_SPRITE_WIDTH;
   floor_pos->tex_y =
      (int)(floor_pos->y * GRAPHICS_SPRITE_HEIGHT) % GRAPHICS_SPRITE_HEIGHT;

   //return floor_pos;
}

#endif /* RAYCAST_OLD_DOUBLE */

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

#if 0

void graphics_raycast_create(
   GRAPHICS_RAY_FPP* ray, GFX_COORD_PIXEL stripe,
   const GRAPHICS_PLANE_FPP* plane_pos, const GRAPHICS_PLANE_FPP* cam_pos,
   const GRAPHICS* g
) {
   GFX_COORD_FPP fp_cam_map_floor_x,
      fp_cam_map_floor_y,
      fp_camera_x;
   GFX_COORD_PIXEL delta_tmp;

   ray->x = graphics_unprecise( cam_pos->fp_x );
   ray->y = graphics_unprecise( cam_pos->fp_y );

   /* This is the mathematical floor, not the ground. i.e. floor() */
   fp_cam_map_floor_x = graphics_precise( ray->x );
   fp_cam_map_floor_y = graphics_precise( ray->y );

   //camera_x = 2 * x / (double)(g->w) - 1;
   fp_camera_x = graphics_precise( 2 * stripe );
   fp_camera_x = graphics_divide_fp( fp_camera_x, g->fp_w );
   fp_camera_x -= graphics_precise( 1 );

   /* Get the direction vector for this stripe. */
   ray->fp_direction_x = cam_pos->fp_facing_x +
      graphics_multiply_fp( plane_pos->fp_x, fp_camera_x );
   ray->fp_direction_y = cam_pos->fp_facing_y +
      graphics_multiply_fp( plane_pos->fp_y, fp_camera_x );
   if( 0 == ray->fp_direction_x ) {
      ray->fp_delta_dist_x = 0;
   } else {
      delta_tmp =
         graphics_divide_fp( graphics_precise( 1 ), ray->fp_direction_x );
      ray->fp_delta_dist_x = abs( delta_tmp );
   }
   if( 0 == ray->fp_direction_y ) {
      ray->fp_delta_dist_x = 0;
   } else {
      delta_tmp =
         graphics_divide_fp( graphics_precise( 1 ), ray->fp_direction_y );
      ray->fp_delta_dist_y = abs( delta_tmp );
   }
   ray->steps = 0;
   ray->infinite_dist = FALSE;

   /* Calculate step and initial side_dist. */
   if( 0 > ray->fp_direction_x ) {
      ray->step_x = -GRAPHICS_RAY_INITIAL_STEP_X;
      ray->fp_side_dist_x = graphics_multiply_fp(
         cam_pos->fp_x - fp_cam_map_floor_x, ray->fp_delta_dist_x );
   } else {
      ray->step_x = GRAPHICS_RAY_INITIAL_STEP_X;
      ray->fp_side_dist_x = graphics_multiply_fp(
         fp_cam_map_floor_x + graphics_precise( 1 ) - cam_pos->fp_x,
         ray->fp_delta_dist_x );
   }
   if( 0 > ray->fp_direction_y ) {
      ray->step_y = -GRAPHICS_RAY_INITIAL_STEP_Y;
      ray->fp_side_dist_y = graphics_multiply_fp(
         cam_pos->fp_y - fp_cam_map_floor_y,
         ray->fp_delta_dist_y );
   } else {
      ray->step_y = GRAPHICS_RAY_INITIAL_STEP_Y;
      ray->fp_side_dist_y = graphics_multiply_fp(
         fp_cam_map_floor_y + graphics_precise( 1 ) - cam_pos->fp_y,
         ray->fp_delta_dist_y );
   }
}

void graphics_raycast_iterate(
   GRAPHICS_RAY_FPP* ray, const GRAPHICS_PLANE_FPP* plane_pos,
   const GRAPHICS_PLANE_FPP* cam_pos, const GRAPHICS* g
) {
   GFX_COORD_FPP
      fp_ray_tile_x,
      fp_ray_tile_y,
      fp_ray_step_x,
      fp_ray_step_y,
      dist_tmp;

   /* Jump to next map square, OR in x-direction, OR in y-direction. */
   if( ray->fp_side_dist_x < ray->fp_side_dist_y ) {
      ray->fp_side_dist_x += ray->fp_delta_dist_x;
      ray->x += ray->step_x;
      ray->side = RAY_SIDE_NORTH_SOUTH;
   } else {
      ray->fp_side_dist_y += ray->fp_delta_dist_y;
      ray->y += ray->step_y;
      ray->side = RAY_SIDE_EAST_WEST;
   }

   /* Assume distance is finite to start. */
   ray->infinite_dist = FALSE;

#ifdef LIMIT_RAY_STEPS
   ray->steps++;
   if( 10 < ray->steps ) {
      if( ray->side_dist_x >= ray->side_dist_y ) {
         ray->side_dist_y += ray->delta_dist_y;
         wall_pos->y += ray->step_y;
         wall_pos->side = 1;
      }
      ray->infinite_dist = TRUE;
   }
#endif /* LIMIT_RAY_STEPS */

   /* Don't draw walls outside of the map. */
   if(
      ray->x > ray->map_w ||
      ray->y > ray->map_h ||
      0 > ray->x ||
      0 > ray->y
   ) {
      ray->infinite_dist = TRUE;
   }

   fp_ray_tile_x = graphics_precise( ray->x );
   fp_ray_tile_y = graphics_precise( ray->y );
   fp_ray_step_x = graphics_precise( ray->step_x );
   fp_ray_step_y = graphics_precise( ray->step_y );
   if( RAY_SIDE_NORTH_SOUTH == ray->side ) {
      if( 0 < ray->fp_direction_x ) {
         //wall_pos->perpen_dist =
         //   (wall_pos->x - cam_pos->x + (-1 - ray->step_x) / 2) / ray->direction_x;
         dist_tmp = fp_ray_tile_x - cam_pos->fp_x +
            graphics_divide_fp( graphics_precise( -1 ) - fp_ray_step_x,
               graphics_precise( 2 ) );
         ray->fp_perpen_dist = graphics_divide_fp(
            dist_tmp,
            ray->fp_direction_x
         );
      } else {
         ray->fp_perpen_dist = 0;
      }
   } else {
      if( 0 < ray->fp_direction_y ) {
         //wall_pos->perpen_dist =
         //   (wall_pos->y - cam_pos->y + (-1 - ray->step_y) / 2) / ray->direction_y;
         dist_tmp = fp_ray_tile_y - cam_pos->fp_y +
            graphics_divide_fp( graphics_precise( -1 ) - fp_ray_step_y,
               graphics_precise( 2 ) );
         ray->fp_perpen_dist = graphics_divide_fp(
            dist_tmp,
            ray->fp_direction_y
         );
      } else {
         ray->fp_perpen_dist = 0;
      }
   }
}

void graphics_raycast_floor_texture(
   GRAPHICS_POINT* tex, GRAPHICS_RAY_FPP* ray, const GRAPHICS_PLANE_FPP* cam_pos,
   GFX_COORD_PIXEL y, GRAPHICS* g
) {
   GFX_COORD_FPP fp_current_dist,
      fp_weight,
      fp_floor_spot_x,
      fp_floor_spot_y,
      fp_y_times_2;

   fp_y_times_2 = graphics_multiply_fp( graphics_precise( 2 ), graphics_precise( y ) );
   fp_y_times_2 -= g->fp_h;
   if( 0 == fp_y_times_2 ) {
      fp_current_dist = 0;
   } else {
      fp_current_dist = graphics_divide_fp( g->fp_h, fp_y_times_2 );
   }

   if( 0 == ray->fp_perpen_dist ) {
      return;
   }

   fp_weight = graphics_divide_fp( fp_current_dist, ray->fp_perpen_dist );

   fp_floor_spot_x = (fp_weight * ray->x) +
      graphics_multiply_fp(
         graphics_precise( 1 ) - fp_weight, cam_pos->fp_x );
   fp_floor_spot_y = (fp_weight * ray->y) +
      graphics_multiply_fp(
         graphics_precise( 1 ) - fp_weight, cam_pos->fp_y );

   tex->x = graphics_unprecise(
      (fp_floor_spot_x * GRAPHICS_SPRITE_WIDTH) % GRAPHICS_SPRITE_WIDTH );
   tex->x = graphics_unprecise(
      (fp_floor_spot_y * GRAPHICS_SPRITE_HEIGHT) % GRAPHICS_SPRITE_HEIGHT );
}

 #endif // 0

#ifdef RAYCAST_OLD_DOUBLE

void graphics_raycast_wall_create(
   GRAPHICS_RAY* ray, int x, GRAPHICS_DELTA* wall_pos, const GRAPHICS_PLANE* plane_pos,
   const GRAPHICS_PLANE* cam_pos, const GRAPHICS* g
) {
   double camera_x;
   int cam_map_pos_x = (int)(cam_pos->precise_x);
   int cam_map_pos_y = (int)(cam_pos->precise_y);

   memset( wall_pos, sizeof( GRAPHICS_DELTA ), '\0' );

   camera_x = 2 * x / (double)(g->w) - 1;
   ray->direction_x = cam_pos->facing_x + plane_pos->precise_x * camera_x;
   ray->direction_y = cam_pos->facing_y + plane_pos->precise_y * camera_x;
   ray->delta_dist_x = fabs( 1 / ray->direction_x );
   ray->delta_dist_y = fabs( 1 / ray->direction_y );
   wall_pos->steps = 0;
   ray->origin_x = cam_pos->precise_x;
   ray->origin_y = cam_pos->precise_y;

   /* Assume distance is finite to start. */
   //ray->infinite_dist = FALSE;

   /* Calculate step and initial sideDist. */
   if( 0 > ray->direction_x ) {
      ray->step_x = -GRAPHICS_RAY_INITIAL_STEP_X;
      wall_pos->side_dist_x =
         (cam_pos->precise_x - cam_map_pos_x) * ray->delta_dist_x;
   } else {
      ray->step_x = GRAPHICS_RAY_INITIAL_STEP_X;
      wall_pos->side_dist_x =
         (cam_map_pos_x + 1.0 - cam_pos->precise_x) * ray->delta_dist_x;
   }
   if( 0 > ray->direction_y ) {
      ray->step_y = -GRAPHICS_RAY_INITIAL_STEP_Y;
      wall_pos->side_dist_y =
         (cam_pos->precise_y - cam_map_pos_y) * ray->delta_dist_y;
   } else {
      ray->step_y = GRAPHICS_RAY_INITIAL_STEP_Y;
      wall_pos->side_dist_y =
         (cam_map_pos_y + 1.0 - cam_pos->precise_y) * ray->delta_dist_y;
   }
}

#if 0
void graphics_raycast_wall_throw(
   GRAPHICS_RAY* ray, GFX_RAY_WALL* wall_pos,
   const GFX_DELTA* cam_pos, const GRAPHICS* g,
   BOOL (collision_check)( GFX_RAY_WALL*, void* ), void* data
) {
   BOOL wall_hit = FALSE;
   double dist_tmp;

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
      dist_tmp = wall_pos->x - cam_pos->precise_x + (-1 - ray->step_x) / 2;
      wall_pos->perpen_dist = dist_tmp / ray->direction_x;
   } else {
      //wall_pos->perpen_dist =
      //   (wall_pos->y - cam_pos->y + (-1 - ray->step_y) / 2) / ray->direction_y;
      dist_tmp = wall_pos->y - cam_pos->precise_y + (-1 - ray->step_y) / 2;
      wall_pos->perpen_dist = dist_tmp / ray->direction_y;
   }

   /* Calculate height of line to draw on screen. */
   //return (int)(g->h / wall_pos->perpen_dist);
}
#endif // 0

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

void graphics_raycast_wall_iterate( GRAPHICS_DELTA* point, const GRAPHICS_RAY* ray ) {
   double dist_tmp;

   /* Jump to next map square, OR in x-direction, OR in y-direction. */
   if( point->side_dist_x < point->side_dist_y ) {
      point->side_dist_x += ray->delta_dist_x;
      point->map_x += ray->step_x;
      point->side = RAY_SIDE_NORTH_SOUTH;
   } else {
      point->side_dist_y += ray->delta_dist_y;
      point->map_y += ray->step_y;
      point->side = RAY_SIDE_EAST_WEST;
   }

   /* Assume distance is finite to start. */
   //ray->infinite_dist = FALSE;

#ifdef LIMIT_RAY_STEPS
   ray->steps++;
   if( 10 < ray->steps ) {
      if( ray->side_dist_x >= ray->side_dist_y ) {
         ray->side_dist_y += ray->delta_dist_y;
         wall_pos->y += ray->step_y;
         wall_pos->side = 1;
      }
      ray->infinite_dist = TRUE;
   }
#endif /* LIMIT_RAY_STEPS */

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

   /* Figure out the precise pixel on the wall hit by this stripe, for
    * texture-mapping purposes. */
   if( 0 == point->side ) {
      point->stripe_x_hit = ray->origin_x + point->perpen_dist * ray->direction_y;
   } else {
      point->stripe_x_hit = ray->origin_x + point->perpen_dist * ray->direction_x;
   }
   point->stripe_x_hit -= floor( point->stripe_x_hit );
}

#endif /* RAYCAST_OLD_DOUBLE */

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

#endif /* !DISABLE_MODE_POV */
