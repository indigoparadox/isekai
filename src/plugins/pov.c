
#if defined( USE_DYNAMIC_PLUGINS ) || defined( USE_STATIC_MODE_POV )

#include <stdlib.h>

#include <graphics.h>
//#include <vbool.h>
#include "../channel.h"
#include "../mobile.h"
#include <math.h>
#include "../plugin.h"
#include "../client.h"
#include "../action.h"
#include "../proto.h"
#include "../callback.h"

typedef enum {
   POV_LAYER_LEVEL_MAX = 4,
   POV_LAYER_LEVEL_WALL = 3,
   POV_LAYER_LEVEL_RAISED = 2,
   POV_LAYER_LEVEL_NONE = 1,
   POV_LAYER_LEVEL_INSET = 0
} POV_LAYER;

struct POV_CLIENT_DATA {
   GRAPHICS_PLANE cam_pos;
   GRAPHICS_PLANE plane_pos;
   GFX_COORD_FPP* z_buffer;
};

struct POV_MOBILE_DATA {
   double ray_distance;
   bool animation_flipped; /*!< true if looking in - direction in POV. */
   struct TILEMAP_POSITION last_pos;
};

extern bstring client_input_from_ui;

extern struct tagbstring str_client_cache_path;
extern struct tagbstring str_wid_debug_tiles_pos;
extern struct tagbstring str_client_window_id_chat;
extern struct tagbstring str_client_window_title_chat;
extern struct tagbstring str_client_control_id_chat;
extern struct tagbstring str_client_window_id_inv;
extern struct tagbstring str_client_window_title_inv;
extern struct tagbstring str_client_control_id_inv_self;

static struct tagbstring mode_name = bsStatic( "POV" );
static struct tagbstring mode_key = bsStatic( "pov" );

static GRAPHICS* ray_view = NULL;
struct HASHMAP* tileset_status;

