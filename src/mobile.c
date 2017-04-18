
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"
#include "hashmap.h"
#include "callback.h"
#include "datafile.h"
#include "vm.h"

#ifdef USE_MOBILE_FRAME_COUNTER
static uint8_t mobile_frame_counter = 0;
#endif /* USE_MOBILE_FRAME_COUNTER */
#ifdef USE_MOBILE_MOVE_COUNTER
static uint8_t mobile_move_counter = 0;
#endif /* USE_MOBILE_MOVE_COUNTER */

static void mobile_cleanup( const struct REF* ref ) {
   struct MOBILE* o = scaffold_container_of( ref, struct MOBILE, refcount );

#ifdef ENABLE_LOCAL_CLIENT
   if( NULL != o->sprites ) {
      graphics_surface_free( o->sprites );
   }
#endif /* ENABLE_LOCAL_CLIENT */
   if( NULL != o->owner ) {
      client_clear_puppet( o->owner );
      o->owner = NULL;
   }
   if( NULL != o->channel ) {
      channel_free( o->channel );
      scaffold_print_debug(
         &module,
         "Mobile %d decreased refcount of channel %b to: %d\n",
         o->serial, o->channel->name, o->channel->refcount.count
      );
      o->channel = NULL;
   }

   vector_remove_cb( &(o->sprite_defs), callback_free_generic, NULL );
   vector_cleanup( &(o->sprite_defs) );

   /* vector_remove_cb( &(o->speech_backlog), callback_free_strings, NULL );
   vector_cleanup( &(o->speech_backlog) ); */

   hashmap_remove_cb( &(o->ani_defs), callback_free_ani_defs, NULL );
   hashmap_cleanup( &(o->ani_defs) );

   hashmap_remove_cb( &(o->vm_scripts), callback_free_strings, NULL );
   hashmap_cleanup( &(o->vm_scripts) );

   hashmap_remove_cb( &(o->vm_globals), callback_free_strings, NULL );
   hashmap_cleanup( &(o->vm_globals) );

   scaffold_print_debug(
      &module,
      "Mobile %b (%d) destroyed.\n",
      o->display_name, o->serial
   );

   bdestroy( o->display_name );
   scaffold_free( o );
}

void mobile_free( struct MOBILE* o ) {
   refcount_dec( o, "mobile" );
}

void mobile_init(
   struct MOBILE* o, const bstring mob_id, GFX_COORD_TILE x, GFX_COORD_TILE y
) {
   int bstr_ret = 0;

   ref_init( &(o->refcount), mobile_cleanup );

   scaffold_assert( NULL != mob_id );

   o->sprites_filename = NULL;
   o->serial = 0;
   o->channel = NULL;
   o->display_name = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   o->current_frame = 0;
   o->steps_inc_default = MOBILE_STEPS_INCREMENT;
   o->vm = NULL;
   o->owner = NULL;
#ifdef USE_TURNS
   o->vm_tick_prev = 0;
#endif /* USE_TURNS */

   vector_init( &(o->sprite_defs) );
   hashmap_init( &(o->ani_defs) );
   hashmap_init( &(o->script_defs) );
   hashmap_init( &(o->vm_scripts) );
   hashmap_init( &(o->vm_globals) );

   o->x = x;
   o->prev_x = x;
   o->y = y;
   o->prev_y = y;
   o->mob_id = bstrcpy( mob_id );

   /* We always need the filename to fetch the file with a chunker. */
   o->def_filename = bfromcstr( "mobs" );
   scaffold_check_null( o->def_filename );
   scaffold_join_path( o->def_filename, o->mob_id );
   scaffold_check_nonzero( scaffold_error );

#ifdef USE_EZXML
   bstr_ret = bcatcstr( o->def_filename, ".xml" );
   scaffold_check_nonzero( bstr_ret );
#endif /* USE_EZXML */

cleanup:
   return;
}

