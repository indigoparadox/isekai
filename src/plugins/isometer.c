
#if defined( USE_DYNAMIC_PLUGINS ) || defined( USE_STATIC_MODE_ISOMETRIC )

#define MODE_C
//#include <mode.h>

#include "../callback.h"
#include "../ipc.h"
#include "../channel.h"
#include "../proto.h"
#include "../plugin.h"

extern bstring client_input_from_ui;
extern struct tagbstring str_client_window_id_chat;
extern struct tagbstring str_client_window_title_chat;
extern struct tagbstring str_client_control_id_chat;

struct tagbstring mode_name = bsStatic( "Isometric" );

static void mode_isometer_tilemap_draw_tile(
   struct TILEMAP_LAYER* layer, struct TWINDOW* twindow,
   TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y, SCAFFOLD_SIZE gid
);

/** \brief Callback: Draw all of the layers for the iterated individual
 *         tile/position (kind of the opposite of tilemap_draw_layer_cb.)
 */
static void* mode_isometer_tilemap_draw_tile_cb(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct TILEMAP_LAYER* layer = NULL;
   //struct CLIENT* c = NULL;
   struct TILEMAP* t = NULL;
   int layer_idx = 0;
   int layer_max;
   uint32_t tile;

   //c = scaffold_container_of( twindow, struct CLIENT, local_window );
   //t = c->active_tilemap;

   t = twindow_get_tilemap_active( twindow );

   vector_lock( t->layers, TRUE );
   layer_max = vector_count( t->layers );
   for( layer_idx = 0 ; layer_max > layer_idx ; layer_idx++ ) {
      layer = vector_get( t->layers, layer_idx );
      assert(
         TILEMAP_ORIENTATION_ISO == layer->tilemap->orientation );
      tile = tilemap_layer_get_tile_gid( layer, pos->x, pos->y );
      if( 0 < tile ) {
         mode_isometer_tilemap_draw_tile(
            layer, twindow, pos->x, pos->y, tile );
      }
   }
   vector_lock( t->layers, FALSE );

   return NULL;
}

static void* mode_isometer_draw_mobile_cb(
   size_t idx, void* iter, void* arg
) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct CLIENT* local_client = NULL;

   if( NULL == o ) { return NULL; }

   if( TRUE == mobile_get_animation_reset( o ) ) {
      mobile_do_reset_2d_animation( o );
   }
   mobile_animate( o );
   local_client = twindow_get_local_client( twindow );
   mobile_draw_ortho( o, local_client, twindow );

   return NULL;
}

#ifdef USE_ITEMS

static void* mode_isometer_tilemap_draw_items_cb(
   size_t idx, void* iter, void* arg
) {
   GRAPHICS_RECT* rect = (GRAPHICS_RECT*)arg;
   struct ITEM* e = (struct ITEM*)iter;
   struct GRAPHICS* g_screen = NULL;

   g_screen = client_get_screen( e->client_or_server );

   item_draw_ortho( e, rect->x, rect->y, g_screen );
}

#endif // USE_ITEMS

/** \brief Callback: Draw iterated layer.
 */
static void* mode_isometer_tilemap_draw_layer_cb(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   SCAFFOLD_SIZE_SIGNED
      x = 0,
      y = 0;
   uint32_t tile;
   //struct VECTOR* tiles = NULL;

   //assert( TILEMAP_ORIENTATION_ISO == layer->tilemap->orientation );

   /* TODO: Do culling in iso-friendly way. */
   for( x = twindow_get_min_x( twindow ) ; twindow_get_max_x( twindow ) > x ; x++ ) {
      for( y = twindow_get_min_y( twindow ) ; twindow_get_max_y( twindow ) > y ; y++ ) {
         tile = tilemap_layer_get_tile_gid( layer, x, y );
         if( 0 == tile ) {
            continue;
         }
         mode_isometer_tilemap_draw_tile(
            layer, twindow, x, y, tile );
      }
   }

   return NULL;
}