PLUGIN_RESULT mode_pov_mobile_init( struct MOBILE* o, struct CHANNEL* l ) {
   mobile_set_mode_data( o, &mode_key, l, mem_alloc( 1, struct POV_MOBILE_DATA ) );
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_pov_client_init( struct CLIENT* c, struct CHANNEL* l ) {
   client_set_mode_data( c, &mode_key, l, mem_alloc( 1, struct POV_CLIENT_DATA ) );
   return PLUGIN_SUCCESS;
}

static void mode_pov_mobile_set_animation( struct MOBILE* o, struct CLIENT* c ) {
   bstring buffer = NULL;
   enum MOBILE_FACING o_facing = MOBILE_FACING_DOWN,
      puppet_facing = MOBILE_FACING_DOWN;
   struct MOBILE* puppet = NULL;
   struct MOBILE_ANI_DEF* o_animation = NULL;
   struct POV_MOBILE_DATA* o_data = NULL;
   struct CHANNEL* l = NULL;

   l = mobile_get_channel( o );
   lgc_null( l );
   o_facing = mobile_get_facing( o );
   o_data = mobile_get_mode_data( o, &mode_key, l );

   puppet = client_get_puppet( c );
   if( NULL == puppet ) {
      goto cleanup;
   }

   puppet_facing = mobile_get_facing( puppet );

   if( mobile_get_animation_reset( o ) || mobile_get_animation_reset( puppet ) ) {
      if( MOBILE_FACING_DOWN == puppet_facing ) {
         o_data->animation_flipped = true;
         switch( o_facing ) {
         case MOBILE_FACING_DOWN:
            o_facing =  MOBILE_FACING_UP;
            break;
         case MOBILE_FACING_UP:
            o_facing = MOBILE_FACING_DOWN;
            break;
         case MOBILE_FACING_LEFT:
            o_facing = MOBILE_FACING_RIGHT;
            break;
         case MOBILE_FACING_RIGHT:
            o_facing = MOBILE_FACING_LEFT;
            break;
         }
      } else if( MOBILE_FACING_LEFT == puppet_facing ) {
         switch( o_facing ) {
         case MOBILE_FACING_DOWN:
            o_facing =  MOBILE_FACING_LEFT;
            break;
         case MOBILE_FACING_UP:
            o_facing = MOBILE_FACING_RIGHT;
            break;
         case MOBILE_FACING_LEFT:
            o_facing = MOBILE_FACING_UP;
            break;
         case MOBILE_FACING_RIGHT:
            o_facing = MOBILE_FACING_DOWN;
            break;
         }
      } else if( MOBILE_FACING_RIGHT == puppet_facing ) {
         o_data->animation_flipped = true;
         switch( o_facing ) {
         case MOBILE_FACING_DOWN:
            o_facing =  MOBILE_FACING_RIGHT;
            break;
         case MOBILE_FACING_UP:
            o_facing = MOBILE_FACING_LEFT;
            break;
         case MOBILE_FACING_LEFT:
            o_facing = MOBILE_FACING_DOWN;
            break;
         case MOBILE_FACING_RIGHT:
            o_facing = MOBILE_FACING_UP;
            break;
         }
      }
   }

   o_animation = mobile_get_animation_current( o );
   if( NULL != o_animation ) {
      buffer = bformat(
         "%s-%s",
         bdata( o_animation->name ),
         str_mobile_facing[o_facing].data
      );
      mobile_set_animation( o, buffer );
   }

   mobile_set_animation_reset( o, false );

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
   struct POV_CLIENT_DATA* c_data = NULL;
   GRAPHICS_PLANE* cam_pos = NULL;
   GRAPHICS_PLANE* plane_pos = NULL;
   struct POV_MOBILE_DATA* o_data = NULL;
   GFX_COORD_FPP* z_buffer = NULL;
   struct CHANNEL* l = NULL;

   if( NULL == mobile_get_sprites( o ) ) {
      goto cleanup;
   }

   l = mobile_get_channel( o );
   lgc_null( l );
   c_data = client_get_mode_data( c, &mode_key, l );
   lgc_null( c_data );
   cam_pos = &(c_data->cam_pos);
   lgc_null( cam_pos );
   plane_pos = &(c_data->plane_pos);
   lgc_null( plane_pos );
   o_data = mobile_get_mode_data( o, &mode_key, l );
   lgc_null( o_data );
   z_buffer = c_data->z_buffer;
   lgc_null( z_buffer );

   /* Translate sprite position to relative to camera. */
   sprite_x = mobile_get_x( o ) - cam_pos->precise_x;
   sprite_y = mobile_get_y( o ) - cam_pos->precise_y;

   /* Transform sprite with the inverse camera matrix:
    *
    * [ plane.x  cam.dir.x ] -1       1 /              [ cam.dir.x  -cam.dir.x ]
    * [                    ] = (plane.x * cam.dir.y -  [                       ]
    * [ plane.y  cam.dir.y ]    cam.dir.x * plane.y) * [ -plane.y   plane.x    ]
    */

   /* Required for correct matrix multiplication. */
   inv_det = 1.0 / (plane_pos->precise_x * cam_pos->facing_y -
      cam_pos->facing_x * plane_pos->precise_y);

   transform_x = inv_det * (cam_pos->facing_y * sprite_x -
      cam_pos->facing_x * sprite_y);
   /* This is actually the depth inside the screen, what Z is in 3D. */
   transform_y = inv_det * (-(plane_pos->precise_y) * sprite_x +
      plane_pos->precise_x * sprite_y);

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

   current_frame = mobile_get_animation_frame_current( o );
   lgc_null( current_frame );
   current_sprite.w = spritesheet.w = mobile_get_sprite_width( o );
   current_sprite.h = spritesheet.h = mobile_get_sprite_height( o );
   mobile_get_spritesheet_pos_ortho(
      o, &current_sprite, current_frame->id
   );

   /* Loop through every vertical stripe of the sprite on screen. */
   for( stripe = draw_rect.x ; stripe < draw_rect.w ; stripe++ ) {

      stepped_x = stripe;
      if(
         MOBILE_FACING_LEFT == mobile_get_facing( o ) &&
            false == o_data->animation_flipped ||
         MOBILE_FACING_RIGHT == mobile_get_facing( o ) &&
            false == o_data->animation_flipped
      ) {
         stepped_x = (stripe - sprite_screen_w) + mobile_get_steps_remaining( o );
      } else {
         stepped_x = (stripe + sprite_screen_w) - mobile_get_steps_remaining( o );
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
         transform_y < z_buffer[stripe]
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
            color = mobile_spritesheet_get_pixel( o, spritesheet.x, spritesheet.y );
            if( GRAPHICS_COLOR_TRANSPARENT != color ) {
               mobile_spritesheet_set_pixel( o, stepped_x, stepped_y, color );
            }
         }
      }
   }

cleanup:
   return;
}

