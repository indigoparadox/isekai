
#include <stdlib.h>
#include <check.h>

#include "../src/graphics.h"
#include "check_data.h"

static struct tagbstring module = bsStatic( "check_ray.c" );

START_TEST( test_ray_create ) {
   //GRAPHICS_RAY_FPP ray;
   GFX_COORD_PIXEL x_i = 0,
      y_i = 0;
   //GRAPHICS_PLANE_FPP camera,
   //   plane;
   GRAPHICS* g = NULL;
   //GRAPHICS_POINT tex;
   int ray_pass = 0;
   int64_t fp_res = 0,
      dbl_res = 0;

#ifdef RAYCAST_OLD_DOUBLE
   GRAPHICS_PLANE camera_dbl = { 0 },
      plane_dbl = { 0 };
   GRAPHICS_RAY ray_dbl = { 0 };
   GFX_RAY_FLOOR floor_pos_dbl = { 0 };
   GRAPHICS_DELTA wall_pos_dbl = { 0 };
#endif /* RAYCAST_OLD_DOUBLE */

   graphics_screen_new( &g, 640, 480, 0, 0, 0, NULL );

   //ray.map_w = 25;
   //ray.map_h = 50;

#ifdef RAYCAST_OLD_DOUBLE
   camera_dbl.precise_x = 10.0;
   camera_dbl.precise_y = 15.0;
   camera_dbl.facing_x = 0;
   camera_dbl.facing_y = 1.0;
   plane_dbl.precise_x = -GRAPHICS_RAY_FOV;
   plane_dbl.precise_y = 0;
   wall_pos_dbl.perpen_dist = 0;
   //wall_pos_dbl.map_x = 10;
   //wall_pos_dbl.map_y = 15;
   wall_pos_dbl.map_w = 40;
   wall_pos_dbl.map_h = 30;
   //floor_pos_dbl.tex_w = GRAPHICS_SPRITE_WIDTH;
   //floor_pos_dbl.tex_h = GRAPHICS_SPRITE_HEIGHT;
#endif /* RAYCAST_OLD_DOUBLE */

   #if 0
   camera.fp_x = graphics_precise( 10 );
   camera.fp_y = graphics_precise( 15 );
   camera.fp_facing_x = graphics_precise( 0 ); /* TODO */
   camera.fp_facing_y = graphics_precise( 1 );
   plane.fp_x = -GRAPHICS_RAY_FOV_FP;
   plane.fp_y = graphics_precise( 0 );
   #endif // 0

   for( x_i = 0 ; GRAPHICS_SCREEN_WIDTH > x_i ; x_i++ ) {

#ifdef RAYCAST_OLD_DOUBLE
      graphics_raycast_wall_create( &ray_dbl, x_i, &plane_dbl, &camera_dbl, g );
      /* graphics_floorcast_create(
         &floor_pos_dbl, &ray_dbl, x_i, &camera_dbl, &wall_pos_dbl, g
      ); */

      //ck_assert_int_eq( wall_pos_dbl.map_x, 10 );
      //ck_assert_int_eq( wall_pos_dbl.map_y, 15 );

      ray_pass = 0;
      do {

         graphics_raycast_wall_iterate( &wall_pos_dbl, &ray_dbl, g );

         if( 0 == x_i && 0 == ray_pass ) {
            ck_assert_int_eq( wall_pos_dbl.map_x, 10 );
            ck_assert_int_eq( wall_pos_dbl.map_y, 16 );
            ck_assert_int_ge( (int)ray_dbl.side_dist_x, 1 );
            ck_assert_int_eq( (int)ray_dbl.side_dist_y, 2 );
            ck_assert_int_eq( (int)wall_pos_dbl.stripe_x_hit, 0 );
            ck_assert_int_eq( wall_pos_dbl.side, RAY_SIDE_EAST_WEST );
         } else if( 0 == x_i && 10 == ray_pass ) {
            ck_assert_int_eq( wall_pos_dbl.map_x, 14 );
            ck_assert_int_eq( wall_pos_dbl.map_y, 22 );
            ck_assert_int_ge( (int)ray_dbl.side_dist_x, 7 );
            ck_assert_int_eq( (int)ray_dbl.side_dist_y, 8 );
            ck_assert_int_eq( (int)wall_pos_dbl.stripe_x_hit, 0 );
            ck_assert_int_eq( wall_pos_dbl.side, RAY_SIDE_EAST_WEST );
         } else if( 0 == x_i && 20 == ray_pass ) {
            ck_assert_int_eq( wall_pos_dbl.map_x, 18 );
            ck_assert_int_eq( wall_pos_dbl.map_y, 28 );
            ck_assert_int_eq( wall_pos_dbl.perpen_dist, 12 );
            ck_assert_int_ge( (int)ray_dbl.side_dist_x, 13 );
            ck_assert_int_eq( (int)ray_dbl.side_dist_y, 14 );
            ck_assert_int_eq( (int)wall_pos_dbl.stripe_x_hit, 0 );
            ck_assert_int_eq( wall_pos_dbl.cell_height, 40 );
            ck_assert_int_eq( wall_pos_dbl.side, RAY_SIDE_EAST_WEST );
         } else if( 1 == x_i && 10 == ray_pass ) {
            ck_assert_int_eq( wall_pos_dbl.map_x, 14 );
            ck_assert_int_eq( wall_pos_dbl.map_y, 22 );
            ck_assert_int_eq( wall_pos_dbl.perpen_dist, 6 );
            ck_assert_int_ge( (int)ray_dbl.side_dist_x, 7 );
            ck_assert_int_eq( (int)ray_dbl.side_dist_y, 8 );
            ck_assert_int_eq( (int)wall_pos_dbl.stripe_x_hit, 0 );
            ck_assert_int_eq( wall_pos_dbl.cell_height, 80 );
            ck_assert_int_eq( wall_pos_dbl.side, RAY_SIDE_EAST_WEST );
         }

         ray_pass++;
      } while( !graphics_raycast_point_is_infinite( &wall_pos_dbl ) );

#endif /* RAYCAST_OLD_DOUBLE */

      //graphics_raycast_create( &ray, x_i, &plane, &camera, g );

      for( y_i = 0 ; GRAPHICS_SCREEN_HEIGHT > y_i ; y_i++ ) {
#ifdef RAYCAST_OLD_DOUBLE
         graphics_floorcast_throw(
            &floor_pos_dbl, x_i, y_i,
            &camera_dbl, &wall_pos_dbl, &ray_dbl,
            g
         );
#endif /* RAYCAST_OLD_DOUBLE */

#if 0
         graphics_raycast_iterate( &ray, &plane, &camera, g );
         //printf( "%ld\n", ray.fp_perpen_dist );
         graphics_raycast_floor_texture( &tex, &ray, &camera, y_i, g );

         dbl_res = ray_dbl.delta_dist_x * GRAPHICS_FIXED_PRECISION;
         ck_assert_int_eq( ray.x, wall_pos_dbl.x );

         dbl_res = ray_dbl.side_dist_x * GRAPHICS_FIXED_PRECISION;
         ck_assert_int_eq( ray.y, wall_pos_dbl.y );

         dbl_res = ray_dbl.delta_dist_x * GRAPHICS_FIXED_PRECISION;
         ck_assert_int_eq(
            ray.fp_delta_dist_x,
            dbl_res
         );

         dbl_res = ray_dbl.side_dist_x * GRAPHICS_FIXED_PRECISION;
         ck_assert_int_eq(
            ray.fp_side_dist_x,
            dbl_res
         );

         dbl_res = ray_dbl.delta_dist_y * GRAPHICS_FIXED_PRECISION;
         ck_assert_int_eq(
            ray.fp_delta_dist_y,
            dbl_res
         );

         dbl_res = ray_dbl.side_dist_y * GRAPHICS_FIXED_PRECISION;
         ck_assert_int_eq(
            ray.fp_side_dist_y,
            dbl_res
         );

         dbl_res = wall_pos_dbl.perpen_dist * GRAPHICS_FIXED_PRECISION;
         ck_assert_int_eq(
            ray.fp_perpen_dist,
            dbl_res
         );
#endif // 0
      }
   }

cleanup:
   return;
}
END_TEST

Suite* ray_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Ray" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_ray_create );
   suite_add_tcase( s, tc_core );

   return s;
}
