
#define MODE_C
#include "../mode.h"

#include "../callback.h"
#include "../ui.h"
#include "../ipc.h"
#include "../channel.h"
#include "../proto.h"

extern bstring client_input_from_ui;

static void mode_topdown_tilemap_draw_tile(
   struct TILEMAP_LAYER* layer, struct TWINDOW* twindow,
   TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y, SCAFFOLD_SIZE gid
);

/** \brief Callback: Draw all of the layers for the iterated individual
 *         tile/position (kind of the opposite of tilemap_draw_layer_cb.)
 */
static void* mode_topdown_tilemap_draw_tile_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct TILEMAP_LAYER* layer = NULL;
   struct CLIENT* c = NULL;
   struct TILEMAP* t = NULL;
   int layer_idx = 0;
   int layer_max;
   uint32_t tile;

   c = scaffold_container_of( twindow, struct CLIENT, local_window );
   t = c->active_tilemap;

   vector_lock( &(t->layers), TRUE );
   layer_max = vector_count( &(t->layers) );
   for( layer_idx = 0 ; layer_max > layer_idx ; layer_idx++ ) {
      layer = vector_get( &(t->layers), layer_idx );
      scaffold_assert(
         TILEMAP_ORIENTATION_ORTHO == layer->tilemap->orientation );
      tile = tilemap_get_tile( layer, pos->x, pos->y );
      if( 0 < tile ) {
         mode_topdown_tilemap_draw_tile(
            layer, twindow, pos->x, pos->y, tile );
      }
   }
   vector_lock( &(t->layers), FALSE );
}

static void* mode_topdown_draw_mobile_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   struct CLIENT* local_client = NULL;

   if( NULL == o ) { return NULL; }

   if( TRUE == o->animation_reset ) {
      mobile_do_reset_2d_animation( o );
   }
   mobile_animate( o );
   local_client = scaffold_container_of( twindow, struct CLIENT, local_window );
   mobile_draw_ortho( o, local_client, twindow );

   return NULL;
}

static void* mode_topdown_tilemap_draw_items_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   GRAPHICS_RECT* rect = (GRAPHICS_RECT*)arg;
   struct ITEM* e = (struct ITEM*)iter;
   struct GRAPHICS* g_screen = NULL;

   g_screen = client_get_screen( e->client_or_server );

   item_draw_ortho( e, rect->x, rect->y, g_screen );
}

/** \brief Callback: Draw iterated layer.
 */
static void* mode_topdown_tilemap_draw_layer_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct TWINDOW* twindow = (struct TWINDOW*)arg;
   SCAFFOLD_SIZE_SIGNED
      x = 0,
      y = 0;
   uint32_t tile;
   struct VECTOR* tiles = NULL;

   tiles = &(layer->tiles);

   if( NULL == tiles || 0 == vector_count( tiles ) ) {
      goto cleanup;
   }

   scaffold_assert( TILEMAP_ORIENTATION_ORTHO == layer->tilemap->orientation );

   /* TODO: Do culling in iso-friendly way. */
   for( x = twindow->min_x ; twindow->max_x > x ; x++ ) {
      for( y = twindow->min_y ; twindow->max_y > y ; y++ ) {
         tile = tilemap_get_tile( layer, x, y );
         if( 0 == tile ) {
            continue;
         }
         mode_topdown_tilemap_draw_tile(
            layer, twindow, x, y, tile );
      }
   }

