
#define MODE_C

#include <callback.h>
#include <ui.h>
#include <ipc.h>
#include <channel.h>
#include <proto.h>
#include <plugin.h>
#include <twindow.h>

extern struct tagbstring str_client_cache_path;
extern struct tagbstring str_wid_debug_tiles_pos;
extern struct tagbstring str_client_window_id_chat;
extern struct tagbstring str_client_window_title_chat;
extern struct tagbstring str_client_control_id_chat;
extern struct tagbstring str_client_window_id_inv;
extern struct tagbstring str_client_window_title_inv;
extern struct tagbstring str_client_control_id_inv_self;

extern bstring client_input_from_ui;
extern struct tagbstring str_client_control_id_inv_ground;

struct tagbstring mode_name = bsStatic( "Top Down" );

static void mode_topdown_tilemap_draw_tile(
   struct TILEMAP_LAYER* layer, struct TWINDOW* twindow,
   TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y );

/** \brief Callback: Draw all of the layers for the iterated individual
 *         tile/position (kind of the opposite of tilemap_draw_layer_cb.)
 */
static void* mode_topdown_tilemap_draw_tile_cb(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct TILEMAP_LAYER* layer = NULL;
   struct TILEMAP* t = NULL;
   int layer_idx = 0;
   int layer_max;

   t = twindow_get_tilemap_active( twindow );
   lgc_null( t );

   layer_max = vector_count( t->layers );
   for( layer_idx = 0 ; layer_max > layer_idx ; layer_idx++ ) {
      layer = vector_get( t->layers, layer_idx );
      mode_topdown_tilemap_draw_tile(
         layer, twindow, pos->x, pos->y );
   }

cleanup:
   return NULL;
}

static void* mode_topdown_draw_mobile_cb(
   size_t idx, void* iter, void* arg
) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct CLIENT* local_client = NULL;

   if( NULL == o ) { return NULL; }

   if( mobile_get_animation_reset( o ) ) {
      mobile_do_reset_2d_animation( o );
   }
   mobile_animate( o );
   local_client = twindow_get_local_client( twindow );
   lgc_null( local_client );

   mobile_draw_ortho( o, local_client, twindow );

cleanup:
   return NULL;
}

#ifdef USE_ITEMS

static void* mode_topdown_tilemap_draw_items_cb(
   size_t idx, void* iter, void* arg
) {
   GRAPHICS_RECT* rect = (GRAPHICS_RECT*)arg;
   struct ITEM* e = (struct ITEM*)iter;
   struct GRAPHICS* g_screen = NULL;

   g_screen = client_get_screen( e->client_or_server );

   item_draw_ortho( e, rect->x, rect->y, g_screen );

   return NULL;
}

#endif // USE_ITEMS

/** \brief Callback: Draw iterated layer.
 */
static void* mode_topdown_tilemap_draw_layer_cb(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   SCAFFOLD_SIZE_SIGNED
      x = 0,
      y = 0;

   scaffold_assert( TILEMAP_ORIENTATION_ORTHO == layer->tilemap->orientation );

   for( x = twindow_get_min_x( twindow ) ; twindow_get_max_x( twindow ) > x ; x++ ) {
      for( y = twindow_get_min_y( twindow ) ; twindow_get_max_y( twindow ) > y ; y++ ) {
         mode_topdown_tilemap_draw_tile( layer, twindow, x, y );
      }
   }

   return NULL;
}

#ifdef DEBUG_TILES

