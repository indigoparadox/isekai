
#define MODE_C
#include "../mode.h"

#ifdef USE_RAYCASTING

#include <stdlib.h>

#include "../callback.h"
#include "../ui.h"
#include "../ipc.h"
#include "../channel.h"
#include "../tilemap.h"
#include "../proto.h"
#include "../mobile.h"

extern bstring client_input_from_ui;

static GFX_RAY_WALL* cl_walls = NULL;
static GFX_RAY_FLOOR* cl_floors = NULL;
static GRAPHICS* ray_view = NULL;

static void* mode_pov_raycol_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct GFX_RAY_WALL* pos = (struct GFX_RAY_WALL*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP* t = twindow->t;
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   int i;

   scaffold_check_null( t );



cleanup:
   return NULL;
}

static BOOL check_ray_wall_collision(
   GFX_RAY_WALL* wall_map_pos, const struct TILEMAP_LAYER* layer,
   const struct GRAPHICS_TILE_WINDOW* twindow
) {
   int res = 0,
      i;
   uint32_t tile;
   struct TILEMAP_TILESET* set;
   struct TILEMAP_TILE_DATA* tile_info;
   struct TILEMAP_TERRAIN_DATA* terrain_iter;

   tile = tilemap_get_tile( layer, wall_map_pos->x, wall_map_pos->y );
   if( 0 == tile ) {
      goto cleanup;
   }

   set = tilemap_get_tileset( twindow->t, tile, NULL );
   if( NULL == set ) {
      goto cleanup;
   }

   tile_info = vector_get( &(set->tiles), tile - 1 );
   for( i = 0 ; 4 > i ; i++ ) {
      terrain_iter = tile_info->terrain[i];
      if( NULL == terrain_iter ) { continue; }
      if( TILEMAP_MOVEMENT_BLOCK == terrain_iter->movement ) {
         res = 1;
         goto cleanup;
      }
   }

   //res = vector_iterate( &(twindow->t->layers), mode_pov_raycol_cb, twindow );

cleanup:
   return res;
}