void mobile_load_local( struct MOBILE* o ) {

   /* Refactored this out of the init because all mobiles need an ID, but
    * client-side mobiles don't need to load from the file system.
    */

   SCAFFOLD_SIZE bytes_read = 0,
      mobdata_size = 0;
   BYTE* mobdata_buffer = NULL;
   bstring mobdata_path = NULL;

   scaffold_assert( NULL != o );
   scaffold_assert( NULL != o->mob_id );
   scaffold_assert( NULL != o->def_filename );

   mobdata_path = bstrcpy( &str_server_data_path );
   scaffold_check_null( mobdata_path );
   scaffold_join_path( mobdata_path, o->def_filename );
   scaffold_check_nonzero( scaffold_error );

#ifdef USE_EZXML

   /* TODO: Support other mobile formats. */
   scaffold_print_info(
      &module, "Loading for XML data in: %s\n", bdata( mobdata_path )
   );
   bytes_read = scaffold_read_file_contents(
      mobdata_path, &mobdata_buffer, &mobdata_size
   );
   scaffold_check_null_msg( mobdata_buffer, "Unable to load mobile data." );
   scaffold_check_zero_msg( bytes_read, "Unable to load mobile data." );

   datafile_parse_ezxml_string(
      o, mobdata_buffer, mobdata_size, FALSE, DATAFILE_TYPE_MOBILE, mobdata_path
   );

#endif /* USE_EZXML */

   o->initialized = TRUE;

cleanup:
   bdestroy( mobdata_path );
   if( NULL != mobdata_buffer ) {
      scaffold_free( mobdata_buffer );
   }
   return;
}

void mobile_animate( struct MOBILE* o ) {
#ifdef USE_MOBILE_FRAME_COUNTER
   if( mobile_frame_counter++ > MOBILE_FRAME_DIVISOR ) {
      mobile_frame_counter = 0;
   }
   if( 0 == mobile_frame_counter ) {
#endif /* USE_MOBILE_FRAME_COUNTER */
   if( NULL != o && NULL != o->current_animation ) {
      if( o->current_frame >= vector_count( &(o->current_animation->frames) ) - 1 ) {
         o->current_frame = 0;
      } else {
         o->current_frame++;
      }
   }
#ifdef USE_MOBILE_FRAME_COUNTER
   }
#endif /* USE_MOBILE_FRAME_COUNTER */

#ifdef USE_MOBILE_MOVE_COUNTER
   if( mobile_move_counter++ > MOBILE_MOVE_DIVISOR ) {
      mobile_move_counter = 0;
   }
   if( 0 == mobile_move_counter ) {
#endif /* USE_MOBILE_MOVE_COUNTER */
   if( NULL != o ) {
      /* TODO: Enforce walking speed server-side. */
      if( 0 != o->steps_remaining ) {
         o->steps_remaining += o->steps_inc;
         /* Clamp to zero for odd increments. */
         if(
            (0 < o->steps_inc && 0 < o->steps_remaining) ||
            (0 > o->steps_inc && 0 > o->steps_remaining)
         ) {
            o->steps_remaining = 0;
         }
      } else {
         o->prev_x = o->x;
         o->prev_y = o->y;
      }
   }
#ifdef USE_MOBILE_MOVE_COUNTER
   }
#endif /* USE_MOBILE_MOVE_COUNTER */
}

#ifdef ENABLE_LOCAL_CLIENT

SCAFFOLD_INLINE void mobile_get_spritesheet_pos_ortho(
   struct MOBILE* o, SCAFFOLD_SIZE gid,
   GFX_COORD_PIXEL* x, GFX_COORD_PIXEL* y
) {
   GFX_COORD_TILE tiles_wide = 0;

   scaffold_check_null( o->sprites );

   tiles_wide = o->sprites->w / o->sprite_width;

   *y = ((gid) / tiles_wide) * o->sprite_height;
   *x = ((gid) % tiles_wide) * o->sprite_width;

cleanup:
   return;
}

SCAFFOLD_INLINE
GFX_COORD_PIXEL
mobile_get_steps_remaining_x( const struct MOBILE* o, BOOL reverse ) {
   GFX_COORD_PIXEL steps_out = 0;
   if( o->prev_x != o->x ) {
      if( TRUE != reverse ) {
         steps_out = o->steps_remaining;
      } else {
         steps_out = -1 * o->steps_remaining;
      }
   }
   return steps_out;
}

SCAFFOLD_INLINE
GFX_COORD_PIXEL
mobile_get_steps_remaining_y( const struct MOBILE* o, BOOL reverse ) {
   GFX_COORD_PIXEL steps_out = 0;
   if( o->prev_y != o->y ) {
      if( TRUE != reverse ) {
         steps_out = o->steps_remaining;
      } else {
         steps_out = -1 * o->steps_remaining;
      }
   }
   return steps_out;
}