static
void* mode_pov_draw_mobile_cb( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct CLIENT* c = NULL;
   GRAPHICS* g = NULL;

   c = twindow_get_local_client( twindow );
   lgc_null( c );
   g = twindow_get_screen( twindow );
   lgc_null( g );

   if( NULL == o ) {
      goto cleanup; /* Silently. */
   }

   if( mobile_get_animation_reset( o ) ) {
      mode_pov_mobile_set_animation( o, c );
   }
   mobile_animate( o );
   mode_pov_draw_sprite( o, c, g );

cleanup:
   return NULL;
}

static void* mode_pov_mob_calc_dist_cb( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct TILEMAP* t = NULL;
   struct CLIENT* c = NULL;
   GRAPHICS_PLANE* cam_pos = NULL;
   struct POV_CLIENT_DATA* c_data = NULL;
   struct POV_MOBILE_DATA* o_data = NULL;
   struct CHANNEL* l = NULL;

   c = twindow_get_local_client( twindow );
   lgc_null( c );
   t = client_get_tilemap_active( c );
   lgc_null( t );
   l = client_get_channel_active( c );
   lgc_null( l );
   c_data = client_get_mode_data( c, &mode_key, l );
   lgc_null( c_data );
   cam_pos = &(c_data->cam_pos);
   lgc_null( cam_pos );

   lgc_null( t );
   if( NULL == o ) {
      goto cleanup; /* Silently. */
   }

   o_data = mobile_get_mode_data( o, &mode_key, l );
   lgc_null( o_data );

   o_data->ray_distance =
      ((cam_pos->precise_x - mobile_get_x( o )) *
      (cam_pos->precise_x - mobile_get_x( o )) +
         (cam_pos->precise_y - mobile_get_y( o )) *
         (cam_pos->precise_y - mobile_get_y( o )));

cleanup:
   return NULL;
}

