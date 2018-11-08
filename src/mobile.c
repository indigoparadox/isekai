
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"
#include "callback.h"
#include "datafile.h"
#include "vm.h"
#include "backlog.h"
#include "channel.h"
#include "ipc.h"
#include "files.h"
#include "twindow.h"

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
      lg_debug(
         __FILE__,
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

   lg_debug(
      __FILE__,
      "Mobile %b (%d) destroyed.\n",
      o->display_name, o->serial
   );

   bdestroy( o->display_name );
   mem_free( o );
}

void mobile_free( struct MOBILE* o ) {
   refcount_dec( o, "mobile" );
}

void mobile_animation_free( struct MOBILE_ANI_DEF* animation ) {
   vector_cleanup_force( &(animation->frames) );
   bdestroy( animation->name );
}

void mobile_init(
   struct MOBILE* o, const bstring mob_id, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
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
   o->owner = NULL;
   o->animation_reset = FALSE;
#ifdef USE_TURNS
   o->vm_tick_prev = 0;
#endif /* USE_TURNS */

#ifdef USE_VM
   vm_caddy_new( o->vm_caddy );
#endif /* USE_VM */

   vector_init( &(o->sprite_defs) );
   hashmap_init( &(o->ani_defs) );
   hashmap_init( &(o->script_defs) );
   vector_init( &(o->items) );

   o->x = x;
   o->prev_x = x;
   o->y = y;
   o->prev_y = y;
   o->mob_id = bstrcpy( mob_id );

   /* We always need the filename to fetch the file with a chunker. */
   o->def_filename = bfromcstr( "mobs" );
   lgc_null( o->def_filename );
   files_join_path( o->def_filename, o->mob_id );

   lg_debug( __FILE__, "scaffold_error: %d\n", lgc_error );
   lgc_nonzero( lgc_error );

#ifdef USE_EZXML
   bstr_ret = bcatcstr( o->def_filename, ".xml" );
   lgc_nonzero( bstr_ret );
#endif /* USE_EZXML */

cleanup:
   return;
}

void mobile_load_local( struct MOBILE* o ) {

   /* Refactored this out of the init because all mobiles need an ID, but
    * client-side mobiles don't need to load from the file system.
    */

   size_t bytes_read = 0,
      mobdata_size = 0;
   BYTE* mobdata_buffer = NULL;
   bstring mobdata_path = NULL;

   scaffold_assert( NULL != o );
   scaffold_assert( NULL != o->mob_id );
   scaffold_assert( NULL != o->def_filename );

   mobdata_path = files_root( o->def_filename );

#ifdef USE_EZXML

   /* TODO: Support other mobile formats. */
   lg_debug(
      __FILE__, "Looking for XML data in: %s\n", bdata( mobdata_path )
   );
   bytes_read = files_read_contents(
      mobdata_path, &mobdata_buffer, &mobdata_size
   );
   lgc_null_msg( mobdata_buffer, "Unable to load mobile data." );
   lgc_zero_msg( bytes_read, "Unable to load mobile data." );

   datafile_parse_ezxml_string(
      o, mobdata_buffer, mobdata_size, FALSE, DATAFILE_TYPE_MOBILE, mobdata_path
   );

#endif /* USE_EZXML */

   o->initialized = TRUE;

cleanup:
   bdestroy( mobdata_path );
   if( NULL != mobdata_buffer ) {
      mem_free( mobdata_buffer );
   }
   return;
}

#ifdef ENABLE_LOCAL_CLIENT

static GFX_COORD_PIXEL mobile_calculate_terrain_sprite_height(
   struct TILEMAP* t, GFX_COORD_PIXEL sprite_height_in,
   TILEMAP_COORD_TILE x_2, TILEMAP_COORD_TILE y_2
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
      vector_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_end );

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      lgc_null( tile_iter );

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
      vector_cleanup_force( tiles_end );
      mem_free( tiles_end );
   }
   return sprite_height_out;
}