void mobile_draw_ortho( struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow ) {
   GFX_COORD_PIXEL
      sprite_x,
      sprite_y,
      pix_x,
      pix_y,
      steps_remaining_x,
      steps_remaining_y;
   struct MOBILE_SPRITE_DEF* current_frame = NULL;
   struct MOBILE* o_player = NULL;
   struct CLIENT* local_client = twindow->local_client;

#ifdef DEBUG_TILES
   bstring bnum = NULL;
#endif /* DEBUG_TILES */

   scaffold_assert_client();

   if( NULL == o || NULL == o->sprites_filename ) {
      return;
   }

   if(
      o->x > twindow->max_x ||
      o->y > twindow->max_y ||
      o->x < twindow->min_x ||
      o->y < twindow->min_y ||
      NULL == o->current_animation
   ) {
      goto cleanup;
   }

   /* If the current mobile spritesheet doesn't exist, then load it. */
   if(
      NULL == o->sprites &&
      NULL == hashmap_get( &(local_client->sprites), o->sprites_filename )
   ) {
      /* No sprites and no request yet, so make one! */
      client_request_file( local_client, CHUNKER_DATA_TYPE_MOBSPRITES, o->sprites_filename );
      goto cleanup;
   } else if(
      NULL == o->sprites &&
      NULL != hashmap_get( &(local_client->sprites), o->sprites_filename )
   ) {
      o->sprites = (GRAPHICS*)hashmap_get( &(local_client->sprites), o->sprites_filename );
   } else if( NULL == o->sprites ) {
      /* Sprites must not be ready yet. */
      goto cleanup;
   }

   /* Figure out the window position to draw to. */
   /* TODO: Support variable sprite size. */
   /* TODO: This should use tilemap tile size. */
   pix_x = (MOBILE_SPRITE_SIZE * (o->x - (twindow->x)));
   pix_y = (MOBILE_SPRITE_SIZE * (o->y - (twindow->y)));

   o_player = local_client->puppet;

   if(
      mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_x( o->x + 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_x( o->x - 1, twindow ))
   ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, FALSE );
      pix_x += steps_remaining_x;
   } else if( !mobile_is_local_player( o, local_client ) ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, FALSE );
      pix_x += steps_remaining_x;
   }

   if(
      mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_y( o->y + 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_window_deadzone_y( o->y - 1, twindow ))
   ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o, FALSE );
      pix_y += steps_remaining_y;
   } else if( !mobile_is_local_player( o, local_client ) ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o, FALSE );
      pix_y += steps_remaining_y;
   }

   /* Offset the player's  movement. */
   if(
      !mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_x( o_player->x + 1, twindow ) ||
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_inner_map_x( o_player->x - 1, twindow ))
   ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o_player, TRUE );
      pix_x += steps_remaining_x;
   }

   if(
      !mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_y( o_player->y + 1, twindow ) ||
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_y( o_player->y - 1, twindow ))
   ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o_player, TRUE );
      pix_y += steps_remaining_y;
   }

   /* Figure out the graphical sprite to draw from. */
   /* TODO: Support varied spritesheets. */
   current_frame = (struct MOBILE_SPRITE_DEF*)
      vector_get( &(o->current_animation->frames), o->current_frame );
   scaffold_check_null( current_frame );

   mobile_get_spritesheet_pos_ortho(
      o, current_frame->id, &sprite_x, &sprite_y
   );

   /* Add dirty tiles to list before drawing. */
   tilemap_add_dirty_tile( twindow->t, o->x, o->y );
   tilemap_add_dirty_tile( twindow->t, o->prev_x, o->prev_y );

   graphics_blit_partial(
      twindow->g,
      pix_x, pix_y,
      sprite_x, sprite_y,
      o->sprite_width, o->sprite_display_height,
      o->sprites
   );

   /*
   if( mobile_is_local_player( o ) ) {
      graphics_draw_rect( twindow->g, pix_x, pix_y, o->sprite_width, o->sprite_height, GRAPHICS_COLOR_BLUE );
   }
   */

#if 0
   /* FIXME */
   pix_y -= o->sprite_height;

   if( 0 < vector_count( &(o->speech_backlog) ) ) {
      graphics_set_color( twindow->g, GRAPHICS_COLOR_CYAN);
      graphics_draw_text(
         twindow->g, pix_x, pix_y, GRAPHICS_TEXT_ALIGN_CENTER,
         (bstring)vector_get( &(o->speech_backlog), 0 )
      );
   }
