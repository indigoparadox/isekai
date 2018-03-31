
#define MODE_C
#include "../mode.h"

#ifndef DISABLE_MODE_POV

#include <stdlib.h>

#include "../callback.h"
#include "../ui.h"
#include "../ipc.h"
#include "../channel.h"
#include "../tilemap.h"
#include "../proto.h"
#include "../mobile.h"

typedef enum {
   POV_LAYER_LEVEL_MAX = 4,
   POV_LAYER_LEVEL_WALL = 3,
   POV_LAYER_LEVEL_RAISED = 2,
   POV_LAYER_LEVEL_NONE = 1,
   POV_LAYER_LEVEL_INSET = 0
} POV_LAYER;

#define POV_RAISED_DIVISOR 8
#define POV_RAISED_DIVISOR_PENULT (POV_RAISED_DIVISOR - 1)

extern bstring client_input_from_ui;

//static GFX_RAY_WALL* cl_walls = NULL;
//static GFX_RAY_FLOOR* cl_floors = NULL;
static GRAPHICS* ray_view = NULL;
struct HASHMAP tileset_status;

static void mode_pov_mobile_set_animation( struct MOBILE* o, struct CLIENT* c ) {
   bstring buffer = NULL;
   int facing = o->facing;
   struct MOBILE* puppet = c->puppet;

   if( NULL == puppet ) {
      goto cleanup;
   }

   if( o->animation_reset || puppet->animation_reset ) {
      if( MOBILE_FACING_DOWN == puppet->facing ) {
         o->animation_flipped = TRUE;
         switch( o->facing ) {
         case MOBILE_FACING_DOWN:
            facing =  MOBILE_FACING_UP;
            break;
         case MOBILE_FACING_UP:
            facing = MOBILE_FACING_DOWN;
            break;
         case MOBILE_FACING_LEFT:
            facing = MOBILE_FACING_RIGHT;
            break;
         case MOBILE_FACING_RIGHT:
            facing = MOBILE_FACING_LEFT;
            break;
         }
      } else if( MOBILE_FACING_LEFT == puppet->facing ) {
         switch( o->facing ) {
         case MOBILE_FACING_DOWN:
            facing =  MOBILE_FACING_LEFT;
            break;
         case MOBILE_FACING_UP:
            facing = MOBILE_FACING_RIGHT;
            break;
         case MOBILE_FACING_LEFT:
            facing = MOBILE_FACING_UP;
            break;
         case MOBILE_FACING_RIGHT:
            facing = MOBILE_FACING_DOWN;
            break;
         }
      } else if( MOBILE_FACING_RIGHT == puppet->facing ) {
         o->animation_flipped = TRUE;
         switch( o->facing ) {
         case MOBILE_FACING_DOWN:
            facing =  MOBILE_FACING_RIGHT;
            break;
         case MOBILE_FACING_UP:
            facing = MOBILE_FACING_LEFT;
            break;
         case MOBILE_FACING_LEFT:
            facing = MOBILE_FACING_DOWN;
            break;
         case MOBILE_FACING_RIGHT:
            facing = MOBILE_FACING_UP;
            break;
         }
      }
   }

   if( NULL != o->current_animation ) {
      buffer = bformat(
         "%s-%s",
         bdata( o->current_animation->name ),
         str_mobile_facing[facing].data
      );
      if( NULL != hashmap_get( &(o->ani_defs), buffer ) ) {
         o->current_animation = hashmap_get( &(o->ani_defs), buffer );
      }
   }

   o->animation_reset = FALSE;

cleanup:
   bdestroy( buffer );
}