static void mode_isometer_tilemap_draw_tile(
   struct TILEMAP_LAYER* layer, struct TWINDOW* twindow,
   TILEMAP_COORD_TILE tile_x, TILEMAP_COORD_TILE tile_y, SCAFFOLD_SIZE gid
) {
   struct TILEMAP_TILESET* set = NULL;
   GRAPHICS_RECT tile_tilesheet_pos;
   GRAPHICS_RECT tile_screen_rect;
   struct CLIENT* local_client = NULL;
   struct TILEMAP* t = NULL;
   //const struct MOBILE* o = NULL;
   GRAPHICS* g_tileset = NULL;
   SCAFFOLD_SIZE set_firstgid = 0;
#ifdef USE_ITEMS
   struct TILEMAP_ITEM_CACHE* cache = NULL;
#endif // USE_ITEMS
   GFX_COORD_PIXEL
      iso_dest_offset_x,
      iso_dest_offset_y;

   local_client = twindow_get_local_client( twindow );
   lgc_null( local_client );
   t = client_get_tilemap_active( local_client );
   lgc_null( t );
   //o = local_client->puppet;
   set = tilemap_get_tileset( t, gid, &set_firstgid );
   if( NULL == set ) {
      goto cleanup; /* Silently. */
   }

   lgc_zero_against(
      t->scaffold_error, tilemap_tileset_get_tile_width( set ), "Tile width is zero." );
   lgc_zero_against(
      t->scaffold_error, tilemap_tileset_get_tile_height( set ), "Tile height is zero." );

   /* Figure out the window position to draw to. */
   //tile_screen_rect.x = set->tilewidth * (x - twindow->x);
   //tile_screen_rect.y = set->tileheight * (y - twindow->y);

   iso_dest_offset_x = tilemap_tileset_get_tile_width( set ) / 2;
   iso_dest_offset_y = tilemap_tileset_get_tile_height( set ) / 2;
   tile_screen_rect.x = ((tile_x - tile_y) * iso_dest_offset_x) - twindow_get_x( twindow );
   tile_screen_rect.y = ((tile_x + tile_y) * iso_dest_offset_y) - twindow_get_y( twindow );

   if( 0 > tile_screen_rect.x || 0 > tile_screen_rect.y ) {
      goto cleanup; /* Silently. */
   }

   /* Figure out the graphical tile to draw from. */
   g_tileset = tilemap_tileset_get_image_default( set, local_client );
   /* FIXME */
   /* If the current tileset doesn't exist, then load it. */
   //g_tileset = hashmap_iterate(
   //   &(set->images), callback_search_tileset_img_gid, local_client );
   if( NULL == g_tileset ) {
      /* TODO: Use a built-in placeholder tileset. */
      goto cleanup;
   }

   tilemap_get_tile_tileset_pos(
      set, set_firstgid, g_tileset, gid, &tile_tilesheet_pos );

      /*
   if(
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_x( o->x + 1, twindow ) &&
       TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_inner_map_x( o->x, twindow )
      ) || (
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_x( o->x - 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_x( o->x, twindow )
      )
   ) {
      tile_screen_rect.x += mobile_get_steps_remaining_x( o, TRUE );
   }

   if(
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_y( o->y + 1, twindow )  &&
       TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_inner_map_y( o->y, twindow )
      ) || (
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_y( o->y - 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_y( o->y, twindow )
      )
   ) {
      tile_screen_rect.y += mobile_get_steps_remaining_y( o, TRUE );
   }
   */

   graphics_blit_partial(
      twindow_get_screen( twindow ),
      tile_screen_rect.x, tile_screen_rect.y,
      tile_tilesheet_pos.x, tile_tilesheet_pos.y,
      tilemap_tileset_get_tile_width( set ), tilemap_tileset_get_tile_height( set ),
      g_tileset
   );

#ifdef USE_ITEMS

   cache = tilemap_get_item_cache( t, tile_x, tile_y, FALSE );
   if( NULL != cache ) {
      vector_iterate(
         &(cache->items), mode_isometer_tilemap_draw_items_cb, &tile_screen_rect
      );
   }

#endif // USE_ITEMS

cleanup:
   return;
}