#if 0
static void* mode_pov_mob_sort_dist_cb(
   size_t idx, void* iter, void* arg
) {
   //struct MOBILE* m = (struct MOBILE*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct TILEMAP* t = NULL;
   struct CLIENT* c = NULL;

   c = scaffold_container_of( twindow, struct CLIENT, local_window );
   t = c->active_tilemap;

   lgc_null( t );

   /* TODO: Sort by distance. */

cleanup:
   return NULL;
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

static void mode_pov_set_facing( struct CLIENT* c, MOBILE_FACING facing ) {
   double old_dir_x = 0;
   GRAPHICS_PLANE* cam_pos = NULL;
   GRAPHICS_PLANE* plane_pos = NULL;
   struct POV_CLIENT_DATA* c_data = NULL;
   struct CHANNEL* l = NULL;

   l = client_get_channel_active( c );
   lgc_null( l );
   c_data = client_get_mode_data( c, &mode_key, l );
   lgc_null( c_data );
   cam_pos = &(c_data->cam_pos);
   lgc_null( cam_pos );
   plane_pos = &(c_data->plane_pos);
   lgc_null( plane_pos );

   switch( facing ) {
      case MOBILE_FACING_LEFT:
         old_dir_x = 0;
         cam_pos->facing_x = old_dir_x * cos( GRAPHICS_RAY_ROTATE_INC ) -
            -1 * sin( GRAPHICS_RAY_ROTATE_INC );
         cam_pos->facing_y = old_dir_x * sin( GRAPHICS_RAY_ROTATE_INC ) +
            -1 * cos( GRAPHICS_RAY_ROTATE_INC );

         old_dir_x = GRAPHICS_RAY_FOV;
         plane_pos->precise_x = old_dir_x * cos( GRAPHICS_RAY_ROTATE_INC ) -
            0 * sin( GRAPHICS_RAY_ROTATE_INC );
         plane_pos->precise_y = old_dir_x * sin( GRAPHICS_RAY_ROTATE_INC ) +
            0 * cos( GRAPHICS_RAY_ROTATE_INC );
         break;

      case MOBILE_FACING_RIGHT:
         old_dir_x = 0;
         cam_pos->facing_x = old_dir_x * cos( -GRAPHICS_RAY_ROTATE_INC ) -
            -1 * sin( -GRAPHICS_RAY_ROTATE_INC );
         cam_pos->facing_y = old_dir_x * sin( -GRAPHICS_RAY_ROTATE_INC ) +
            -1 * cos( -GRAPHICS_RAY_ROTATE_INC );

         old_dir_x = GRAPHICS_RAY_FOV;
         plane_pos->precise_x = old_dir_x * cos(-GRAPHICS_RAY_ROTATE_INC ) -
            0 * sin( -GRAPHICS_RAY_ROTATE_INC );
         plane_pos->precise_y = old_dir_x * sin( -GRAPHICS_RAY_ROTATE_INC ) +
            0 * cos( -GRAPHICS_RAY_ROTATE_INC );
         break;

      case MOBILE_FACING_UP:
         cam_pos->facing_x = 0; /* TODO */
         cam_pos->facing_y = -1;
         plane_pos->precise_x = GRAPHICS_RAY_FOV;
         plane_pos->precise_y = 0;
         break;

      case MOBILE_FACING_DOWN:
         cam_pos->facing_x = 0; /* TODO */
         cam_pos->facing_y = 1;
         plane_pos->precise_x = -GRAPHICS_RAY_FOV;
         plane_pos->precise_y = 0;
         break;
   }

cleanup:
   return;
}

static bool mode_pov_draw_floor(
   GFX_RAY_FLOOR* floor_pos,
   GFX_COORD_PIXEL i_x,
   GFX_COORD_PIXEL above_wall_draw_end,
   GFX_COORD_PIXEL below_wall_draw_start,
   GFX_COORD_PIXEL below_wall_height,
   const GRAPHICS_DELTA* wall_map_pos,
   const struct TILEMAP_LAYER* layer,
   const GRAPHICS_RAY* ray, struct CLIENT* c, GRAPHICS* g
) {
   GFX_COORD_PIXEL i_y = 0;
   struct TILEMAP_TILESET* set = NULL;
   struct TILEMAP* t = NULL;
   size_t set_firstgid = 0;
   GRAPHICS* g_tileset = NULL;
   bool ret_error = false;
   GRAPHICS_RECT tile_tilesheet_pos = { 0 };
   int tex_x = 0,
      tex_y = 0;
   GRAPHICS_COLOR color = GRAPHICS_COLOR_TRANSPARENT;
   uint32_t tile = 0;
   GRAPHICS_PLANE* cam_pos = NULL;
   GRAPHICS_PLANE* plane_pos = NULL;
   struct POV_CLIENT_DATA* c_data = NULL;
   struct TWINDOW* w = NULL;
   struct CHANNEL* l = NULL;

   l = client_get_channel_active( c );
   lgc_null( l );
   c_data = client_get_mode_data( c, &mode_key, l );
   lgc_null( c_data );
   cam_pos = &(c_data->cam_pos);
   lgc_null( cam_pos );
   plane_pos = &(c_data->plane_pos);
   lgc_null( plane_pos );
   w = client_get_local_window( c );
   lgc_null( w );

   t = client_get_tilemap_active( c );
   if( NULL == t ) {
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
   if( 0 > above_wall_draw_end ) {
      above_wall_draw_end = g->h - 1;
   }
   for( i_y = above_wall_draw_end ; i_y < below_wall_draw_start ; i_y++ ) {
      graphics_floorcast_throw(
         floor_pos, i_x, i_y, below_wall_height,
         cam_pos, wall_map_pos, ray,
         twindow_get_grid_w( w ), twindow_get_grid_h( w ),
         g
      );

      /* Ensure we have everything needed to draw the tile. */
      tile = tilemap_layer_get_tile_gid( layer, (int)floor_pos->x, (int)floor_pos->y  );
      if( 0 == tile ) {
         continue;
      }

      set = tilemap_get_tileset( t, tile, &set_firstgid );
      if( NULL == set ) {
         continue;
      }

      g_tileset = tilemap_tileset_get_image_default( set, c );
      if( NULL == g_tileset ) {
         /* Tileset not yet loaded, so fail gracefully. */
         ret_error = true;
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
 * \return true on error, false otherwise.
 *
 */
static bool mode_pov_update_view(
   GFX_COORD_PIXEL i_x, int layer_max, int layer_pov, GRAPHICS_RAY* ray,
   GFX_COORD_PIXEL prev_wall_top, GFX_COORD_PIXEL prev_wall_height,
   struct CLIENT* c, GRAPHICS* g
) {
   GFX_RAY_FLOOR floor_pos = { 0 };
   uint32_t tile = 0;
   GRAPHICS_COLOR color = GRAPHICS_COLOR_TRANSPARENT;
   bool wall_hit = false;
   bool ret_error = false,
      ret_tmp = false;
   GRAPHICS_DELTA wall_map_pos = { 0 };
   struct TILEMAP* t = NULL;
   GFX_COORD_PIXEL cell_height = 0,
      cell_height_qtr = 0,
      wall_draw_bottom = 0,
      wall_draw_top = 0;
   struct TILEMAP_LAYER* layer = NULL,
      * opaque_layer = NULL;
   int layer_index = 0,
      opaque_index = 0,
      pov_incr = 0;
   bool recurse = true;
   struct POV_CLIENT_DATA* c_data = NULL;
   struct CHANNEL* l = NULL;

   t = client_get_tilemap_active( c );
   lgc_null( t );
   l = tilemap_get_channel( t );
   lgc_null( l );
   c_data = client_get_mode_data( c, &mode_key, l );
   lgc_null( c_data );
   if( NULL == c_data->z_buffer ) {
      c_data->z_buffer = mem_alloc( graphics_surface_get_w( g ), GFX_COORD_FPP );
   }

   /* Do the actual casting. */
   wall_hit = false;
   while( false == wall_hit ) {
      graphics_raycast_wall_iterate( &wall_map_pos, ray, prev_wall_height, g );

      if( graphics_raycast_point_is_infinite( &wall_map_pos ) ) {
         /* The ray has to stop at some point, or this will become an
          * infinite loop! */
         recurse = false;
         break;
      }

      for( layer_index = layer_pov ; layer_max > layer_index ; layer_index++ ) {
         /* Detect if the opaque layer exists. */
         opaque_index = layer_index + 1;
         layer = vector_get( t->layers, layer_index );
         if( opaque_index >= layer_max || POV_LAYER_LEVEL_INSET == layer_index ) {
            opaque_layer = NULL;
         } else {
            opaque_layer = vector_get( t->layers, opaque_index );
         }

         /* Detect raised terrain as a wall hit. */
         if( NULL != opaque_layer ) {
            tile = tilemap_layer_get_tile_gid(
               opaque_layer, wall_map_pos.map_x, wall_map_pos.map_y );
            if( 0 != tile ) {
               wall_hit = true;
               if( POV_LAYER_LEVEL_WALL <= opaque_index ) {
                  recurse = false;
               }
               break;
            }
         }
      }
   }

   c_data->z_buffer[i_x] = wall_map_pos.perpen_dist;

   cell_height = (int)(g->h / wall_map_pos.perpen_dist);

   wall_draw_bottom = (cell_height / 2) + (g->h / 2);
   if( wall_draw_bottom >= g->h ) {
      wall_draw_bottom = g->h - 1;
   }

   if( POV_LAYER_LEVEL_RAISED == opaque_index ) {
      cell_height /= 4;
      pov_incr = 1;
   }
   wall_draw_top = wall_draw_bottom - cell_height;
   if( 0 > wall_draw_top ) {
      wall_draw_top = 0;
   }

   if( recurse ) {
      ret_tmp = mode_pov_update_view(
         i_x, layer_max, layer_pov + pov_incr, ray, wall_draw_top, cell_height, c, g );
      if( false != ret_tmp ) {
         ret_error = true;
         goto cleanup;
      }
   }

   /* Draw the pixels of the stripe as a vertical line. */
   if( 0 < cell_height ) {
      if( false != wall_hit ) {
         /* Choose wall color. */
         color = get_wall_color( &wall_map_pos );
#ifdef RAYCAST_FOG
      } else {
         /* Fog. */
         color = RAY_SIDE_EAST_WEST == wall_map_pos->side ? GRAPHICS_COLOR_GRAY : GRAPHICS_COLOR_WHITE;
      }
#endif /* RAYCAST_FOG */
      graphics_draw_line( g, i_x, wall_draw_top, i_x, wall_draw_bottom, color );
#ifndef RAYCAST_FOG
      }
#endif /* !RAYCAST_FOG */
   }

   for( layer_index = 0 ; layer_index < layer_max ; layer_index++ ) {
      layer = vector_get( t->layers, layer_index );
      ret_tmp = mode_pov_draw_floor(
         &floor_pos, i_x, wall_draw_bottom, prev_wall_top, prev_wall_height,
         &wall_map_pos, layer, ray, c, g );
      if( false != ret_tmp ) {
         ret_error = true;
      }
   }

cleanup:
   return ret_error;
}

PLUGIN_RESULT mode_pov_draw(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   GRAPHICS* g = NULL;
   struct MOBILE* player = NULL;
   static bool draw_failed = false;
   struct TILEMAP* t = NULL;
   int i_x = 0,
      layer_max = 0,
      i_layer = 0;
   GRAPHICS_RAY ray = { 0 };
   struct TILEMAP_LAYER* layer_player = NULL;
   uint32_t tile_player = 0;
   struct TWINDOW* twindow = NULL;
   struct POV_MOBILE_DATA* o_data = NULL;
   struct POV_CLIENT_DATA* c_data = NULL;
   GRAPHICS_PLANE* cam_pos = NULL;
   GRAPHICS_PLANE* plane_pos = NULL;

   twindow = client_get_local_window( c );
   lgc_null( twindow );
   g = twindow_get_screen( twindow );
   lgc_null( g );
   t = client_get_tilemap_active( c );
   lgc_null( t );
   player = client_get_puppet( c );
   lgc_null( player );
   c_data = client_get_mode_data( c, &mode_key, l );
   //assert( NULL != c_data );
   if( NULL == c_data ) { goto cleanup; }
   lgc_null( c_data );
   o_data = mobile_get_mode_data( player, &mode_key, l );
   if( NULL == o_data ) { goto cleanup; }
   lgc_null( o_data );
   cam_pos = &(c_data->cam_pos);
   plane_pos = &(c_data->plane_pos);

   mode_pov_set_facing( c, mobile_get_facing( player ) );

#ifdef RAYCAST_CACHE

   if( NULL == ray_view ) {
      graphics_surface_new( ray_view, 0, 0, g->w, g->h );
   }

   if(
      draw_failed ||
      mobile_get_x( player ) != o_data->last_pos.x ||
      mobile_get_y( player ) != o_data->last_pos.y ||
      mobile_get_facing( player ) != o_data->last_pos.facing
   ) {
#endif /* RAYCAST_CACHE */

   /* Draw a sky. */
   graphics_draw_rect(
#ifdef RAYCAST_CACHE
         ray_view,
#else
         g,
#endif /* RAYCAST_CACHE */
      0, 0, g->w, g->h, GRAPHICS_COLOR_CYAN, true
   );

   layer_max = vector_count( t->layers );
   for( i_layer = layer_max - 1 ; 0 <= i_layer ; i_layer-- ) {
      layer_player = vector_get( t->layers, i_layer );
      tile_player = tilemap_layer_get_tile_gid( layer_player,
         mobile_get_x( player ), mobile_get_y( player ) );
      if( 0 != tile_player ) {
         /* Found highest non-empty layer presently under player. */
         break;
      }
   }

   /* Cast a ray on each vertical line in the canvas. */
   for( i_x = 0 ; g->w > i_x ; i_x++ ) {
      /* Calculate ray position and direction. */
      graphics_raycast_wall_create(
         &ray, i_x, t->width, t->height, plane_pos, cam_pos, g );

      draw_failed = mode_pov_update_view(
         i_x, layer_max, i_layer, &ray, g->h, 0, c,
#ifdef RAYCAST_CACHE
         ray_view
#else
         g
#endif /* RAYCAST_CACHE */
      );
   }

#ifdef RAYCAST_CACHE
      o_data->last_pos.x = mobile_get_x( player );
      o_data->last_pos.y = mobile_get_y( player );
      o_data->last_pos.facing = mobile_get_facing( player );
   }

   if( !draw_failed ) {
      /* Draw the cached background. */
      graphics_blit( g, 0, 0, ray_view );
   }

#endif /* RAYCAST_CACHE */

   /* Begin drawing sprites. */
   vector_iterate( l->mobiles, mode_pov_mob_calc_dist_cb, twindow );
   vector_iterate( l->mobiles, mode_pov_draw_mobile_cb, twindow );

cleanup:
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_pov_update(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   /* tilemap_update_window_ortho(
      twindow, c->puppet->x, c->puppet->y
   ); */

//cleanup:
   return PLUGIN_SUCCESS;
}

static bool mode_pov_poll_keyboard( struct CLIENT* c, struct INPUT* p ) {
   struct MOBILE* puppet = NULL;
   struct ACTION_PACKET* update = NULL;
   struct UI* ui = NULL;
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;
   struct CHANNEL* l = NULL;
#ifdef USE_ITEMS
   struct TILEMAP* t = NULL;
   struct TILEMAP_ITEM_CACHE* cache = NULL;
#endif // USE_ITEMS

   puppet = client_get_puppet( c );

   /* Make sure the buffer that all windows share is available. */
   if(
      NULL == puppet ||
      (mobile_get_steps_remaining( puppet ) < -8 ||
      mobile_get_steps_remaining( puppet ) > 8)
   ) {
      /* TODO: Handle limited input while loading. */
      input_clear_buffer( p );
      return false; /* Silently ignore input until animations are done. */
   } else {
      l = mobile_get_channel( puppet );
      lgc_null( l );
      update = action_packet_new( l, puppet, ACTION_OP_NONE, 0, 0, NULL );
   }

   /* If no windows need input, then move on to game input. */
   switch( p->character ) {
   case INPUT_ASSIGNMENT_QUIT: proto_client_stop( c ); return true;
   case INPUT_ASSIGNMENT_UP:
      action_packet_set_op( update, ACTION_OP_MOVEUP );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) - 1 );
      proto_client_send_update( c, update );
      return true;

   case INPUT_ASSIGNMENT_LEFT:
      action_packet_set_op( update, ACTION_OP_MOVELEFT );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) - 1 );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) );
      proto_client_send_update( c, update );
      return true;

   case INPUT_ASSIGNMENT_DOWN:
      action_packet_set_op( update, ACTION_OP_MOVEDOWN );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) + 1 );
      proto_client_send_update( c, update );
      return true;

   case INPUT_ASSIGNMENT_RIGHT:
      action_packet_set_op( update, ACTION_OP_MOVERIGHT );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) + 1 );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) );
      proto_client_send_update( c, update );
      return true;

   case INPUT_ASSIGNMENT_ATTACK:
      action_packet_set_op( update, ACTION_OP_ATTACK );
      /* TODO: Get attack target. */
      proto_client_send_update( c, update );
      return true;