static
void mode_pov_draw_sprite( struct MOBILE* o, struct CLIENT* c, GRAPHICS* g ) {
   double sprite_x = 0,
      sprite_y = 0,
      inv_det = 0,
      sprite_height_theory = 0,
      transform_x = 0,
      transform_y = 0;
   int sprite_height = 0,
      stripe = 0,
      y = 0,
      d = 0,
      stepped_x = 0,
      stepped_y = 0,
      sprite_screen_x = 0,
      sprite_screen_w = 0;
   GRAPHICS_COLOR color = GRAPHICS_COLOR_TRANSPARENT;
   GRAPHICS_RECT draw_rect = { 0 }; /* H and W are really EndY and EndX. */
   GRAPHICS_RECT spritesheet = { 0 };
   GRAPHICS_RECT current_sprite = { 0 };
   struct MOBILE_SPRITE_DEF* current_frame = NULL;

   if( NULL == o || NULL == o->sprites ) {
      goto cleanup;
   }

   /* Translate sprite position to relative to camera. */
   sprite_x = o->x - c->cam_pos.precise_x;
   sprite_y = o->y - c->cam_pos.precise_y;

   /* Transform sprite with the inverse camera matrix:
    *
    * [ plane.x  cam.dir.x ] -1       1 /              [ cam.dir.x  -cam.dir.x ]
    * [                    ] = (plane.x * cam.dir.y -  [                       ]
    * [ plane.y  cam.dir.y ]    cam.dir.x * plane.y) * [ -plane.y   plane.x    ]
    */

   /* Required for correct matrix multiplication. */
   inv_det = 1.0 / (c->plane_pos.precise_x * c->cam_pos.facing_y - c->cam_pos.facing_x * c->plane_pos.precise_y);

   transform_x = inv_det * (c->cam_pos.facing_y * sprite_x - c->cam_pos.facing_x * sprite_y);
   /* This is actually the depth inside the screen, what Z is in 3D. */
   transform_y = inv_det * (-c->plane_pos.precise_y * sprite_x + c->plane_pos.precise_x * sprite_y);

   sprite_screen_x = ((int)(g->w / 2) * (1 + transform_x / transform_y));

   /* Calculate height of the sprite on screen.
    * Using "transform_y" instead of the real distance prevents fisheye effect. */
   sprite_height_theory = g->h / transform_y;
   if( 0 < sprite_height_theory ) {
      sprite_height = abs( (int)sprite_height_theory );
   } else {
      goto cleanup;
   }

   /* Calculate lowest and highest pixel to fill in in the current stripe. */
   draw_rect.y = -sprite_height / 2 + g->h / 2;
   if( 0 > draw_rect.y ) {
      draw_rect.y = 0;
   }
   draw_rect.h = sprite_height / 2 + g->h / 2;
   if( draw_rect.h >= g->h ) {
      draw_rect.h = g->h - 1;
   }

   /* Calculate width of the sprite on the screen. */
   sprite_screen_w = abs( (int)(g->h / (transform_y)));
   draw_rect.x = -sprite_screen_w / 2 + sprite_screen_x;
   if( draw_rect.x < 0 ) {
      draw_rect.x = 0;
   }
   draw_rect.w = sprite_screen_w / 2 + sprite_screen_x;
   if( draw_rect.w >= g->w) {
      draw_rect.w = g->w - 1;
   }

   current_frame = (struct MOBILE_SPRITE_DEF*)
      vector_get( &(o->current_animation->frames), o->current_frame );
   scaffold_check_null( current_frame );
   current_sprite.w = spritesheet.w = o->sprite_width;
   current_sprite.h = spritesheet.h = o->sprite_height;
   graphics_get_spritesheet_pos_ortho(
      o->sprites, &current_sprite, current_frame->id
   );

   /* Loop through every vertical stripe of the sprite on screen. */
   for( stripe = draw_rect.x ; stripe < draw_rect.w ; stripe++ ) {

      stepped_x = stripe;
      if(
         MOBILE_FACING_LEFT == o->facing && FALSE == o->animation_flipped ||
         MOBILE_FACING_RIGHT == o->facing && FALSE == o->animation_flipped
      ) {
         stepped_x = (stripe - sprite_screen_w) + o->steps_remaining;
      } else {
         stepped_x = (stripe + sprite_screen_w) - o->steps_remaining;
      }

      spritesheet.x =
         ((int)(256 * (stripe - (-sprite_screen_w / 2 + sprite_screen_x))
                     * spritesheet.w / sprite_screen_w) / 256);

      /* Select the correct sprite column. */
      spritesheet.x += current_sprite.x;

      /* Ensure things are visible with these conditions: */
      if(
         transform_y > 0 && /* 1) It's in front of camera plane. */
         stripe > 0 &&      /* 2) It's on the screen (left). */
         stripe < g->w &&   /* 3) It's on the screen (right). */
         /* 4) ZBuffer, with perpendicular distance. */
         transform_y < c->z_buffer[stripe]
      ) {

         /* For every pixel of the current stripe: */
         for( y = draw_rect.y ; y < draw_rect.h; y++ ) {
            /* Grab the pixel from the currently selected sprite region.
             * Use 256 and 128 factors to avoid floats. */
            d = y * 256 - g->h * 128 + sprite_height * 128;
            spritesheet.y = (((d * spritesheet.h) / sprite_height) / 256);

            /* Select the correct sprite row. */
            spritesheet.y += current_sprite.y;

            stepped_y = y;

            /* Perform the pixel-by-pixel stretch blit from spritesheet to
             * output canvas. */
            color = graphics_get_pixel( o->sprites, spritesheet.x, spritesheet.y );
            if( GRAPHICS_COLOR_TRANSPARENT != color ) {
               graphics_set_pixel( g, stepped_x, stepped_y, color );
            }
         }
      }
   }

cleanup:
   return;
}