cleanup:
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
   scaffold_check_null( bnum );

   /* FIXME: How does gid resolve in a tileset that can have a variable firstgid? */
   set = tilemap_get_tileset( t, gid, &set_firstgid );
   scaffold_check_null( set );
   scaffold_check_zero_against(
      t->scaffold_error, set->tilewidth, "Tile width is zero." );
   scaffold_check_zero_against(
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
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 10, GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, FALSE
         );
         bstr_result = bassignformat( bnum, "%d", tile_y );
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + 22, GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, FALSE
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
         scaffold_check_nonzero( bstr_result );
         graphics_draw_text(
            g, pix_x + 16, pix_y + (10 * layer->z),
            GRAPHICS_TEXT_ALIGN_CENTER,
            GRAPHICS_COLOR_DARK_BLUE, GRAPHICS_FONT_SIZE_8, bnum, FALSE
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_QUARTERS:
      for( td_i = 0 ; 4 > td_i ; td_i++ ) {
         if( NULL == tile_info || NULL == tile_info->terrain[td_i] ) {
            bstr_result = bassignformat( bnum, "x" );
            scaffold_check_nonzero( bstr_result );
         } else {
            bstr_result = bassignformat(
               bnum, "%d",
               tile_info->terrain[td_i]->id
            );
            scaffold_check_nonzero( bstr_result );
         }
         graphics_draw_text(
            g,
            pix_x + ((td_i % 2) * 12),
            pix_y + ((td_i / 2) * 16),
            GRAPHICS_TEXT_ALIGN_CENTER,
            td_i + 4, GRAPHICS_FONT_SIZE_8,
            bnum,
            FALSE
         );
      }
      break;
   case TILEMAP_DEBUG_TERRAIN_DEADZONE:
      if(
         !tilemap_inside_inner_map_x( tile_x, twin ) &&
         !tilemap_inside_inner_map_y( tile_y, twin )
      ) {
         graphics_draw_rect(
            g, pix_x, pix_y, 32, 32, GRAPHICS_COLOR_DARK_RED, TRUE
         );
      }
      if(
         !tilemap_inside_window_deadzone_x( tile_x, twin ) &&
         !tilemap_inside_window_deadzone_y( tile_y, twin )
      ) {
         graphics_draw_rect(
            g, pix_x, pix_y, 32, 32, GRAPHICS_COLOR_DARK_CYAN, TRUE
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
   TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y, SCAFFOLD_SIZE gid
) {
   struct TILEMAP_TILESET* set = NULL;
   GRAPHICS_RECT tile_tilesheet_pos;
   GRAPHICS_RECT tile_screen_rect;
   struct CLIENT* local_client = NULL;
   struct TILEMAP* t = NULL;
   const struct MOBILE* o = NULL;
   GRAPHICS* g_tileset = NULL;
   SCAFFOLD_SIZE set_firstgid = 0;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   local_client = scaffold_container_of( twindow, struct CLIENT, local_window );
   t = local_client->active_tilemap;
   o = local_client->puppet;
   set = tilemap_get_tileset( t, gid, &set_firstgid );
   if( NULL == set ) {
      goto cleanup; /* Silently. */
   }

   scaffold_check_zero_against(
      t->scaffold_error, set->tilewidth, "Tile width is zero." );
   scaffold_check_zero_against(
      t->scaffold_error, set->tileheight, "Tile height is zero." );
   if( 0 == set->tilewidth || 0 == set->tileheight ) {
      goto cleanup;
   }

   /* Figure out the window position to draw to. */
   tile_screen_rect.x = set->tilewidth * (x - twindow->x);
   tile_screen_rect.y = set->tileheight * (y - twindow->y);

   if( 0 > tile_screen_rect.x || 0 > tile_screen_rect.y ) {
      goto cleanup; /* Silently. */
   }

   /* Figure out the graphical tile to draw from. */
   g_tileset = (GRAPHICS*)hashmap_get_first( &(set->images) );
   /* FIXME */
   /* If the current tileset doesn't exist, then load it. */
   g_tileset = hashmap_iterate(
      &(set->images), callback_search_tileset_img_gid, local_client );
   if( NULL == g_tileset ) {
      /* TODO: Use a built-in placeholder tileset. */
      goto cleanup;
   }

   tilemap_get_tile_tileset_pos(
      set, set_firstgid, g_tileset, gid, &tile_tilesheet_pos );

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

   graphics_blit_partial(
      twindow->g,
      tile_screen_rect.x, tile_screen_rect.y,
      tile_tilesheet_pos.x, tile_tilesheet_pos.y,
      set->tilewidth, set->tileheight,
      g_tileset
   );

   cache = tilemap_get_item_cache( t, x, y, FALSE );
   if( NULL != cache ) {
      vector_iterate(
         &(cache->items), mode_topdown_tilemap_draw_items_cb, &tile_screen_rect
      );
   }

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
      border_x = twindow->x == 0 ? 0 : TILEMAP_BORDER,
      border_y = twindow->y == 0 ? 0 : TILEMAP_BORDER;
   struct CLIENT* c = NULL;
   struct TILEMAP* t = NULL;
   TILEMAP_EXCLUSION exclusion;

   c = scaffold_container_of( twindow, struct CLIENT, local_window );
   t = c->active_tilemap;

   if( NULL == t ) {
      return;
   }

   /* TODO: Request item caches for tiles scrolling into view. */

   /* Find the focal point if we're not centered on it. */
   if( focal_x < twindow->x || focal_x > twindow->x + twindow->width ) {
      twindow->x = focal_x - (twindow->width / 2);
   }
   if( focal_y < twindow->y || focal_y > twindow->y + twindow->height ) {
      twindow->y = focal_y - (twindow->height / 2);
   }

   /* Scroll the window to follow the focal point. */
   exclusion = tilemap_inside_window_deadzone_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point right of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x++;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point left of window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x--;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   exclusion = tilemap_inside_window_deadzone_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point below window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y++;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point above window dead zone.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y--;
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   }

   /* Clamp the window to the edge of the map. */
   exclusion = tilemap_inside_inner_map_x( focal_x, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map left edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x = t->width - twindow->width;
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map right edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->x = 0;
   }

   exclusion = tilemap_inside_inner_map_y( focal_y, twindow );
   if( TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map bottom edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y = t->height - twindow->height;
   } else if( TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP == exclusion ) {
#ifdef DEBUG_TILES_VERBOSE
      scaffold_print_debug(
         &module, "Focal point too close to map top edge.\n" );
#endif /* DEBUG_TILES_VERBOSE */
      twindow->y = 0;
   }

   /* Only calculate these when window moves and store them. */
   twindow->max_x = twindow->x + twindow->width + border_x < t->width ?
      twindow->x + twindow->width + border_x : t->width;
   twindow->max_y = twindow->y + twindow->height + border_y < t->height ?
      twindow->y + twindow->height + border_y : t->height;

   twindow->min_x =
      twindow->x - border_x >= 0 &&
      twindow->x + twindow->width <= t->width
      ? twindow->x - border_x : 0;
   twindow->min_y =
      twindow->y - border_y >= 0 &&
      twindow->y + twindow->height <= t->height
      ? twindow->y - border_y : 0;
}

static void mode_topdown_tilemap_draw_tilemap( struct TWINDOW* twindow ) {
   struct CLIENT* local_client = NULL;
   struct MOBILE* o = NULL;
   struct TILEMAP* t = NULL;

   local_client = scaffold_container_of( twindow, struct CLIENT, local_window );
   o = local_client->puppet;
   t = local_client->active_tilemap;

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
      vector_iterate( &(t->layers), mode_topdown_tilemap_draw_layer_cb, twindow );
   } else if(
      TILEMAP_REDRAW_DIRTY == t->redraw_state &&
      0 < vector_count( &(t->dirty_tiles ) )
   ) {
      /* Just redraw dirty tiles. */
      vector_iterate(
         &(t->dirty_tiles), mode_topdown_tilemap_draw_tile_cb, twindow
      );
   }

   /* If we've done a full redraw as requested then switch back to just dirty *
    * tiles.                                                                  */
   if(
      0 != o->steps_remaining &&
      TILEMAP_REDRAW_ALL != t->redraw_state
   ) {
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );
   } else if(
      0 == o->steps_remaining &&
      TILEMAP_REDRAW_DIRTY != t->redraw_state
   ) {
      tilemap_set_redraw_state( t, TILEMAP_REDRAW_DIRTY );
   }
}