#endif
cleanup:
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

void mobile_set_channel( struct MOBILE* o, struct CHANNEL* l ) {
   if( NULL != o->channel ) {
      channel_free( o->channel );
      scaffold_print_debug(
         &module,
         "Mobile %d decreased refcount of channel %b to: %d\n",
         o->serial, l->name, l->refcount.count
      );
   }
   o->channel = l;
   if( NULL != o->channel ) {
      refcount_inc( l, "channel" );
      scaffold_print_debug(
         &module,
         "Mobile %d increased refcount of channel %b to: %d\n",
         o->serial, l->name, l->refcount.count
      );
   }
}

/** \brief Calculate the movement success between two tiles on the given
 *         mobile's present tilemap based on all available layers.
 * \param[in] x_1 Starting X.
 * \param[in] y_1 Starting Y.
 * \param[in] x_2 Finishing X.
 * \param[in] y_2 Finishing Y.
 * \return A MOBILE_UPDATE indicating action resulting.
 */
static MOBILE_UPDATE mobile_calculate_terrain_result(
   struct TILEMAP* t, MOBILE_UPDATE update_in,
   GFX_COORD_TILE x_1, GFX_COORD_TILE y_1, GFX_COORD_TILE x_2, GFX_COORD_TILE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   MOBILE_UPDATE update_out = update_in;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   uint8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;

   if( MOBILE_UPDATE_NONE == update_in ) {
      update_out = MOBILE_UPDATE_NONE;
      goto cleanup;
   }

   if( x_2 >= t->width || y_2 >= t->height || x_2 < 0 || y_2 < 0 ) {
      update_out = MOBILE_UPDATE_NONE;
      goto cleanup;
   }

   pos_end.x = x_2;
   pos_end.y = y_2;

   /* Fetch the source tile on all layers. */
   tiles_end =
      hashmap_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_end );
   if( NULL == tiles_end ) {
      scaffold_print_error(
         &module, "No tileset loaded; unable to process move.\n" );
      update_out = MOBILE_UPDATE_NONE;
      goto cleanup;
   }

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      scaffold_check_null( tile_iter );

      for( j = 0 ; 4 > j ; j++ ) {
         /* TODO: Implement terrain slow-down. */
         terrain_iter = tile_iter->terrain[j];
         if( NULL == terrain_iter ) { continue; }
         if( TILEMAP_MOVEMENT_BLOCK == terrain_iter->movement ) {
            update_out = MOBILE_UPDATE_NONE;
         }
      }
   }

cleanup:
   if( NULL != tiles_end ) {
      /* Force the count to 0 so we can delete it. */
      tiles_end->count = 0;
      vector_cleanup( tiles_end );
      scaffold_free( tiles_end );
   }
   return update_out;
}

/** \brief Calculate the movement success between two tiles on the given
 *         mobile's present tilemap based on neighboring mobiles.
 * \param[in] x_1 Starting X.
 * \param[in] y_1 Starting Y.
 * \param[in] x_2 Finishing X.
 * \param[in] y_2 Finishing Y.
 * \return A MOBILE_UPDATE indicating action resulting.
 */
static MOBILE_UPDATE mobile_calculate_mobile_result(
   struct CHANNEL* l, MOBILE_UPDATE update_in,
   SCAFFOLD_SIZE x_1, SCAFFOLD_SIZE y_1, SCAFFOLD_SIZE x_2, SCAFFOLD_SIZE y_2
) {
   struct TILEMAP_POSITION pos;
   struct MOBILE* o_test = NULL;
   MOBILE_UPDATE update_out = update_in;

   if( MOBILE_UPDATE_NONE == update_in ) {
      update_out = MOBILE_UPDATE_NONE;
      goto cleanup;
   }

   pos.x = x_2;
   pos.y = y_2;

   o_test =
      vector_iterate_nolock( &(l->mobiles), callback_search_mobs_by_pos, &pos );

   if( NULL != o_test ) {
      /* TODO: Default to something else for friendlies? */
      update_out = MOBILE_UPDATE_ATTACK;
   }

cleanup:
   return update_out;
}