static
void* mode_pov_draw_mobile_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct CLIENT* c = twindow->local_client;

   if( NULL == o ) {
      goto cleanup; /* Silently. */
   }

   if( TRUE == o->animation_reset ) {
      mode_pov_mobile_set_animation( o, twindow->local_client );
   }
   mobile_animate( o );
   mode_pov_draw_sprite( o, c, twindow->g );

cleanup:
   return NULL;
}

static void* mode_pov_mob_calc_dist_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct MOBILE* m = (struct MOBILE*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP* t = twindow->local_client->active_t;
   struct CLIENT* c = twindow->local_client;

   scaffold_check_null( t );
   if( NULL == m ) {
      goto cleanup; /* Silently. */
   }

   m->ray_distance =
      ((c->cam_pos.precise_x - m->x) * (c->cam_pos.precise_x - m->x) +
         (c->cam_pos.precise_y - m->y) * (c->cam_pos.precise_y - m->y));

cleanup:
   return NULL;
}

static void* mode_pov_mob_sort_dist_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct MOBILE* m = (struct MOBILE*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP* t = twindow->local_client->active_t;

   scaffold_check_null( t );

   /* TODO: Sort by distance. */

cleanup:
   return NULL;
}

#if 0
static BOOL check_ray_wall_collision(
   GRAPHICS_DELTA* wall_map_pos, const struct TILEMAP_LAYER* layer,
   const struct GRAPHICS_TILE_WINDOW* twindow
) {
   int res = 0,
      i;
   //uint32_t tile;
   struct TILEMAP_TILESET* set;
   struct TILEMAP_TILE_DATA* tile_info;
   struct TILEMAP_TERRAIN_DATA* terrain_iter;

   //tile = tilemap_get_tile( layer, wall_map_pos->map_x, wall_map_pos->map_y );
   if( 0 == wall_map_pos->data ) {
      goto cleanup;
   }

   set = tilemap_get_tileset( twindow->local_client->active_t, wall_map_pos->data, NULL );
   if( NULL == set ) {
      goto cleanup;
   }

   tile_info = vector_get( &(set->tiles), wall_map_pos->data - 1 );
   for( i = 0 ; 4 > i ; i++ ) {
      terrain_iter = tile_info->terrain[i];
      if( NULL == terrain_iter ) { continue; }
      if( TILEMAP_MOVEMENT_BLOCK == terrain_iter->movement ) {
         res = 1;
         goto cleanup;
      }
   }

cleanup:
   return res;
}
#endif // 0