void mode_topdown_draw(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   mode_topdown_tilemap_draw_tilemap( &(c->local_window) );
   vector_iterate( l->mobiles, mode_topdown_draw_mobile_cb, &(c->local_window) );
}

void mode_topdown_update(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   struct MOBILE* o = NULL;
   o = client_get_puppet( c );
   if( NULL == o ) {
      return;
   }
   mode_topdown_tilemap_update_window(
      &(c->local_window), o->x, o->y
   );
}

static BOOL mode_topdown_poll_keyboard( struct CLIENT* c, struct INPUT* p ) {
   struct MOBILE* puppet = NULL;
   struct MOBILE_UPDATE_PACKET update;
   struct UI* ui = NULL;
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;
   struct CHANNEL* l = NULL;
   struct TILEMAP* t = NULL;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   puppet = client_get_puppet( c );

   /* Make sure the buffer that all windows share is available. */
   if(
      NULL == puppet ||
      (puppet->steps_remaining < -8 || puppet->steps_remaining > 8)
   ) {
      /* TODO: Handle limited input while loading. */
      input_clear_buffer( p );
      return FALSE; /* Silently ignore input until animations are done. */
   } else {
      ui = c->ui;
      update.o = puppet;
      update.l = puppet->channel;
      scaffold_check_null( update.l );
      l = puppet->channel;
      scaffold_check_null_msg( l, "No channel loaded." );
      t = l->tilemap;
      scaffold_check_null_msg( t, "No tilemap loaded." );
   }

   /* If no windows need input, then move on to game input. */
   switch( p->character ) {
   case INPUT_ASSIGNMENT_QUIT: proto_client_stop( c ); return TRUE;
   case INPUT_ASSIGNMENT_UP:
      update.update = MOBILE_UPDATE_MOVEUP;
      update.x = puppet->x;
      update.y = puppet->y - 1;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_LEFT:
      update.update = MOBILE_UPDATE_MOVELEFT;
      update.x = puppet->x - 1;
      update.y = puppet->y;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_DOWN:
      update.update = MOBILE_UPDATE_MOVEDOWN;
      update.x = puppet->x;
      update.y = puppet->y + 1;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_RIGHT:
      update.update = MOBILE_UPDATE_MOVERIGHT;
      update.x = puppet->x + 1;
      update.y = puppet->y;
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
   return FALSE;
}

void mode_topdown_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p ) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         mode_topdown_poll_keyboard( c, p );
      }
   }
   return;
}

void mode_topdown_free( struct CLIENT* c ) {
   if(
      TRUE == client_is_local( c ) &&
      hashmap_is_valid( &(c->sprites ) )
   ) {
      /* FIXME: This causes crash on re-login. */
      //hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }
}