static void mode_isometer_tilemap_update_window(
   struct TWINDOW* twindow,
   TILEMAP_COORD_TILE focal_x, TILEMAP_COORD_TILE focal_y
) {
   TILEMAP_COORD_TILE
      border_x = 0,
      border_y = 0;
   //struct CLIENT* c = NULL;
   struct TILEMAP* t = NULL;
   TILEMAP_EXCLUSION exclusion;

   if( 0 != twindow_get_x( twindow ) ) {
      border_x = TILEMAP_BORDER;
   }
   if( 0 != twindow_get_y( twindow ) ) {
      border_y = TILEMAP_BORDER;
   }

   //c = scaffold_container_of( twindow, struct CLIENT, local_window );
   t = twindow_get_tilemap_active( twindow );

   if( NULL == t ) {
      return;
   }

   /* TODO: Request item caches for tiles scrolling into view. */

   /* Find the focal point if we're not centered on it. */
   if( focal_x < twindow_get_x( twindow ) || focal_x > twindow_get_right( twindow ) ) {
      twindow_set_x( twindow, focal_x - (twindow_get_width( twindow ) / 2) );
   }
   if( focal_y < twindow_get_y( twindow ) || focal_y > twindow_get_bottom( twindow ) ) {
      twindow_set_y( twindow, focal_y - (twindow_get_height( twindow ) / 2) );
   }

   /* Scroll the window to follow the focal point. */
   exclusion = tilemap_inside_window_deadzone_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point right of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_shift_right_x( twindow, 1 );
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point left of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_shift_left_x( twindow, 1 );
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   exclusion = tilemap_inside_window_deadzone_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point below window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_shift_down_y( twindow, 1 );
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point above window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_shift_up_y( twindow, 1 );
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   /* Clamp the window to the edge of the map. */
   exclusion = tilemap_inside_inner_map_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point too close to map left edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_set_x( twindow, t->width - twindow_get_width( twindow ) );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point too close to map right edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_set_x( twindow, 0 );
   }

   exclusion = tilemap_inside_inner_map_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point too close to map bottom edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_set_y( twindow, t->height - twindow_get_height( twindow ) );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      lg_debug(
         __FILE__, "Focal point too close to map top edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow_set_y( twindow, 0 );
   }

   /* Only calculate these when window moves and store them. */
   twindow_set_max_x( twindow, twindow_get_right( twindow ) + border_x < t->width ?
      twindow_get_right( twindow ) + border_x : t->width );
   twindow_set_max_y( twindow, twindow_get_bottom( twindow ) + border_y < t->height ?
      twindow_get_bottom( twindow ) + border_y : t->height );

   twindow_set_min_x( twindow,
      twindow_get_x( twindow ) - border_x >= 0 &&
      twindow_get_right( twindow ) <= t->width
      ? twindow_get_x( twindow ) - border_x : 0 );
   twindow_set_min_y( twindow,
      twindow_get_y( twindow ) - border_y >= 0 &&
      twindow_get_bottom( twindow ) <= t->height
      ? twindow_get_y( twindow ) - border_y : 0 );
}


static void mode_isometer_tilemap_draw_tilemap( struct TWINDOW* twindow ) {
   struct CLIENT* local_client = NULL;
   struct MOBILE* o = NULL;
   struct TILEMAP* t = NULL;

   local_client = twindow_get_local_client( twindow );
   o = client_get_puppet( local_client );
   t = client_get_tilemap_active( local_client );

   if( NULL == t ) {
      return;
   }

   /* Redraw all tiles if requested. */
   if(
      TILEMAP_REDRAW_ALL == t->redraw_state
#ifdef DEBUG_TILES
      || TILEMAP_DEBUG_TERRAIN_OFF != tilemap_dt_state
#endif /* DEBUG_TILES */
   ) {
      vector_iterate( t->layers, mode_isometer_tilemap_draw_layer_cb, twindow );
   } else if(
      TILEMAP_REDRAW_DIRTY == t->redraw_state &&
      0 < vector_count( t->dirty_tiles )
   ) {
      /* Just redraw dirty tiles. */
      vector_iterate(
         t->dirty_tiles, mode_isometer_tilemap_draw_tile_cb, twindow
      );
   }

   /* If we've done a full redraw as requested then switch back to just dirty *
    * tiles.                                                                  */
   if(
      0 != mobile_get_steps_remaining( o ) &&
      TILEMAP_REDRAW_ALL != t->redraw_state
   ) {
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if(
      0 == mobile_get_steps_remaining( o ) &&
      TILEMAP_REDRAW_DIRTY != t->redraw_state
   ) {
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_DIRTY );
   }
}