static GRAPHICS_COLOR get_wall_color( GRAPHICS_DELTA* wall_pos ) {
#ifdef TECHNICOLOR_RAYS
   switch( (rand() + 1) % 4 ) {
#endif /* TECHNICOLOR_RAYS */
   /* TODO: Wall colors? */
   return 1 == wall_pos->side ? GRAPHICS_COLOR_BLUE : GRAPHICS_COLOR_DARK_BLUE;
#if 0
   switch( worldMap[x][y] ) {
      case -1:
         return GRAPHICS_COLOR_DARK_GREEN;
         break;
      case 1:
         return 1 == wall_pos->side ? GRAPHICS_COLOR_RED : GRAPHICS_COLOR_DARK_RED;
      case 2:
         return 1 == wall_pos->side ? GRAPHICS_COLOR_GREEN : GRAPHICS_COLOR_DARK_GREEN;
      case 3:
         return 1 == wall_pos->side ? GRAPHICS_COLOR_BLUE : GRAPHICS_COLOR_DARK_BLUE;
      case 4:
         return 1 == wall_pos->side ? GRAPHICS_COLOR_CYAN : GRAPHICS_COLOR_DARK_CYAN;
      default:
         return 1 == wall_pos->side ? GRAPHICS_COLOR_YELLOW : GRAPHICS_COLOR_BROWN;
   }
#endif /* 0 */
}

#endif // 0

static void mode_pov_set_facing( struct CLIENT* c, MOBILE_FACING facing ) {
#ifdef RAYCAST_OLD_DOUBLE
   double old_dir_x = 0;
#else
   GFX_COORD_FPP fp_old_dir_x;
#endif /* RAYCAST_OLD_DOUBLE */
   double cos_dbl = 0,
      sin_dbl = 0;
/*
   if( facing == c->cam_pos.facing ) {
      goto cleanup;
   }
*/
   switch( facing ) {
      case MOBILE_FACING_LEFT:
#ifdef RAYCAST_OLD_DOUBLE
         old_dir_x = 0;
         c->cam_pos.facing_x = old_dir_x * cos( GRAPHICS_RAY_ROTATE_INC ) -
            -1 * sin( GRAPHICS_RAY_ROTATE_INC );
         c->cam_pos.facing_y = old_dir_x * sin( GRAPHICS_RAY_ROTATE_INC ) +
            -1 * cos( GRAPHICS_RAY_ROTATE_INC );

         old_dir_x = GRAPHICS_RAY_FOV;
         c->plane_pos.precise_x = old_dir_x * cos( GRAPHICS_RAY_ROTATE_INC ) -
            0 * sin( GRAPHICS_RAY_ROTATE_INC );
         c->plane_pos.precise_y = old_dir_x * sin( GRAPHICS_RAY_ROTATE_INC ) +
            0 * cos( GRAPHICS_RAY_ROTATE_INC );
#else
         fp_old_dir_x = 0;
         c->cam_pos.fp_facing_x = fp_old_dir_x * GRAPHICS_RAY_ROTATE_INC_FP_COS -
            -1 * GRAPHICS_RAY_ROTATE_INC_FP_SIN;
         c->cam_pos.fp_facing_y = fp_old_dir_x * GRAPHICS_RAY_ROTATE_INC_FP_SIN +
            -1 * GRAPHICS_RAY_ROTATE_INC_FP_COS;

         fp_old_dir_x = GRAPHICS_RAY_FOV_FP;
         c->plane_pos.fp_x = fp_old_dir_x * GRAPHICS_RAY_ROTATE_INC_FP_COS -
            0 * GRAPHICS_RAY_ROTATE_INC_FP_SIN;
         c->plane_pos.fp_y = fp_old_dir_x * GRAPHICS_RAY_ROTATE_INC_FP_SIN +
            0 * GRAPHICS_RAY_ROTATE_INC_FP_COS;
#endif /* RAYCAST_OLD_DOUBLE */
         break;

      case MOBILE_FACING_RIGHT:
#ifdef RAYCAST_OLD_DOUBLE
         old_dir_x = 0;
         c->cam_pos.facing_x = old_dir_x * cos( -GRAPHICS_RAY_ROTATE_INC ) -
            -1 * sin( -GRAPHICS_RAY_ROTATE_INC );
         c->cam_pos.facing_y = old_dir_x * sin( -GRAPHICS_RAY_ROTATE_INC ) +
            -1 * cos( -GRAPHICS_RAY_ROTATE_INC );

         old_dir_x = GRAPHICS_RAY_FOV;
         c->plane_pos.precise_x = old_dir_x * cos(-GRAPHICS_RAY_ROTATE_INC ) -
            0 * sin( -GRAPHICS_RAY_ROTATE_INC );
         c->plane_pos.precise_y = old_dir_x * sin( -GRAPHICS_RAY_ROTATE_INC ) +
            0 * cos( -GRAPHICS_RAY_ROTATE_INC );
         break;

      case MOBILE_FACING_UP:
         c->cam_pos.facing_x = 0; /* TODO */
         c->cam_pos.facing_y = -1;
         c->plane_pos.precise_x = GRAPHICS_RAY_FOV;
         c->plane_pos.precise_y = 0;
         break;

      case MOBILE_FACING_DOWN:
         c->cam_pos.facing_x = 0; /* TODO */
         c->cam_pos.facing_y = 1;
         c->plane_pos.precise_x = -GRAPHICS_RAY_FOV;
         c->plane_pos.precise_y = 0;
         break;
   }
}

static BOOL mode_pov_draw_floor(
   GFX_RAY_FLOOR* floor_pos,
   GFX_COORD_PIXEL i_x,
   GFX_COORD_PIXEL wall_draw_end,
   const GRAPHICS_DELTA* wall_map_pos,
   const struct TILEMAP_LAYER* layer,
   const GRAPHICS_RAY* ray, const struct CLIENT* c, GRAPHICS* g
) {
   GFX_COORD_PIXEL i_y = 0;
   struct TILEMAP_TILESET* set = NULL;
   struct TILEMAP* t = NULL;
   SCAFFOLD_SIZE set_firstgid = 0;
   GRAPHICS* g_tileset = NULL;
   BOOL ret_error = FALSE;
   GRAPHICS_RECT tile_tilesheet_pos = { 0 };
   int tex_x = 0,
      tex_y = 0;
   GRAPHICS_COLOR color = GRAPHICS_COLOR_TRANSPARENT;
   uint32_t tile = 0;

   if( NULL != c->active_t ) {
      t = c->active_t;
   } else {
      goto cleanup;
   }

   switch( layer->z ) {
   case POV_LAYER_LEVEL_NONE:
      floor_pos->floor_height = 0;
      break;
   case POV_LAYER_LEVEL_RAISED:
      floor_pos->floor_height = 8;
      break;
   }

   /* Draw the floor from draw_end to the bottom of the screen. */
   for( i_y = wall_draw_end +1; i_y < g->h; i_y++ ) {
      graphics_floorcast_throw(
         floor_pos, i_x, i_y,
         &(c->cam_pos), wall_map_pos, ray,
         g
      );

      /* Ensure we have everything needed to draw the tile. */
      tile = tilemap_get_tile( layer, (int)floor_pos->x, (int)floor_pos->y  );
      if( 0 == tile ) {
         continue;
      }

      set = tilemap_get_tileset( t, tile, &set_firstgid );
      if( NULL == set ) {
         continue;
      }

      g_tileset = hashmap_iterate(
         &(set->images),
         callback_search_tileset_img_gid,
         c
      );
      if( NULL == g_tileset ) {
         /* Tileset not yet loaded, so fail gracefully. */
         ret_error = TRUE;
         continue;
      }

      /* Move the source region on the tilesheet. */
      tilemap_get_tile_tileset_pos(
         set, set_firstgid, g_tileset, tile, &tile_tilesheet_pos );
      tex_x = floor_pos->tex_x + tile_tilesheet_pos.x;
      tex_y = floor_pos->tex_y + tile_tilesheet_pos.y;
      color = graphics_get_pixel( g_tileset, tex_x, tex_y );
      if( GRAPHICS_COLOR_TRANSPARENT != color ) {
         graphics_set_pixel( g, i_x, i_y, color );
      }
   }

cleanup:
   return ret_error;
}

/** \brief Create the first-person view and walls to draw on screen beneath the
 *         sprites.
 *
 * \param
 * \param
 * \return TRUE on error, FALSE otherwise.
 *
 */
static BOOL mode_pov_update_view(
   GRAPHICS_DELTA wall_positions[POV_LAYER_LEVEL_MAX][GRAPHICS_SCREEN_WIDTH],
   GFX_COORD_PIXEL i_x, GRAPHICS_RAY* ray,
   const struct TILEMAP_LAYER* layer, const struct TILEMAP_LAYER* opaque_layer,
   struct CLIENT* c, GRAPHICS* g
) {
   //int draw_start = 0,
   //   draw_end = 0;
   int done = 0;
   GFX_RAY_FLOOR floor_pos = { 0 };
   //int line_height = 0;
   uint32_t tile = 0;
   GRAPHICS_COLOR color = GRAPHICS_COLOR_TRANSPARENT;
   BOOL wall_hit = FALSE;
   BOOL ret_error = FALSE,
      ret_tmp = FALSE;
   GRAPHICS_DELTA* wall_map_pos = &(wall_positions[layer->z][i_x]),
      * opaque_map_pos = NULL;
      //old_map_pos = { 0 };
   struct TILEMAP* t = c->active_t;
   GFX_COORD_PIXEL cell_height = 0,
      wall_draw_end = 0,
      wall_draw_start = 0;

   scaffold_check_null( t );

   //wall_map_pos->data = tilemap_get_tile( layer, wall_map_pos->map_x, wall_map_pos->map_y );

   /* Do the actual casting. */
   wall_hit = FALSE;
   //while( FALSE == wall_hit ) {
      graphics_raycast_wall_iterate( wall_map_pos, ray, g );

      if( NULL != opaque_layer ) {
         scaffold_assert( opaque_layer->z == layer->z + 1 );
         opaque_map_pos = &(wall_positions[opaque_layer->z][i_x]);
         opaque_map_pos->data = tilemap_get_tile( opaque_layer, wall_map_pos->map_x, wall_map_pos->map_y );
      }

      if( graphics_raycast_point_is_infinite( wall_map_pos ) ) {
         /* The ray has to stop at some point, or this will become an
          * infinite loop! */
         /*if(
            MOBILE_FACING_LEFT == facing ||
            MOBILE_FACING_RIGHT == facing
         ) {
            //ray.side_dist_x += ray.delta_dist_x;
            //wall_map_pos.map_x += ray.step_x;
            wall_map_pos->side = 0;
         }*/

         goto start_drawing;
      }

      /* Check if ray has hit a wall. */
      if( NULL != opaque_layer && 0 != opaque_map_pos->data ) {
         wall_hit = TRUE;
         goto start_drawing;
      }
   //}

   ret_tmp = mode_pov_update_view( wall_positions, i_x, ray, layer, opaque_layer, c, g );
   if( TRUE == ret_tmp ) {
      ret_error = TRUE;
      goto cleanup;
   }

start_drawing:

   c->z_buffer[i_x] = wall_map_pos->perpen_dist;
   cell_height = (int)(g->h / wall_map_pos->perpen_dist);

   wall_draw_end = (cell_height / 2) + (g->h / 2);
   //draw_end = ray.origin_x + wall_map_pos->perpen_dist * ray.direction_y;
   if( wall_draw_end >= g->h ) {
      wall_draw_end = g->h - 1;
   }/*
   draw_start = (-line_height / 2) + (g->h / 2);
   */
   wall_draw_start = wall_draw_end - cell_height;
   if( 0 > wall_draw_start ) {
      wall_draw_start = 0;
   }

   /* graphics_floorcast_create(
      &floor_pos, &ray, i_x, &(c->cam_pos),
      &wall_map_pos, g
   ); */

   if( 0 > wall_draw_end ) {
      /* Clamp line bottom to the screen bottom. */
      wall_draw_end = g->h;
   }

   //memcpy( &old_map_pos, wall_map_pos, sizeof( GRAPHICS_DELTA ) );

   /*
   if( g->w > i_x ) {
      ret_tmp = mode_pov_update_view(
         wall_positions, i_x + 1, &ray, layer, opaque_layer, c, g
      );
      if( ret_tmp ) {
         ret_error = TRUE;
      }
   } else {
      goto cleanup;
   };
   */

   /* Draw the pixels of the stripe as a vertical line. */
   if( 0 < cell_height ) {
      if( !graphics_raycast_point_is_infinite( wall_map_pos ) ) {
         /* Choose wall color. */
         color = get_wall_color( wall_map_pos );
#ifdef RAYCAST_FOG
      } else {
         /* Fog. */
         color = RAY_SIDE_EAST_WEST == wall_map_pos->side ? GRAPHICS_COLOR_GRAY : GRAPHICS_COLOR_WHITE;
      }
#endif /* RAYCAST_FOG */
      graphics_draw_line( g, i_x, wall_draw_start, i_x, wall_draw_end, color );
#ifndef RAYCAST_FOG
      }
#endif /* RAYCAST_FOG */
   }

   ret_tmp = mode_pov_draw_floor( &floor_pos, i_x, wall_draw_end, wall_map_pos, layer, &ray, c, g );
   if( TRUE == ret_tmp ) {
      ret_error = TRUE;
   }

cleanup:
   return ret_error;/* Draw the floor from draw_end to the bottom of the screen. */
}