void mobile_animate( struct MOBILE* o ) {

   if( NULL == o || NULL == o->current_animation ) {
      goto cleanup;
   }

   if( o->current_frame >= vector_count( &(o->current_animation->frames) ) - 1 ) {
      o->current_frame = 0;
   } else {
      o->current_frame++;
   }

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

      if(
         (0 < o->steps_inc && MOBILE_STEPS_HALF > o->steps_remaining) ||
         (0 > o->steps_inc && (-1 * MOBILE_STEPS_HALF) < o->steps_remaining)
      ) {
         o->sprite_display_height =
            mobile_calculate_terrain_sprite_height(
               o->channel->tilemap, o->sprite_height,
               o->x, o->y );
      }

      if( 0 == o->steps_remaining ) {
         /* This could have been set to zero by the increment above. */
         o->prev_x = o->x;
         o->prev_y = o->y;
      }
   }

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

void mobile_draw_ortho( struct MOBILE* o, struct CLIENT* local_client, struct TWINDOW* twindow ) {
   GFX_COORD_PIXEL
      pix_x,
      pix_y,
      steps_remaining_x,
      steps_remaining_y;
   struct MOBILE_SPRITE_DEF* current_frame = NULL;
   struct MOBILE* o_player = NULL;
   GRAPHICS_RECT sprite_rect;
   struct TILEMAP* active_tilemap = NULL;

   scaffold_assert_client();

   if( NULL == o || NULL == o->sprites_filename || NULL == o->sprites ) {
      return;
   }

   if(
      o->x > twindow_get_max_x( twindow ) ||
      o->y > twindow_get_max_y( twindow ) ||
      o->x < twindow_get_min_x( twindow ) ||
      o->y < twindow_get_min_y( twindow ) ||
      NULL == o->current_animation
   ) {
      goto cleanup;
   }

   /* Figure out the window position to draw to. */
   /* TODO: Support variable sprite size. */
   /* TODO: This should use tilemap tile size. */
   pix_x = (MOBILE_SPRITE_SIZE * (o->x - twindow_get_x( twindow )));
   pix_y = (MOBILE_SPRITE_SIZE * (o->y - twindow_get_y( twindow )));

   o_player = client_get_puppet( local_client );

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
   lgc_null( current_frame );

   sprite_rect.w = o->sprite_width;
   sprite_rect.h = o->sprite_height;
   graphics_get_spritesheet_pos_ortho(
      o->sprites, &sprite_rect, current_frame->id
   );

   /* Add dirty tiles to list before drawing. */
   active_tilemap = client_get_channel_active( local_client )->tilemap;
   tilemap_add_dirty_tile( active_tilemap, o->x, o->y );
   tilemap_add_dirty_tile( active_tilemap, o->prev_x, o->prev_y );

   graphics_blit_partial(
      twindow_get_screen( twindow ),
      pix_x, pix_y,
      sprite_rect.x, sprite_rect.y,
      o->sprite_width, o->sprite_display_height,
      o->sprites
   );

cleanup:
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

void mobile_set_channel( struct MOBILE* o, struct CHANNEL* l ) {
   if( NULL != o->channel ) {
      channel_free( o->channel );
      lg_debug(
         __FILE__,
         "Mobile %d decreased refcount of channel %b to: %d\n",
         o->serial, l->name, l->refcount.count
      );
   }
   o->channel = l;
   if( NULL != o->channel ) {
      refcount_inc( l, "channel" );
      lg_debug(
         __FILE__,
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
   TILEMAP_COORD_TILE x_1, TILEMAP_COORD_TILE y_1, TILEMAP_COORD_TILE x_2, TILEMAP_COORD_TILE y_2
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
      vector_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_end );
   if( NULL == tiles_end ) {
#ifdef DEBUG_VERBOSE
      lg_error(
         __FILE__, "No tileset loaded; unable to process move.\n" );
#endif /* DEBUG_VERBOSE */
      update_out = MOBILE_UPDATE_NONE;
      goto cleanup;
   }

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      lgc_null( tile_iter );

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
      vector_cleanup_force( tiles_end );
      mem_free( tiles_end );
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
   const struct CHANNEL* l, MOBILE_UPDATE update_in,
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
      // XXX: NOLOCK
      vector_iterate( l->mobiles, callback_search_mobs_by_pos, &pos );

   if( NULL != o_test ) {
      /* TODO: Default to something else for friendlies? */
      /* update_out = MOBILE_UPDATE_ATTACK; */
      update_out = MOBILE_UPDATE_NONE;
   }

cleanup:
   return update_out;
}

static GFX_COORD_PIXEL mobile_calculate_terrain_steps_inc(
   struct TILEMAP* t, GFX_COORD_PIXEL steps_inc_in,
   TILEMAP_COORD_TILE x_2, TILEMAP_COORD_TILE y_2
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
      vector_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_end );

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      lgc_null( tile_iter );

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
      vector_cleanup_force( tiles_end );
      mem_free( tiles_end );
   }
   return steps_inc_out;
}

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

   switch( update->update ) {
   case MOBILE_UPDATE_MOVEUP:
      if( (update->x != o->x || update->y != o->y - 1) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_terrain_result( l->tilemap, update->update,
            o->x, o->y, update->x, update->y )) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_mobile_result( l, update->update,
            o->x, o->y, update->x, update->y ))
      ) {
         goto cleanup;
      }
      o->prev_x = o->x; /* Forceful reset. */
      o->y = update->y;
      o->x = update->x;
      o->facing = MOBILE_FACING_UP;
      /* We'll calculate the actual animation frames to use in the per-mode
       * update() function, where we have access to the current camera rotation
       * and stuff like that. */
      mobile_call_reset_animation( o );
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            l->tilemap, o->steps_inc_default,
            o->x, o->y ) * -1;
      if( FALSE != instant ) {
         o->prev_y = o->y;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_MOVEDOWN:
      if( (update->x != o->x || update->y != o->y + 1) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_terrain_result( l->tilemap, update->update,
            o->x, o->y, update->x, update->y )) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_mobile_result( l, update->update,
            o->x, o->y, update->x, update->y ))
      ) {
         goto cleanup;
      }
      o->prev_x = o->x; /* Forceful reset. */
      o->y = update->y;
      o->x = update->x;
      o->facing = MOBILE_FACING_DOWN;
      /* We'll calculate the actual animation frames to use in the per-mode
       * update() function, where we have access to the current camera rotation
       * and stuff like that. */
      o->animation_reset = TRUE;
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            l->tilemap, o->steps_inc_default,
            o->x, o->y );
      if( FALSE != instant ) {
         o->prev_y = o->y;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX * -1;
      }
      break;

   case MOBILE_UPDATE_MOVELEFT:
      if(
         (update->x != o->x - 1 || update->y != o->y) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_terrain_result( l->tilemap, update->update,
            o->x, o->y, update->x, update->y )) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_mobile_result( l, update->update,
            o->x, o->y, update->x, update->y ))
      ) {
         goto cleanup;
      }
      o->prev_y = o->y; /* Forceful reset. */
      o->y = update->y;
      o->x = update->x;
      o->facing = MOBILE_FACING_LEFT;
      /* We'll calculate the actual animation frames to use in the per-mode
       * update() function, where we have access to the current camera rotation
       * and stuff like that. */
      o->animation_reset = TRUE;
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            l->tilemap, o->steps_inc_default,
            o->x, o->y ) * -1;
      if( FALSE != instant ) {
         o->prev_x = o->x;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_MOVERIGHT:
      if(
         (update->x != o->x + 1 || update->y != o->y) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_terrain_result( l->tilemap, update->update,
            o->x, o->y, update->x, update->y )) ||
         (MOBILE_UPDATE_NONE ==
         mobile_calculate_mobile_result( l, update->update,
            o->x, o->y, update->x, update->y ))
      ) {
         goto cleanup;
      }
      o->prev_y = o->y; /* Forceful reset. */
      o->y = update->y;
      o->x = update->x;
      o->facing = MOBILE_FACING_RIGHT;
      /* We'll calculate the actual animation frames to use in the per-mode
       * update() function, where we have access to the current camera rotation
       * and stuff like that. */
      o->animation_reset = TRUE;
      o->steps_inc =
         mobile_calculate_terrain_steps_inc(
            l->tilemap, o->steps_inc_default,
            o->x, o->y );
      if( FALSE != instant ) {
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
      && TRUE != ipc_is_local_client( o->owner->link )
#endif /* ENABLE_LOCAL_CLIENT */
   ) {
      vm_tick();
   }