static GFX_COORD_PIXEL mobile_calculate_terrain_steps_inc(
   struct TILEMAP* t, GFX_COORD_PIXEL steps_inc_in,
   GFX_COORD_TILE x_2, GFX_COORD_TILE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   uint8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;
   GFX_COORD_PIXEL steps_inc_out = steps_inc_in;

   pos_end.x = x_2;
   pos_end.y = y_2;

   /* Fetch the destination tile on all layers. */
   tiles_end =
      hashmap_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_end );

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      scaffold_check_null( tile_iter );

      for( j = 0 ; 4 > j ; j++ ) {
         /* TODO: Implement terrain slow-down. */
         terrain_iter = tile_iter->terrain[j];
         if( NULL == terrain_iter ) { continue; }
         if(
            terrain_iter->movement != 0 &&
            steps_inc_in / terrain_iter->movement < steps_inc_out
         ) {
            steps_inc_out = steps_inc_in / terrain_iter->movement;
         }
      }
   }

cleanup:
   if( NULL != tiles_end ) {
      /* Force the count to 0 so we can delete it. */
      tiles_end->count = 0;
      vector_cleanup( tiles_end );
      scaffold_free( tiles_end );
   }
   return steps_inc_out;
}

#ifdef ENABLE_LOCAL_CLIENT

static GFX_COORD_PIXEL mobile_calculate_terrain_sprite_height(
   struct TILEMAP* t, GFX_COORD_PIXEL sprite_height_in,
   GFX_COORD_TILE x_2, GFX_COORD_TILE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   uint8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;
   SCAFFOLD_SIZE sprite_height_out = sprite_height_in;

   pos_end.x = x_2;
   pos_end.y = y_2;

   scaffold_assert( TILEMAP_SENTINAL == t->sentinal );

   /* Fetch the destination tile on all layers. */
   tiles_end =
      hashmap_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_end );

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      scaffold_check_null( tile_iter );

      for( j = 0 ; 4 > j ; j++ ) {
         /* TODO: Implement terrain slow-down. */
         terrain_iter = tile_iter->terrain[j];
         if( NULL == terrain_iter ) { continue; }
         if(
            terrain_iter->cutoff != 0 &&
            sprite_height_in / terrain_iter->cutoff < sprite_height_out
         ) {
            sprite_height_out = sprite_height_in / terrain_iter->cutoff;
         }
      }
   }

cleanup:
   if( NULL != tiles_end ) {
      /* Force thMOBILE_SPRITE_SIZEe count to 0 so we can delete it. */
      tiles_end->count = 0;
      vector_cleanup( tiles_end );
      scaffold_free( tiles_end );
   }
   return sprite_height_out;
}

#endif /* ENABLE_LOCAL_CLIENT */

/** \brief Apply an update received from a remote client to a local mobile.
 * \param[in] update    Packet containing update information.
 * \param[in] instant   A BOOL indicating whether to force the update instantly
 *                      or allow any relevant animations to take place.
 * \return What the update becomes to send to the clients. MOBILE_UPDATE_NONE
 *         if the update failed to occur.
 */
