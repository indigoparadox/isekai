
#define CLIENT_LOCAL_C
#include "../client.h"

#include <stdlib.h>

#include "../callback.h"
#include "../ui.h"
#include "../ipc.h"
#include "../channel.h"
#include "../tilemap.h"
#include "../proto.h"
#include "../mobile.h"

extern bstring client_input_from_ui;

static void* client_local_raylay_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {

}

static void* client_local_raycol_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
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

   //res = vector_iterate( &(twindow->t->layers), client_local_raycol_cb, twindow );

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

void client_local_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {
   int i_x, i_y, draw_start, draw_end;
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
   struct MOBILE* player;
   int tex_x, tex_y;
   int layer_index = 0,
      layer_max = 0;
   BOOL wall_hit = FALSE;

   if( NULL == twindow->t ) {
      return;
   }

   if(
      NULL == twindow->local_client ||
      NULL == twindow->local_client->puppet
   ) {
      return;
   }

   player = twindow->local_client->puppet;
   cam_pos.x = player->x;
   cam_pos.y = player->y;

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

   wall_map_pos.map_w = twindow->t->width;
   wall_map_pos.map_h = twindow->t->height;
   cam_pos.dx.facing = -1; /* TODO */
   cam_pos.dy.facing = 0;
   plane_pos.x = 0;
   plane_pos.y = 0.66;
   floor_pos.tex_w = 32; /* TODO: Get this dynamically. */
   floor_pos.tex_h = 32;

   /* Draw a sky. */
   graphics_draw_rect( twindow->g, 0, 0, twindow->g->w, twindow->g->h, GRAPHICS_COLOR_CYAN, TRUE );

   layer_max = vector_count( &(twindow->t->layers) );
   for( layer_index = 0 ; layer_index < layer_max ; layer_index++ ) {
      layer = vector_get( &(twindow->t->layers), layer_index );

      for( i_x = 0; i_x < twindow->g->w; i_x++ ) {

         wall_map_pos.x = (int)cam_pos.x;
         wall_map_pos.y = (int)cam_pos.y;

         /* Calculate ray position and direction. */
         graphics_raycast_wall_create( &ray, i_x, &wall_map_pos, &plane_pos, &cam_pos, twindow->g );

         /* Do the actual casting. */
         wall_hit = FALSE;
         while( FALSE == wall_hit ) {
            graphics_raycast_wall_iter( &wall_map_pos, &ray );

            if( ray.infinite_dist ) {
               break;
            }

            /* Check if ray has hit a wall. */
            if( check_ray_wall_collision( &wall_map_pos, layer, twindow ) ) {
               wall_hit = TRUE;
            }
         }

         wall_map_pos.perpen_dist =
            graphics_raycast_get_distance( &wall_map_pos, &cam_pos, &ray );
         line_height = (int)(twindow->g->h / wall_map_pos.perpen_dist);

         draw_end = graphics_get_ray_stripe_end( line_height, twindow->g );
         draw_start = graphics_get_ray_stripe_start( line_height, twindow->g );

         if( TRUE != ray.infinite_dist ) {
            /* Choose wall color. */
            color = get_wall_color( &wall_map_pos );
         } else {
            /* Fog. */
            color = GRAPHICS_COLOR_WHITE;
         }

         /* Draw the pixels of the stripe as a vertical line. */
         if( 0 == layer_index ) {
            graphics_draw_line( twindow->g, i_x, draw_start, i_x, draw_end, color );
         }

         graphics_floorcast_create(
            &floor_pos, &ray, i_x, &cam_pos,
            &wall_map_pos, twindow->g
         );

         if( 0 > draw_end ) {
            /* Become < 0 if the integer overflows. */
            draw_end = twindow->g->h;
         }

         /* Draw the floor from draw_end to the bottom of the screen. */
         for( i_y = draw_end +1; i_y < twindow->g->h; i_y++ ) {
            graphics_floorcast_throw(
               &floor_pos, i_x, i_y, line_height,
               &cam_pos, &wall_map_pos,
               twindow->g
            );

            /* Ensure we have everything needed to draw the tile. */
            tile = tilemap_get_tile( layer, (int)floor_pos.x, (int)floor_pos.y  );
            if( 0 > tile ) {
               continue;
            }

            set = tilemap_get_tileset( twindow->t, tile, &set_firstgid );
            if( NULL == set ) {
               continue;
            }

            g_tileset = hashmap_iterate(
               &(set->images),
               callback_search_tileset_img_gid,
               twindow->local_client
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
               graphics_set_pixel( twindow->g, i_x, i_y, color );
            }
         }
      }
   }

   vector_iterate( &(l->mobiles), callback_draw_mobiles, twindow );
}

void client_local_update(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {
   tilemap_update_window_ortho(
      twindow, c->puppet->x, c->puppet->y
   );
}

static BOOL client_local_poll_keyboard( struct CLIENT* c, struct INPUT* p ) {
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

void client_local_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p ) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         client_local_poll_keyboard( c, p );
      }
   }
   return;
}

void client_local_free( struct CLIENT* c ) {
   if(
      TRUE == ipc_is_local_client( c->link ) &&
      HASHMAP_SENTINAL == c->sprites.sentinal
   ) {
      hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }
}

GRAPHICS* client_local_get_screen( struct CLIENT* c ) {
   scaffold_assert_client();

   return c->ui->screen_g;
}