static GRAPHICS_COLOR get_wall_color( GFX_RAY_WALL* wall_pos ) {
//#ifdef TECHNICOLOR_RAYS
   switch( (rand() + 1) % 4 ) {
//#endif /* TECHNICOLOR_RAYS */
//   switch( worldMap[x][y] ) {
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
}

/** \brief Create the first-person view and walls to draw on screen beneath the
 *         sprites.
 *
 * \param
 * \param
 * \return
 *
 */
static void mode_pov_update_view(
   GRAPHICS* g, int x, int y, MOBILE_FACING facing, struct TILEMAP* t,
   struct CLIENT* c
) {
   int i_x, i_y, draw_start, draw_end, j, k;
   struct INPUT p;
   int done = 0;
   GFX_DELTA cam_pos;
   GFX_DELTA plane_pos;
   GFX_RAY_WALL wall_map_pos; /* The position of the found wall. */
   GFX_RAY ray;
   //GFX_DELTA* wall_scr_pos;
   GFX_RAY_FLOOR floor_pos;
   int line_height;
   struct TILEMAP_TILESET* set;
   struct TILEMAP_LAYER* layer;
   uint32_t tile;
   GRAPHICS* g_tileset;
   GRAPHICS_RECT tile_tilesheet_pos;
   SCAFFOLD_SIZE set_firstgid = 0;
   GRAPHICS_COLOR color;
   double steps_remaining = 0;
   int tex_x, tex_y;
   int layer_index = 0,
      layer_max = 0;
   BOOL wall_hit = FALSE;

   /*
   static BOOL pos_dirty = TRUE;
   static GRAPHICS_RECT former_pos;
*/

   if( NULL == t ) {
      return;
   }

   cam_pos.x = x;
   cam_pos.y = y;

#if 0
   if( pos_dirty ) {
      memset( &former_pos, '\0', sizeof( struct TILEMAP_POSITION ) );
      pos_dirty = FALSE;
   }

   if( former_pos.x ) {

   }

   if( 0 < player->steps_remaining ) {
      switch( player->facing )  {
         case MOBILE_FACING_LEFT:
            steps_remaining = 10 / player->steps_remaining;
            cam_pos.x -= steps_remaining;
            break;
         case MOBILE_FACING_RIGHT:
            steps_remaining = 10 - (10 / player->steps_remaining);
            cam_pos.x += steps_remaining;
            break;
         case MOBILE_FACING_DOWN:
            steps_remaining = 10 - (10 / player->steps_remaining);
            cam_pos.y += steps_remaining;
            break;
         case MOBILE_FACING_UP:
            steps_remaining = 10 / player->steps_remaining;
            cam_pos.y -= steps_remaining;
            break;
      }
   }
#endif // 0

   /*if( NULL == twindow->z_buffer ) {
      twindow->z_buffer = calloc( twindow->g->w, sizeof( double ) );
   }*/

   wall_map_pos.map_w = t->width;
   wall_map_pos.map_h = t->height;
   switch( facing ) {
      case MOBILE_FACING_LEFT:
         cam_pos.dx.facing = -1; /* TODO */
         cam_pos.dy.facing = 0;
         plane_pos.x = 0;
         plane_pos.y = 0.66;
         break;
      case MOBILE_FACING_RIGHT:
         cam_pos.dx.facing = 1; /* TODO */
         cam_pos.dy.facing = 0;
         plane_pos.x = 0;
         plane_pos.y = -0.66;
         break;
      case MOBILE_FACING_UP:
         cam_pos.dx.facing = 0; /* TODO */
         cam_pos.dy.facing = -1;
         plane_pos.x = 0.66;
         plane_pos.y = 0;
         break;
      case MOBILE_FACING_DOWN:
         cam_pos.dx.facing = 0; /* TODO */
         cam_pos.dy.facing = 1;
         plane_pos.x = -0.66;
         plane_pos.y = 0;
         break;
   }
   floor_pos.tex_w = 32; /* TODO: Get this dynamically. */
   floor_pos.tex_h = 32;

   /* Draw a sky. */
   graphics_draw_rect( g, 0, 0, g->w, g->h, GRAPHICS_COLOR_CYAN, TRUE );

   layer_max = vector_count( &(t->layers) );
   for( layer_index = 0 ; layer_index < 3 ; layer_index++ ) {
      layer = vector_get( &(t->layers), layer_index );

      for( i_x = 0; i_x < g->w; i_x++ ) {

         wall_map_pos.x = (int)cam_pos.x;
         wall_map_pos.y = (int)cam_pos.y;

         /* Calculate ray position and direction. */
         graphics_raycast_wall_create( &ray, i_x, &wall_map_pos, &plane_pos, &cam_pos, g );

         /* Do the actual casting. */
         wall_hit = FALSE;
         while( FALSE == wall_hit ) {
            graphics_raycast_wall_iter( &wall_map_pos, &ray );

            if( ray.infinite_dist ) {
               if(
                  MOBILE_FACING_LEFT == facing ||
                  MOBILE_FACING_RIGHT == facing
               ) {
                  ray.side_dist_x += ray.delta_dist_x;
                  wall_map_pos.x += ray.step_x;
                  wall_map_pos.side = 0;
               }

               break;
            }

            /* Check if ray has hit a wall. */
            /* if( check_ray_wall_collision( &wall_map_pos, layer, twindow ) ) {
               wall_hit = TRUE;
            } */
            if( 0 != tile && 3 <= layer_index ) {
               wall_hit = TRUE;
            }
         }

         if( 0 == wall_map_pos.side ) {
            wall_map_pos.perpen_dist =
               (wall_map_pos.x - cam_pos.x + (-1 - ray.step_x) / 2) / ray.direction_x;
         } else {
            wall_map_pos.perpen_dist =
               (wall_map_pos.y - cam_pos.y + (-1 - ray.step_y) / 2) / ray.direction_y;
         }

         /* wall_map_pos.perpen_dist =
            graphics_raycast_get_distance( &wall_map_pos, &cam_pos, &ray ); */
         //twindow->z_buffer[i_x] = wall_map_pos.perpen_dist;
         line_height = (int)(g->h / wall_map_pos.perpen_dist);

         draw_end = graphics_get_ray_stripe_end( line_height, g );
         draw_start = graphics_get_ray_stripe_start( line_height, g );

         /* Draw the pixels of the stripe as a vertical line. */
         if( 0 == layer_index && 0 < line_height ) {
            if( TRUE != ray.infinite_dist ) {
               /* Choose wall color. */
               color = get_wall_color( &wall_map_pos );
#ifdef RAY_FOG
            } else {
               /* Fog. */
               color = GRAPHICS_COLOR_WHITE;
            }
#endif /* RAY_FOG */
            graphics_draw_line( g, i_x, draw_start, i_x, draw_end, color );
#ifndef RAY_FOG
            }
#endif /* RAY_FOG */
         }

         graphics_floorcast_create(
            &floor_pos, &ray, i_x, &cam_pos,
            &wall_map_pos, g
         );

         if( 0 > draw_end ) {
            /* Become < 0 if the integer overflows. */
            draw_end = g->h;
         }

         /* Draw the floor from draw_end to the bottom of the screen. */
         for( i_y = draw_end +1; i_y < g->h; i_y++ ) {
            graphics_floorcast_throw(
               &floor_pos, i_x, i_y, line_height,
               &cam_pos, &wall_map_pos,
               g
            );

            /* Ensure we have everything needed to draw the tile. */
            tile = tilemap_get_tile( layer, (int)floor_pos.x, (int)floor_pos.y  );
            if( 0 > tile ) {
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
               continue;
            }

            /* Move the source region on the tilesheet. */
            tilemap_get_tile_tileset_pos(
               set, set_firstgid, g_tileset, tile, &tile_tilesheet_pos );
            tex_x = floor_pos.tex_x + tile_tilesheet_pos.x;
            tex_y = floor_pos.tex_y + tile_tilesheet_pos.y;
            color = graphics_get_pixel( g_tileset, tex_x, tex_y );
            if( GRAPHICS_COLOR_TRANSPARENT != color ) {
               graphics_set_pixel( g, i_x, i_y, color );
            }
         }
      }
   }
}

