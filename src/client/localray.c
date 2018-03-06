
#define CLIENT_LOCAL_C
#include "../client.h"

#include <stdlib.h>

#include "../callback.h"
#include "../ui.h"
#include "../ipc.h"
#include "../channel.h"
#include "../tilemap.h"
#include "../proto.h"

extern bstring client_input_from_ui;

static void* client_local_raycol_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP* t = twindow->t;
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   uint32_t tile;
   struct TILEMAP_TILESET* set;
   struct TILEMAP_TILE_DATA* tile_info;
   int i;
   struct TILEMAP_TERRAIN_DATA* terrain_iter;

   scaffold_check_null( t );

   layer = t->first_layer;
   while( NULL != layer ) {
      tile = tilemap_get_tile( layer, pos->x, pos->y );
      set = tilemap_get_tileset( twindow->t, tile, NULL );
      tile_info = vector_get( &(set->tiles), tile - 1 );
      for( i = 0 ; 4 > i ; i++ ) {
         terrain_iter = tile_info->terrain[i];
         if( NULL == terrain_iter ) { continue; }
         if( TILEMAP_MOVEMENT_BLOCK == terrain_iter->movement ) {
            return 1;
         }
      }
   }

cleanup:
   return NULL;
}

static BOOL check_ray_wall_collision( GFX_RAY_WALL* wall_map_pos, void* data ) {
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)data;
   struct TILEMAP_LAYER* layer;
   int res;

   res = hashmap_iterate( &(twindow->t->layers), client_local_raycol_cb, twindow );

   return res ? 0 : 1;
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
         return 1 == wall_pos->side ? GRAPHICS_COLOR_WHITE : GRAPHICS_COLOR_GRAY;
      default:
         return 1 == wall_pos->side ? GRAPHICS_COLOR_YELLOW : GRAPHICS_COLOR_BROWN;
   }
}

void client_local_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {
   int x, y, draw_start, draw_end;
   struct INPUT p;
   int done = 0;
   GFX_DELTA cam_pos;
   GFX_DELTA plane_pos;
   GFX_RAY_WALL wall_map_pos; /* The position of the found wall. */
   GFX_RAY ray;
   GFX_DELTA* wall_scr_pos;
   GFX_RAY_FLOOR floor_pos;
   int line_height;

   if( NULL == twindow->t ) {
      return;
   }

   if(
      NULL == twindow->local_client ||
      NULL == twindow->local_client->puppet
   ) {
      return;
   }

   floor_pos.tex_w = 4;
   floor_pos.tex_h = 4;
   GRAPHICS_COLOR texture[16] =
      {3,3,3,3,
       3,4,5,3,
       3,5,4,3,
       3,3,3,3};

   wall_map_pos.map_w = twindow->t->width;
   wall_map_pos.map_h = twindow->t->height;
   cam_pos.x = twindow->local_client->puppet->x;
   cam_pos.y = twindow->local_client->puppet->y;
   cam_pos.dx.facing = -1; /* TODO */
   cam_pos.dy.facing = 0;
   plane_pos.x = 0;
   plane_pos.y = 0.66;

   for( x = 0; x < twindow->g->w; x++ ) {

      wall_map_pos.x = (int)cam_pos.x;
      wall_map_pos.y = (int)cam_pos.y;

      /* Calculate ray position and direction. */
      graphics_raycast_create( &ray, x, &plane_pos, &cam_pos, twindow->g );
      line_height = graphics_raycast_wall_throw(
         &cam_pos, &plane_pos, twindow->g, check_ray_wall_collision,
         twindow, &ray, &wall_map_pos
      );

      draw_end = graphics_get_ray_stripe_end( line_height, twindow->g );
      draw_start = graphics_get_ray_stripe_start( line_height, twindow->g );

      if( TRUE != ray.infinite_dist ) {
         /* Choose wall color. */
         GRAPHICS_COLOR color = get_wall_color( &wall_map_pos );

         /* Draw the pixels of the stripe as a vertical line. */
         graphics_draw_line( twindow->g, x, draw_start, x, draw_end, color );
      }

      graphics_floorcast_create(
         &floor_pos, &ray, x, &cam_pos,
         &wall_map_pos, twindow->g
      );

      if( 0 > draw_end ) {
         /* Become < 0 if the integer overflows. */
         draw_end = twindow->g->h;
      }

      /* Draw the floor from draw_end to the bottom of the screen. */
      for( y = draw_end +1; y < twindow->g->h; y++ ) {
         graphics_floorcast_throw(
            &floor_pos, &ray, x, y, line_height,
            &plane_pos, &cam_pos, &wall_map_pos,
            twindow->g
         );

         graphics_draw_line(
            twindow->g, x, y, x, y,
            texture[floor_pos.tex_w * floor_pos.tex_y + floor_pos.tex_x]
         );
      }

   }

   //vector_iterate( &(l->mobiles), callback_draw_mobiles, twindow );
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