void mode_pov_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {
   GRAPHICS* g = twindow->g;
   struct MOBILE* player = NULL;
   static struct TILEMAP_POSITION last;
   static BOOL draw_failed = FALSE;
   //GRAPHICS_DELTA wall_map_pos; /* The position of the found wall. */
   struct TILEMAP* t = twindow->local_client->active_t;
   struct TILEMAP_LAYER* layer = NULL,
      * opaque_layer = NULL;
   int layer_index = 0,
      layer_max = 0,
      i_x = 0;
   GRAPHICS_DELTA wall_positions[POV_LAYER_LEVEL_MAX][GRAPHICS_SCREEN_WIDTH] = { 0 };
   GRAPHICS_RAY ray = { 0 };

   /* if(
      NULL == twindow->local_client ||
      NULL == twindow->local_client->puppet
   ) {
      memset( &last, '\0', sizeof( struct TILEMAP_POSITION ) );
      return;
   } */

   scaffold_check_null( t );

   player = twindow->local_client->puppet;

   scaffold_check_null( player );

   mode_pov_set_facing( c, player->facing );

#ifdef RAYCAST_CACHE

   if( NULL == ray_view ) {
      graphics_surface_new( ray_view, 0, 0, g->w, g->h );
   }

   if( NULL == c->z_buffer ) {
      c->z_buffer = calloc( g->w, sizeof( double ) );
   }

   if(
      TRUE == draw_failed ||
      player->x != last.x ||
      player->y != last.y ||
      player->facing != last.facing
   ) {
#endif /* RAYCAST_CACHE */

   /* Draw a sky. */
   graphics_draw_rect(
#ifdef RAYCAST_CACHE
         ray_view,
#else
         g,
#endif /* RAYCAST_CACHE */
      0, 0, g->w, g->h, GRAPHICS_COLOR_CYAN, TRUE
   );

   layer_max = vector_count( &(t->layers) );
   for( i_x = 0 ; g->w > i_x ; i_x++ ) {
   for( layer_index = 0 ; layer_index < layer_max ; layer_index++ ) {
      layer = vector_get( &(t->layers), layer_index );
      if( layer_index + 1 >= layer_max || POV_LAYER_LEVEL_INSET == layer_index ) {
         opaque_layer = NULL;
      } else {
         opaque_layer = vector_get( &(t->layers), layer_index + 1 );
      }

      /* Calculate ray position and direction. */
      graphics_raycast_wall_create(
         &ray, i_x, t->width, t->height, &(c->plane_pos), &(c->cam_pos), g );

      draw_failed = mode_pov_update_view(
         wall_positions, i_x, &ray, layer, opaque_layer, c,
#ifdef RAYCAST_CACHE
         ray_view
#else
         g
#endif /* RAYCAST_CACHE */
      );
   }
   }

#ifdef RAYCAST_CACHE
      last.x = player->x;
      last.y = player->y;
      last.facing = player->facing;
   }

   if( FALSE == draw_failed ) {
      /* Draw the cached background. */
      graphics_blit( g, 0, 0, ray_view );
   }

#endif /* RAYCAST_CACHE */

   /* Begin drawing sprites. */
   vector_iterate( &(l->mobiles), mode_pov_mob_calc_dist_cb, twindow );
   vector_iterate( &(l->mobiles), mode_pov_draw_mobile_cb, twindow );

cleanup:
   return;
}