#ifdef USE_ITEMS
   case INPUT_ASSIGNMENT_INV:
      if( NULL == client_input_from_ui ) {
         client_input_from_ui = bfromcstralloc( 80, "" );
         lgc_null( client_input_from_ui );
      }
      ui_window_new(
         ui, win, &str_client_window_id_inv,
         &str_client_window_title_inv, NULL, -1, -1, 600, 380
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, true, false,
         client_input_from_ui, 0, UI_CONST_HEIGHT_FULL, 300,
         UI_CONST_HEIGHT_FULL
      );
      ui_control_add( win, &str_client_control_id_inv_self, control );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, true, false,
         client_input_from_ui, 300, UI_CONST_HEIGHT_FULL, 300,
         UI_CONST_HEIGHT_FULL
      );
      cache = tilemap_get_item_cache( t, puppet->x, puppet->y, true );
      ui_set_inventory_pane_list( control, &(cache->items) );
      ui_control_add( win, &str_client_control_id_inv_ground, control );
      ui_window_push( ui, win );
      return true;
#endif // USE_ITEMS

   case '\\':
      if( NULL == client_input_from_ui ) {
         client_input_from_ui = bfromcstralloc( 80, "" );
         lgc_null( client_input_from_ui );
      }
      ui_window_new(
         ui, win, &str_client_window_id_chat,
         &str_client_window_title_chat, NULL, -1, -1, -1, -1
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, true, true,
         client_input_from_ui, -1, -1, -1, -1
      );
      ui_control_add( win, &str_client_control_id_chat, control );
      ui_window_push( ui, win );
      return true;