static void mode_topdown_tilemap_draw_tile_debug(
   struct TILEMAP_LAYER* layer, GRAPHICS* g, struct TWINDOW* twin,
   SCAFFOLD_SIZE_SIGNED tile_x, SCAFFOLD_SIZE_SIGNED tile_y,
   SCAFFOLD_SIZE_SIGNED pix_x, SCAFFOLD_SIZE_SIGNED pix_y, uint32_t gid
) {
   struct TILEMAP_TILESET* set = NULL;
   bstring bnum = NULL;
   struct TILEMAP_TILE_DATA* tile_info = NULL;
   SCAFFOLD_SIZE td_i;
   int bstr_result;
   struct TILEMAP* t = layer->tilemap;
   SCAFFOLD_SIZE set_firstgid = 0;
   SCAFFOLD_SIZE layers_count;

   bnum = bfromcstralloc( 10, "" );
   lgc_null( bnum );

   /* FIXME: How does gid resolve in a tileset that can have a variable firstgid? */
   set = tilemap_get_tileset( t, gid, &set_firstgid );
   lgc_null( set );
   lgc_zero_against(
      t->scaffold_error, set->tilewidth, "Tile width is zero." );
   lgc_zero_against(
      t->scaffold_error, set->tileheight, "Tile height is zero." );

   layers_count = vector_count( &(t->layers) );
   if( layers_count <= tilemap_dt_layer ) {
      tilemap_dt_layer = 0;
   }

   if( layer->z != tilemap_dt_layer ) {
      /* Don't bother with the debug stuff for another layer. */
      goto cleanup;
   }

   tile_info = vector_get( &(set->tiles), gid - 1 );
   switch( tilemap_dt_state ) {
   case TILEMAP_DEBUG_TERRAIN_COORDS:
      if( layers_count - 1 == layer->z ) {
         bstr_result = bassignformat( bnum, "%d,", tile_x );
         lgc_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 10, GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, VFALSE
         );
         bstr_result = bassignformat( bnum, "%d", tile_y );
         lgc_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 22, GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, VFALSE
         );
         bdestroy( bnum );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_NAMES:
      if( NULL != tile_info && NULL != tile_info->terrain[0] ) {
         bstr_result = bassignformat(
            bnum, "%c%c:%d",
            bdata( tile_info->terrain[0]->name )[0],
            bdata( tile_info->terrain[0]->name )[1],
            tile_info->terrain[0]->movement
         );
         lgc_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + (10 * layer->z),
            GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, VFALSE
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_QUARTERS:
      for( td_i = 0 ; 4 > td_i ; td_i++ ) {
         if( NULL == tile_info || NULL == tile_info->terrain[td_i] ) {
            bstr_result = bassignformat( bnum, "x" );
            lgc_nonzero( bstr_result );
         } else {
            bstr_result = bassignformat(
               bnum, "%d",
               tile_info->terrain[td_i]->id
            );
            lgc_nonzero( bstr_result );
         }
         graphics_draw_text(
            g,
            pix_x + ((td_i % 2) * 12),
            pix_y + ((td_i / 2) * 16),
            GRAPHICS_TEXT_ALIGN_CENTER,
            td_i + 4, GRAPHICS_FONT_SIZE_8,
            bnum,
            VFALSE
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_DEADZONE:
      if(
         !tilemap_inside_inner_map_x( tile_x, twin ) &&
         !tilemap_inside_inner_map_y( tile_y, twin )
      ) {
         graphics_draw_rect(
            g, pix_x, pix_y, 32, 32, GRAPHICS_COLOR_DARK_RED, VTRUE
         );
      }
      if(
         !tilemap_inside_window_deadzone_x( tile_x, twin ) &&
         !tilemap_inside_window_deadzone_y( tile_y, twin )
      ) {
         graphics_draw_rect(
            g, pix_x, pix_y, 32, 32, GRAPHICS_COLOR_DARK_CYAN, VTRUE
         );
      }
      break;
   }

cleanup:
   bdestroy( bnum );
   return;
}

#endif /* DEBUG_TILES */

static void mode_topdown_tilemap_draw_tile(
   struct TILEMAP_LAYER* layer, struct TWINDOW* twindow,
   TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
) {
   struct TILEMAP_TILESET* set = NULL;
   struct CLIENT* local_client = NULL;
   struct TILEMAP* t = NULL;
   const struct MOBILE* o_player = NULL;
   size_t set_firstgid = 0;
   GFX_COORD_PIXEL screen_x = 0,
      screen_y = 0;
#ifdef USE_ITEMS
   struct TILEMAP_ITEM_CACHE* cache = NULL;
#endif // USE_ITEMS
   //struct CHANNEL* l = NULL;
   TILEMAP_TILE gid = 0;

   gid = tilemap_layer_get_tile_gid( layer, x, y );
   if( 0 == gid ) {
      goto cleanup;
   }
   scaffold_assert( gid < tilemap_get_tiles_count( layer ) );

   local_client = twindow_get_local_client( twindow );
   lgc_null( local_client );
   t = twindow_get_tilemap_active( twindow );
   lgc_null( t );

   set = tilemap_get_tileset( t, gid, &set_firstgid );
   if( NULL == set ) {
      goto cleanup; /* Silently. */
   }

   o_player = client_get_puppet( local_client );

   /* Figure out the window position to draw to. */
   screen_x =
      tilemap_tileset_get_tile_width( set ) * (x - twindow_get_x( twindow ));
   screen_y =
      tilemap_tileset_get_tile_height( set ) * (y - twindow_get_y( twindow ));

   if( 0 > screen_x || 0 > screen_y ) {
      goto cleanup; /* Silently. */
   }

   if(
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_x( mobile_get_x( o_player ) + 1, twindow ) &&
       TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_inner_map_x( mobile_get_x( o_player ), twindow )
      ) || (
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_x( mobile_get_x( o_player ) - 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_x( mobile_get_x( o_player ), twindow )
      )
   ) {
      screen_x += mobile_get_steps_remaining_x( o_player, VTRUE );
   }

   if(
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_y( mobile_get_y( o_player ) + 1, twindow )  &&
       TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_inner_map_y( mobile_get_y( o_player ), twindow )
      ) || (
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_y( mobile_get_y( o_player ) - 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_y( mobile_get_y( o_player ), twindow )
      )
   ) {
      screen_y += mobile_get_steps_remaining_y( o_player, VTRUE );
   }

   tilemap_tile_draw_ortho( layer, x, y, screen_x, screen_y, set, twindow );

#ifdef USE_ITEMS

   cache = tilemap_get_item_cache( t, x, y, VFALSE );
   if( NULL != cache ) {
      vector_iterate(
         &(cache->items), mode_topdown_tilemap_draw_items_cb, &tile_screen_rect
      );
   }

#endif // USE_ITEMS

#ifdef DEBUG_TILES
   mode_topdown_tilemap_draw_tile_debug(
      layer, twindow->g, twindow, x, y,
      tile_screen_rect.x, tile_screen_rect.y, gid
   );
#endif /* DEBUG_TILES */

cleanup:
   return;
}

static void mode_topdown_tilemap_update_window(
   struct TWINDOW* twindow,
   TILEMAP_COORD_TILE focal_x, TILEMAP_COORD_TILE focal_y
) {
   TILEMAP_COORD_TILE
      border_x = TILEMAP_BORDER,
      border_y = TILEMAP_BORDER;
   //struct CLIENT* c = NULL;
   struct TILEMAP* t = NULL;
   TILEMAP_EXCLUSION exclusion;
   //struct CHANNEL* l = NULL;

   /* Clamp border to the edge of the map. */
   if( 0 == twindow_get_x( twindow ) ) {
      border_x = 0;
   }
   if( 0 == twindow_get_y( twindow ) ) {
      border_y = 0;
   }

   //c = twindow_get_local_client( twindow );
   //lgc_null( c );
   /*l = client_get_channel_active( c );
   lgc_null( l );
   t = channel_get_tilemap( l );*/

   t = twindow_get_tilemap_active( twindow );
   if( NULL == t ) {
      return;
   }

   /* TODO: Request item caches for tiles scrolling into view. */

#if 0
   /* Find the focal point if we're not centered on it. */
   if(
      focal_x < twindow_get_x( twindow ) ||
      focal_x > twindow_get_x( twindow ) + twindow_get_width( twindow )
   ) {
      border_x = focal_x - (twindow_get_width( twindow ) / 2);
      lg_debug( __FILE__, "Setting focal X: %d\n", border_x );
      twindow_set_x( twindow, border_x );
   }
   if(
      focal_y < twindow_get_y( twindow ) ||
      focal_y > twindow_get_y( twindow ) + twindow_get_height( twindow )
   ) {
      border_y = focal_y - (twindow_get_height( twindow ) / 2);
      lg_debug( __FILE__, "Setting focal Y: %d\n", border_y );
      twindow_set_y( twindow, border_y );
   }
#endif // 0

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
   twindow_set_max_x(
      twindow,
      twindow_get_x( twindow ) + twindow_get_width( twindow ) +
      border_x < t->width ?
         twindow_get_right( twindow ) + border_x :
         t->width
   );
   twindow_set_max_y(
      twindow,
      twindow_get_y( twindow ) + twindow_get_height( twindow ) +
      border_y < t->height ?
         twindow_get_bottom( twindow ) + border_y :
         t->height
   );

   twindow_set_min_x(
      twindow,
      twindow_get_x( twindow ) - border_x >= 0 &&
      twindow_get_right( twindow ) <= t->width
      ? twindow_get_x( twindow ) - border_x : 0 );
   twindow_set_min_y(
      twindow,
      twindow_get_y( twindow ) - border_y >= 0 &&
      twindow_get_bottom( twindow ) <= t->height
      ? twindow_get_y( twindow ) - border_y : 0 );

   return;
}

static void mode_topdown_tilemap_draw_tilemap( struct TWINDOW* twindow ) {
   struct CLIENT* local_client = NULL;
   struct MOBILE* o = NULL;
   struct TILEMAP* t = NULL;
   //struct CHANNEL* l = NULL;

   local_client = twindow_get_local_client( twindow );
   lgc_null( local_client );
   o = client_get_puppet( local_client );
   t = twindow_get_tilemap_active( twindow );

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
      vector_iterate( t->layers, mode_topdown_tilemap_draw_layer_cb, twindow );
   } else if(
      TILEMAP_REDRAW_DIRTY == t->redraw_state &&
      0 < vector_count( t->dirty_tiles )
   ) {
      /* Just redraw dirty tiles. */
      vector_iterate(
         t->dirty_tiles, mode_topdown_tilemap_draw_tile_cb, twindow
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

cleanup:
   return;
}

PLUGIN_RESULT mode_topdown_init() {
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_topdown_draw(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   mode_topdown_tilemap_draw_tilemap( client_get_local_window( c ) );
   vector_iterate( l->mobiles, mode_topdown_draw_mobile_cb, client_get_local_window( c ) );
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_topdown_update(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   struct MOBILE* o = NULL;
   PLUGIN_RESULT ret = PLUGIN_SUCCESS;
   struct TILEMAP* t = NULL;

   o = client_get_puppet( c );
   if( NULL == o ) {
      ret = PLUGIN_FAILURE;
      goto cleanup;
   }

   t = client_get_tilemap_active( c );
   if( NULL == t ) {
      ret = PLUGIN_FAILURE;
      lg_error( __FILE__, "No active tilemap set.\n" );
      goto cleanup;
   }

   mode_topdown_tilemap_update_window(
      client_get_local_window( c ), mobile_get_x( o ), mobile_get_y( o )
   );

cleanup:
   return ret;
}

static VBOOL mode_topdown_poll_keyboard( struct CLIENT* c, struct INPUT* p ) {
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
      return VFALSE; /* Silently ignore input until animations are done. */
   } else {
      l = mobile_get_channel( puppet );
      lgc_null( l );
      update = action_packet_new( l, puppet, ACTION_OP_NONE, 0, 0, NULL );
   }

   /* If no windows need input, then move on to game input. */
   switch( p->character ) {
   case INPUT_ASSIGNMENT_QUIT: proto_client_stop( c ); return VTRUE;
   case INPUT_ASSIGNMENT_UP:
      action_packet_set_op( update, ACTION_OP_MOVEUP );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) - 1 );
      proto_client_send_update( c, update );
      return VTRUE;

   case INPUT_ASSIGNMENT_LEFT:
      action_packet_set_op( update, ACTION_OP_MOVELEFT );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) - 1 );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) );
      proto_client_send_update( c, update );
      return VTRUE;

   case INPUT_ASSIGNMENT_DOWN:
      action_packet_set_op( update, ACTION_OP_MOVEDOWN );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) + 1 );
      proto_client_send_update( c, update );
      return VTRUE;

   case INPUT_ASSIGNMENT_RIGHT:
      action_packet_set_op( update, ACTION_OP_MOVERIGHT );
      action_packet_set_tile_x( update, mobile_get_x( puppet ) + 1 );
      action_packet_set_tile_y( update, mobile_get_y( puppet ) );
      proto_client_send_update( c, update );
      return VTRUE;

   case INPUT_ASSIGNMENT_ATTACK:
      action_packet_set_op( update, ACTION_OP_ATTACK );
      /* TODO: Get attack target. */
      proto_client_send_update( c, update );
      return VTRUE;

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
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, VTRUE, VFALSE,
         client_input_from_ui, 0, UI_CONST_HEIGHT_FULL, 300,
         UI_CONST_HEIGHT_FULL
      );
      ui_control_add( win, &str_client_control_id_inv_self, control );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, VTRUE, VFALSE,
         client_input_from_ui, 300, UI_CONST_HEIGHT_FULL, 300,
         UI_CONST_HEIGHT_FULL
      );
      cache = tilemap_get_item_cache( t, puppet->x, puppet->y, VTRUE );
      ui_set_inventory_pane_list( control, &(cache->items) );
      ui_control_add( win, &str_client_control_id_inv_ground, control );
      ui_window_push( ui, win );
      return VTRUE;
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
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, VTRUE, VTRUE,
         client_input_from_ui, -1, -1, -1, -1
      );
      ui_control_add( win, &str_client_control_id_chat, control );
      ui_window_push( ui, win );
      return VTRUE;
#ifdef DEBUG_VM
   //case 'p': windef_show_repl( ui ); return VTRUE;
#endif /* DEBUG_VM */
#ifdef DEBUG_TILES
   case 't':
      if( 0 == p->repeat ) {
         tilemap_toggle_debug_state();
         return VTRUE;
      }
      break;
   case 'l':
      if( 0 == p->repeat ) {
         tilemap_dt_layer++;
         return VTRUE;
      }
      break;
#endif /* DEBUG_TILES */
   }

cleanup:
   action_packet_free( &update );
   return VFALSE;
}

PLUGIN_RESULT mode_topdown_poll_input(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         mode_topdown_poll_keyboard( c, p );
      }
   }
   return PLUGIN_SUCCESS;
}

PLUGIN_RESULT mode_topdown_free( struct CLIENT* c ) {
   #if 0

   if(
      VTRUE == client_is_local( c ) &&
      hashmap_is_valid( &(c->sprites ) )
   ) {
      /* FIXME: This causes crash on re-login. */
      //hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }
   #endif
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

PLUGIN_RESULT mode_topdown_mobile_action_client( struct ACTION_PACKET* update ) {
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

PLUGIN_RESULT mode_topdown_mobile_action_server( struct ACTION_PACKET* update ) {
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