PLUGIN_RESULT mode_isometer_draw(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   mode_isometer_tilemap_draw_tilemap( client_get_local_window( c ) );
   vector_iterate( l->mobiles, mode_isometer_draw_mobile_cb, client_get_local_window( c ) );
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_isometer_update(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   struct MOBILE* o = NULL;
   o = client_get_puppet( c );
   if( NULL == o ) {
      return PLUGIN_SUCCESS;
   }
   mode_isometer_tilemap_update_window(
      client_get_local_window( c ), mobile_get_x( o ), mobile_get_y( o )
   );
   return PLUGIN_SUCCESS;
}

#if 0
static BOOL mode_isometer_poll_keyboard( struct CLIENT* c, struct INPUT* p ) {
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
      lgc_null( update.l );
      l = puppet->channel;
      lgc_null_msg( l, "No channel loaded." );
      t = &(l->tilemap);
      lgc_null_msg( t, "No tilemap loaded." );
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
         lgc_null( client_input_from_ui );
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
         lgc_null( client_input_from_ui );
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

void mode_isometer_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p ) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         mode_isometer_poll_keyboard( c, p );
      }
   }
   return;
}
#endif // 0

PLUGIN_RESULT mode_isometer_free( struct CLIENT* c ) {
   if(
      TRUE == client_is_local( c ) //&&
      //hashmap_is_valid( client_get_local_window( c ) )
   ) {
      /* FIXME: This causes crash on re-login. */
      //hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_isometer_init() {
   return PLUGIN_SUCCESS;
}

static BOOL mode_isometer_poll_keyboard( struct CLIENT* c, struct INPUT* p ) {
   struct MOBILE* puppet = NULL;
   struct ACTION_PACKET* update = NULL;
   struct UI* ui = NULL;
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;
   struct CHANNEL* l = NULL;
   //struct TILEMAP* t = NULL;
#ifdef USE_ITEMS
   struct TILEMAP_ITEM_CACHE* cache = NULL;
#endif // USE_ITEMS

   scaffold_assert_client();

   puppet = client_get_puppet( c );

   /* Make sure the buffer that all windows share is available. */
   if(
      NULL == puppet ||
      (mobile_get_steps_remaining( puppet ) < -8 ||
      mobile_get_steps_remaining( puppet ) > 8)
   ) {
      /* TODO: Handle limited input while loading. */
      input_clear_buffer( p );
      return FALSE; /* Silently ignore input until animations are done. */
   } else {
      l = mobile_get_channel( puppet );
      lgc_null( l );
      update = action_packet_new( l, puppet, ACTION_OP_NONE, 0, 0, NULL );
   }

   /* If no windows need input, then move on to game input. */
   switch( p->character ) {
   case INPUT_ASSIGNMENT_QUIT: proto_client_stop( c ); return TRUE;
   case INPUT_ASSIGNMENT_UP:
      action_packet_set_op( update, ACTION_OP_MOVEUP );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) - 1 );
      proto_client_send_update( c, update );
      return TRUE;

   case INPUT_ASSIGNMENT_LEFT:
      action_packet_set_op( update, ACTION_OP_MOVELEFT );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) - 1 );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) );
      proto_client_send_update( c, update );
      return TRUE;

   case INPUT_ASSIGNMENT_DOWN:
      action_packet_set_op( update, ACTION_OP_MOVEDOWN );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) + 1 );
      proto_client_send_update( c, update );
      return TRUE;

   case INPUT_ASSIGNMENT_RIGHT:
      action_packet_set_op( update, ACTION_OP_MOVERIGHT );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) + 1 );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) );
      proto_client_send_update( c, update );
      return TRUE;

   case INPUT_ASSIGNMENT_ATTACK:
      action_packet_set_op( update, ACTION_OP_ATTACK );
      /* TODO: Get attack target. */
      proto_client_send_update( c, update );
      return TRUE;

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
#endif // USE_ITEMS

   case '\\':
      if( NULL == client_input_from_ui ) {
         action_packet_set_tile_x( update, mobile_get_x( puppet ) );
         action_packet_set_tile_y( update, mobile_get_y( puppet ) - 1 );
         client_input_from_ui = bfromcstralloc( 80, "" );
         lgc_null( client_input_from_ui );
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
   //case 'p': windef_show_repl( ui ); return TRUE;
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
   action_packet_free( &update );
   return FALSE;
}

PLUGIN_RESULT mode_isometer_poll_input(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         mode_isometer_poll_keyboard( c, p );
      }
   }
   return PLUGIN_SUCCESS;
}

#endif /* USE_DYNAMIC_PLUGINS || USE_STATIC_MODE_ISOMETRIC */