#ifdef DEBUG_VM
   case 'p': windef_show_repl( ui ); return true;
#endif /* DEBUG_VM */
#ifdef DEBUG_TILES
   case 't':
      if( 0 == p->repeat ) {
         tilemap_toggle_debug_state();
         return true;
      }
      break;
   case 'l':
      if( 0 == p->repeat ) {
         tilemap_dt_layer++;
         return true;
      }
      break;
#endif /* DEBUG_TILES */
   }

cleanup:
   return false;
}

PLUGIN_RESULT mode_pov_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p ) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         mode_pov_poll_keyboard( c, p );
      }
   }
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_pov_free( struct CLIENT* c ) {
   if(
      client_is_local( c ) //&&
      //hashmap_is_valid( &(c->sprites) )
   ) {
      //hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }

   /* if( NULL != ray_view ) {
      graphics_surface_free( ray_view );
      ray_view = NULL;
   } */
   return PLUGIN_SUCCESS;
}

/* Mobile action chain of events is roughly:
 *
 * (Client-Side) Keyboard > Protocol Message > Server Action
 *
 * During the server action, the movement is either determined to be blocked
 * (and subsequently ignored) or allowed and propogated to all clients
 * (including the one that initiated the action).
 *
 * Having the plugin decide how the chain of actions happens theoretically
 * leaves the door open for plugins that are "smarter" at handling bad network
 * conditions later on (e.g. by predicting movements client-side).
 */

PLUGIN_RESULT mode_pov_mobile_action_client( struct ACTION_PACKET* update ) {
   struct MOBILE* o = NULL;
   GFX_COORD_PIXEL steps_increment = 0;
   struct TILEMAP* t = NULL;

   scaffold_assert_client();

   o = action_packet_get_mobile( update );
   lgc_null( o );
   t = mobile_get_tilemap( o );
   lgc_null( t );

   steps_increment = mobile_calculate_terrain_steps_inc(
      t,
      mobile_get_steps_inc_default( o ),
      action_packet_get_tile_x( update ),
      action_packet_get_tile_y( update )
   );

   lg_debug(
      __FILE__, "Mobile %d walking with speed: %d\n",
      mobile_get_serial( o ), steps_increment
   );

   mobile_walk(
      o,
      action_packet_get_tile_x( update ),
      action_packet_get_tile_y( update ),
      steps_increment
   );

cleanup:
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_pov_mobile_action_server( struct ACTION_PACKET* update ) {
   struct MOBILE* o = NULL;
   struct CHANNEL* l = NULL;
   GFX_COORD_PIXEL steps_increment = 0;
   struct TILEMAP* t = NULL;
   TILEMAP_COORD_TILE dest_x = 0,
      dest_y = 0;

// XXX: Call movement stuff.

   /* Check for valid movement range. */
   /* if(
      dest_x != start_x - 1 &&
      dest_x != start_x + 1 &&
      dest_y != start_y - 1 &&
      dest_y != start_y + 1
   ) {
      lg_error( __FILE__,
         "Mobile (%d) attempted to walk invalid distance: %d, %d to %d, %d\n",
         mobile_get_serial( o ), start_x, start_y, dest_x, dest_y
      );
      goto cleanup;
   } */

   o = action_packet_get_mobile( update );
   lgc_null( o );
   t = mobile_get_tilemap( o );
   lgc_null( t );

   dest_x = action_packet_get_tile_x( update );
   dest_y = action_packet_get_tile_y( update );

   steps_increment = mobile_calculate_terrain_steps_inc(
      t, mobile_get_steps_inc_default( o ), dest_x, dest_y );

   /* Check for blockers. */
   if(
      0 >= steps_increment ||
      !mobile_calculate_mobile_collision(
         o,
         mobile_get_x( o ), mobile_get_y( o ),
         dest_x, dest_y
      )
   ) {
      lg_error( __FILE__,
         "Mobile walking blocked: %d, %d to %d, %d\n",
         mobile_get_x( o ),  mobile_get_y( o ), dest_x, dest_y
      );
      goto cleanup;
   }

   scaffold_assert_server();
   o = action_packet_get_mobile( update );
   /* TODO: Check obstacles. */
   mobile_set_x( o, action_packet_get_tile_x( update ) );
   mobile_set_y( o, action_packet_get_tile_y( update ) );
   mobile_set_prev_x( o, action_packet_get_tile_x( update ) );
   mobile_set_prev_y( o, action_packet_get_tile_y( update ) );

   l = action_packet_get_channel( update );
   hashmap_iterate( l->clients, callback_send_updates_to_client, update );

cleanup:
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_pov_init() {
}

PLUGIN_RESULT mode_pov_client_free( struct CLIENT* c ) {
}

PLUGIN_RESULT mode_pov_mobile_free( struct MOBILE* o ) {
}

#endif /* USE_DYNAMIC_PLUGINS || USE_STATIC_MODE_POV */