#endif /* USE_TURNS */
#endif /* USE_VM */

#ifdef ENABLE_LOCAL_CLIENT
   if( FALSE == instant ) {
      /* Local Client */
      /* o->sprite_display_height =
         mobile_calculate_terrain_sprite_height(
            l->tilemap, o->sprite_height,
            o->x, o->y ); */
   } else {
#endif /* ENABLE_LOCAL_CLIENT */
      /* Server */
      hashmap_iterate( l->clients, callback_send_updates_to_client, update );
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

   lgc_null_msg(
      o->channel, "Mobile without channel cannot speak.\n"
   );

   if( NULL != o->owner ) {
      line_nick = client_get_nick( o->owner );
   } else {
      line_nick = o->display_name;
   }
   scaffold_assert( NULL != line_nick );

#ifndef DISABLE_BACKLOG
   backlog_speak( line_nick, speech );
#endif /* !DISABLE_BACKLOG */

cleanup:
   return;
}

#ifdef ENABLE_LOCAL_CLIENT

/** \return TRUE if the mobile is owned by the client that is currently
 *          locally connected in a hosted coop or single-player instance.
 */
SCAFFOLD_INLINE
BOOL mobile_is_local_player( struct MOBILE* o, struct CLIENT* c ) {
   if( NULL != o->owner && 0 == bstrcmp( client_get_nick( o->owner ), client_get_nick( c ) ) ) {
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

#ifdef USE_ITEMS

void mobile_add_item( struct MOBILE* o, struct ITEM* e ) {
   vector_add( &(o->items), e );
}

#endif // USE_ITEMS

struct CHANNEL* mobile_get_channel( struct MOBILE* o ) {
   if( NULL == o ) {
      return NULL;
   }
   return o->channel;
}

/** \brief The next time the mobile is drawn, start from the first frame of the
 *         animation named by its current_animation. Useful when changing
 *         directions or attacking.
 */
void mobile_call_reset_animation( struct MOBILE* o ) {
   o->animation_reset = TRUE;
}

/** \brief Perform a requested reset in a 2D (topdown/iso) mode. */
void mobile_do_reset_2d_animation( struct MOBILE* o ) {
   bstring ani_key_buffer = NULL;

   /* A mobile should almost always be playing an animation (walking, if      *
    * nothing else).                                                          */
   if( NULL != o->current_animation ) {
      /* The mobile should play an animation, so build its hash key. */
      ani_key_buffer = bformat(
         "%s-%s",
         bdata( o->current_animation->name ),
         str_mobile_facing[o->facing].data
      );
      scaffold_assert( NULL != ani_key_buffer );
      /* Grab the animation from the mobiles linked animation list, if any. */
      if( NULL != hashmap_get( &(o->ani_defs), ani_key_buffer ) ) {
         o->current_animation = hashmap_get( &(o->ani_defs), ani_key_buffer );
      }
   }

   /* No more need to reset. It's done. */
   o->animation_reset = FALSE;

/* cleanup: */
   bdestroy( ani_key_buffer );
}

#ifdef USE_VM

void mobile_vm_start( struct MOBILE* o ) {

   lgc_null( o->vm_caddy );

   lg_debug(
      __FILE__, "Starting script VM for mobile: %d (%b)\n",
      o->serial, o->mob_id
   );

   /* Setup the script caddy. */
   ((struct VM_CADDY*)o->vm_caddy)->exec_start = 0;
   ((struct VM_CADDY*)o->vm_caddy)->caller = o;
   ((struct VM_CADDY*)o->vm_caddy)->caller_type = VM_CALLER_MOBILE;

   vm_caddy_start( o->vm_caddy );

cleanup:
   return;
}

#endif /* USE_VM */