void mode_pov_update(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {
   /* tilemap_update_window_ortho(
      twindow, c->puppet->x, c->puppet->y
   ); */



cleanup:
   return;
}

static BOOL mode_pov_poll_keyboard( struct CLIENT* c, struct INPUT* p ) {
   struct MOBILE* puppet = NULL;
   struct MOBILE_UPDATE_PACKET update = { 0 };
   struct UI* ui = NULL;
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;
   struct CHANNEL* l = NULL;
   struct TILEMAP* t = NULL;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   /* Make sure the buffer that all windows share is available. */
   if(
      NULL == c->puppet ||
      (c->puppet->steps_remaining < -8 || c->puppet->steps_remaining > 8)
   ) {
      /* TODO: Handle limited input while loading. */
      input_clear_buffer( p );
      return FALSE; /* Silently ignore input until animations are done. */
   } else {
      puppet = c->puppet;
      ui = c->ui;
      update.o = puppet;
      update.l = puppet->channel;
      scaffold_check_null( update.l );
      l = puppet->channel;
      scaffold_check_null_msg( l, "No channel loaded." );
      t = &(l->tilemap);
      scaffold_check_null_msg( t, "No tilemap loaded." );
   }

   /* If no windows need input, then move on to game input. */
   switch( p->character ) {
   case INPUT_ASSIGNMENT_QUIT: proto_client_stop( c ); return TRUE;
   case INPUT_ASSIGNMENT_UP:
      update.update = MOBILE_UPDATE_MOVEUP;
      update.x = c->puppet->x;
      update.y = c->puppet->y - 1;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_LEFT:
      update.update = MOBILE_UPDATE_MOVELEFT;
      update.x = c->puppet->x - 1;
      update.y = c->puppet->y;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_DOWN:
      update.update = MOBILE_UPDATE_MOVEDOWN;
      update.x = c->puppet->x;
      update.y = c->puppet->y + 1;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_RIGHT:
      update.update = MOBILE_UPDATE_MOVERIGHT;
      update.x = c->puppet->x + 1;
      update.y = c->puppet->y;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_ATTACK:
      update.update = MOBILE_UPDATE_ATTACK;
      /* TODO: Get attack target. */
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_INV:
      if( NULL == client_input_from_ui ) {
         client_input_from_ui = bfromcstralloc( 80, "" );
         scaffold_check_null( client_input_from_ui );
      }
      ui_window_new(
         ui, win, &str_client_window_id_inv,
         &str_client_window_title_inv, NULL, -1, -1, 600, 380
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, TRUE, FALSE,
         client_input_from_ui, 0, UI_CONST_HEIGHT_FULL, 300,
         UI_CONST_HEIGHT_FULL
      );
      ui_control_add( win, &str_client_control_id_inv_self, control );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, TRUE, FALSE,
         client_input_from_ui, 300, UI_CONST_HEIGHT_FULL, 300,
         UI_CONST_HEIGHT_FULL
      );
      cache = tilemap_get_item_cache( t, puppet->x, puppet->y, TRUE );
      ui_set_inventory_pane_list( control, &(cache->items) );
      ui_control_add( win, &str_client_control_id_inv_ground, control );
      ui_window_push( ui, win );
      return TRUE;

   case '\\':
      if( NULL == client_input_from_ui ) {
         client_input_from_ui = bfromcstralloc( 80, "" );
         scaffold_check_null( client_input_from_ui );
      }
      ui_window_new(
         ui, win, &str_client_window_id_chat,
         &str_client_window_title_chat, NULL, -1, -1, -1, -1
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, TRUE, TRUE,
         client_input_from_ui, -1, -1, -1, -1
      );
      ui_control_add( win, &str_client_control_id_chat, control );
      ui_window_push( ui, win );
      return TRUE;
#ifdef DEBUG_VM
   case 'p': windef_show_repl( ui ); return TRUE;
#endif /* DEBUG_VM */
#ifdef DEBUG_TILES
   case 't':
      if( 0 == p->repeat ) {
         tilemap_toggle_debug_state();
         return TRUE;
      }
      break;
   case 'l':
      if( 0 == p->repeat ) {
         tilemap_dt_layer++;
         return TRUE;
      }
      break;
#endif /* DEBUG_TILES */
   }

cleanup:
   return FALSE;
}

void mode_pov_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p ) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         mode_pov_poll_keyboard( c, p );
      }
   }
   return;
}

void mode_pov_free( struct CLIENT* c ) {
   if(
      TRUE == ipc_is_local_client( c->link ) &&
      HASHMAP_SENTINAL == c->sprites.sentinal
   ) {
      //hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }

   /* if( NULL != ray_view ) {
      graphics_surface_free( ray_view );
      ray_view = NULL;
   } */
}

#endif /* !DISABLE_MODE_POV */