MOBILE_UPDATE mobile_apply_update(
   struct MOBILE_UPDATE_PACKET* update, BOOL instant
) {
   struct MOBILE* o = update->o;
   struct CHANNEL* l = update->l;
   bstring animation_key = NULL;

   /* TODO: Collision detection. */
   if( TRUE == instant ) {
      /* Figure out the desired coordinates given the given update. */
      switch( update->update ) {
      case MOBILE_UPDATE_MOVEUP:
         update->x = o->x;
         update->y = o->y - 1;
         break;
      case MOBILE_UPDATE_MOVEDOWN:
         update->x = o->x;
         update->y = o->y + 1;
         break;
      case MOBILE_UPDATE_MOVELEFT:
         update->x = o->x - 1;
         update->y = o->y;
         break;
      case MOBILE_UPDATE_MOVERIGHT:
         update->x = o->x + 1;
         update->y = o->y;
         break;

      case MOBILE_UPDATE_NONE:
         break;
      }

      update->update =
         mobile_calculate_terrain_result( &(l->tilemap), update->update,
            o->x, o->y, update->x, update->y );

      update->update =
         mobile_calculate_mobile_result( l, update->update,
            o->x, o->y, update->x, update->y );
   }

   switch( update->update ) {
   case MOBILE_UPDATE_MOVEUP:
      o->prev_x = o->x; /* Forceful reset. */
      o->y--;
      o->facing = MOBILE_FACING_UP;
      mobile_set_animation_facing( o, animation_key, MOBILE_FACING_UP );
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            &(l->tilemap), o->steps_inc_default,
            o->x, o->y ) * -1;
      if( TRUE == instant ) {
         o->prev_y = o->y;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_MOVEDOWN:
      o->prev_x = o->x; /* Forceful reset. */
      o->y++;
      o->facing = MOBILE_FACING_DOWN;
      mobile_set_animation_facing( o, animation_key, MOBILE_FACING_DOWN );
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            &(l->tilemap), o->steps_inc_default,
            o->x, o->y );
      if( TRUE == instant ) {
         o->prev_y = o->y;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX * -1;
      }
      break;

   case MOBILE_UPDATE_MOVELEFT:
      o->prev_y = o->y; /* Forceful reset. */
      o->x--;
      o->facing = MOBILE_FACING_LEFT;
      mobile_set_animation_facing( o, animation_key, MOBILE_FACING_LEFT );
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            &(l->tilemap), o->steps_inc_default,
            o->x, o->y ) * -1;
      if( TRUE == instant ) {
         o->prev_x = o->x;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_MOVERIGHT:
      o->prev_y = o->y; /* Forceful reset. */
      o->x++;
      o->facing = MOBILE_FACING_RIGHT;
      mobile_set_animation_facing( o, animation_key, MOBILE_FACING_RIGHT );
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            &(l->tilemap), o->steps_inc_default,
            o->x, o->y );
      if( TRUE == instant ) {
         o->prev_x = o->x;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX * -1;
      }
      break;

   case MOBILE_UPDATE_ATTACK:
      break;

   case MOBILE_UPDATE_NONE:
      goto cleanup;
   }

#ifdef USE_VM
#ifdef USE_TURNS
   if(
      NULL != o->owner
#ifdef ENABLE_LOCAL_CLIENT
      && TRUE != o->owner->client_side
#endif /* ENABLE_LOCAL_CLIENT */
   ) {
      vm_tick();
   }
#endif /* USE_TURNS */
#endif /* USE_VM */

#ifdef ENABLE_LOCAL_CLIENT
   if( FALSE == instant ) {
      /* Local Client */
      o->sprite_display_height =
         mobile_calculate_terrain_sprite_height(
            &(l->tilemap), o->sprite_height,
            o->x, o->y );
   } else {
#endif /* ENABLE_LOCAL_CLIENT */
      /* Server */
      hashmap_iterate( &(l->clients), callback_send_updates_to_client, update );
#ifdef ENABLE_LOCAL_CLIENT
   }
#endif /* ENABLE_LOCAL_CLIENT */

cleanup:
   bdestroy( animation_key );
   return update->update;
}

/** \brief This inserts a line of speech into the local speech buffer.
 *         It should be called by AI speech routines or the receiver for
 *         network chat. See the proto_send_msg* family for sending network
 *         chat.
 * \param[in] o      The mobile to be associated with the line of speech.
 * \param[in] speech The line of speech.
 */
void mobile_speak( struct MOBILE* o, bstring speech ) {
   bstring line_nick = NULL;

   scaffold_check_null_msg(
      o->channel, "Mobile without channel cannot speak.\n"
   );

   if( NULL != o->owner ) {
      line_nick = o->owner->nick;
   } else {
      line_nick = o->display_name;
   }
   scaffold_assert( NULL != line_nick );

   channel_speak( o->channel, line_nick, speech );

cleanup:
   return;
}

#ifdef ENABLE_LOCAL_CLIENT

/** \return TRUE if the mobile is owned by the client that is currently
 *          locally connected in a hosted coop or single-player instance.
 */
SCAFFOLD_INLINE
BOOL mobile_is_local_player( struct MOBILE* o, struct CLIENT* c ) {
   if( NULL != o->owner && 0 == bstrcmp( o->owner->nick, c->nick ) ) {
      return TRUE;
   }
   return FALSE;
}

/** \return TRUE if the mobile is currently doing something (moving, etc).
 */
BOOL mobile_is_occupied( struct MOBILE* o ) {
   return NULL != o && o->steps_remaining > 0;
}

#endif /* ENABLE_LOCAL_CLIENT */