void mode_pov_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {
   GRAPHICS* g = twindow->g;
   struct MOBILE* player = NULL;
   static struct TILEMAP_POSITION last;

   if(
      NULL == twindow->local_client ||
      NULL == twindow->local_client->puppet
   ) {
      memset( &last, '\0', sizeof( struct TILEMAP_POSITION ) );
      return;
   }

   player = twindow->local_client->puppet;

   if( NULL == ray_view ) {
      graphics_surface_new( ray_view, 0, 0, g->w, g->h );
   }

   if(
      player->x != last.x ||
      player->y != last.y ||
      player->facing != last.facing
   ) {
      mode_pov_update_view(
         ray_view, player->x, player->y, player->facing, twindow->t, twindow->local_client
      );
      last.x = player->x;
      last.y = player->y;
      last.facing = player->facing;
   }

   graphics_blit( g, 0, 0, ray_view );

   vector_iterate( &(l->mobiles), callback_draw_mobiles, twindow );

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
   struct MOBILE_UPDATE_PACKET update;
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
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, TRUE, client_input_from_ui,
         0, UI_CONST_HEIGHT_FULL, 300, UI_CONST_HEIGHT_FULL
      );
      ui_control_add( win, &str_client_control_id_inv_self, control );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, TRUE, client_input_from_ui,
         300, UI_CONST_HEIGHT_FULL, 300, UI_CONST_HEIGHT_FULL
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
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, TRUE, client_input_from_ui,
         -1, -1, -1, -1
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
      hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }

   if( NULL != ray_view ) {
      graphics_surface_free( ray_view );
      ray_view = NULL;
   }
}

#endif /* USE_RAYCASTING */
