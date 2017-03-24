
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"
#include "hashmap.h"
#include "callback.h"
#include "datafile.h"
#ifdef USE_DUKTAPE
#include "duktape/duktape.h"
#include "duktape/dukhelp.h"
#endif /* USE_DUKTAPE */

extern struct CLIENT* main_client;

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

   bdestroy( o->display_name );
   scaffold_free( o );
}

void mobile_free( struct MOBILE* o ) {
   refcount_dec( o, "mobile" );
}

void mobile_init(
   struct MOBILE* o, const bstring mob_id, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
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

   datafile_parse_mobile_ezxml_string(
      o, mobdata_buffer, mobdata_size, FALSE
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
   SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
) {
   SCAFFOLD_SIZE tiles_wide = 0;

   scaffold_check_null( o->sprites );

   tiles_wide = o->sprites->w / o->sprite_width;

   *y = ((gid) / tiles_wide) * o->sprite_height;
   *x = ((gid) % tiles_wide) * o->sprite_width;

cleanup:
   return;
}

SCAFFOLD_INLINE
SCAFFOLD_SIZE_SIGNED
mobile_get_steps_remaining_x( const struct MOBILE* o, BOOL reverse ) {
   SCAFFOLD_SIZE_SIGNED steps_out = 0;
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
SCAFFOLD_SIZE_SIGNED
mobile_get_steps_remaining_y( const struct MOBILE* o, BOOL reverse ) {
   SCAFFOLD_SIZE_SIGNED steps_out = 0;
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
   SCAFFOLD_SIZE
      sprite_x,
      sprite_y,
      pix_x,
      pix_y;
   SCAFFOLD_SIZE_SIGNED
      steps_remaining_x,
      steps_remaining_y;
   struct MOBILE_SPRITE_DEF* current_frame = NULL;
   struct MOBILE* o_player = NULL;

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
   if( NULL == o->sprites && NULL == hashmap_get( &(twindow->c->sprites), o->sprites_filename ) ) {
      /* No sprites and no request yet, so make one! */
      client_request_file( twindow->c, CHUNKER_DATA_TYPE_MOBSPRITES, o->sprites_filename );
      goto cleanup;
   } else if( NULL == o->sprites && NULL != hashmap_get( &(twindow->c->sprites), o->sprites_filename ) ) {
      o->sprites = (GRAPHICS*)hashmap_get( &(twindow->c->sprites), o->sprites_filename );
      refcount_inc( o->sprites, "spritesheet" );
   } else if( NULL == o->sprites ) {
      /* Sprites must not be ready yet. */
      goto cleanup;
   }

   /* Figure out the window position to draw to. */
   /* TODO: Support variable sprite size. */
   /* TODO: This should use tilemap tile size. */
   pix_x = (MOBILE_SPRITE_SIZE * (o->x - (twindow->x)));
   pix_y = (MOBILE_SPRITE_SIZE * (o->y - (twindow->y)));

   o_player = twindow->c->puppet;

   if(
      mobile_is_local_player( o ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_x( o->x + 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_x( o->x - 1, twindow ))
   ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, FALSE );
      pix_x += steps_remaining_x;
   } else if( !mobile_is_local_player( o ) ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, FALSE );
      pix_x += steps_remaining_x;
   }

   if(
      mobile_is_local_player( o ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_y( o->y + 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_window_deadzone_y( o->y - 1, twindow ))
   ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o, FALSE );
      pix_y += steps_remaining_y;
   } else if( !mobile_is_local_player( o ) ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o, FALSE );
      pix_y += steps_remaining_y;
   }

   /* Offset the player's  movement. */
   if(
      !mobile_is_local_player( o ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_x( o_player->x + 1, twindow ) ||
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_inner_map_x( o_player->x - 1, twindow ))
   ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o_player, TRUE );
      pix_x += steps_remaining_x;
   }

   if(
      !mobile_is_local_player( o ) &&
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
   }
   o->channel = l;
   if( NULL != o->channel ) {
      refcount_inc( l, "channel" );
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
   SCAFFOLD_SIZE x_1, SCAFFOLD_SIZE y_1, SCAFFOLD_SIZE x_2, SCAFFOLD_SIZE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   MOBILE_UPDATE update_out = update_in;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   int8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;

   if( x_2 >= t->width || y_2 >= t->height || x_2 < 0 || y_2 < 0 ) {
      update_out = MOBILE_UPDATE_NONE;
      goto cleanup;
   }

   pos_end.x = x_2;
   pos_end.y = y_2;

   /* Fetch the source tile on all layers. */
   tiles_end =
      hashmap_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_end );

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      scaffold_check_null( tile_iter );

      for( j = 0 ; 4 > j ; j++ ) {
         /* TODO: Implement terrain slow-down. */
         terrain_iter = tile_iter->terrain[j];
         if( NULL == terrain_iter ) { continue; }
         switch( terrain_iter->movement ) {
         case TILEMAP_MOVEMENT_BLOCK:
            update_out = MOBILE_UPDATE_NONE;
            break;
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

static SCAFFOLD_SIZE mobile_calculate_terrain_steps_inc(
   struct TILEMAP* t, SCAFFOLD_SIZE steps_inc_in,
   SCAFFOLD_SIZE x_2, SCAFFOLD_SIZE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   int8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;
   SCAFFOLD_SIZE steps_inc_out = steps_inc_in;

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

static SCAFFOLD_SIZE mobile_calculate_terrain_sprite_height(
   struct TILEMAP* t, SCAFFOLD_SIZE sprite_height_in,
   SCAFFOLD_SIZE x_2, SCAFFOLD_SIZE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   int8_t i, j;
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

#ifdef ENABLE_LOCAL_CLIENT

   if( FALSE == instant ) {
      o->sprite_display_height =
         mobile_calculate_terrain_sprite_height(
            &(l->tilemap), o->sprite_height,
            o->x, o->y );
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
   struct CHANNEL_BUFFER_LINE* line = NULL;
   time_t time_now;
   struct tm* time_temp = NULL;

   scaffold_check_null_msg(
      o->channel, "Mobile without channel cannot speak.\n" );

   line = (struct CHANNEL_BUFFER_LINE*)calloc(
      1, sizeof( struct CHANNEL_BUFFER_LINE)
   );
   scaffold_check_null( line );

   if( NULL != o->owner ) {
      line->nick = bstrcpy( o->owner->nick );
   }
   line->display_name = bstrcpy( o->display_name );
   line->line = bstrcpy( speech );

   /* Grab the time in a platform-agnostic way. */
   time( &time_now );
   time_temp = localtime( &time_now );
   memcpy( &(line->time), time_temp, sizeof( struct tm ) );

   vector_insert( &(o->channel->speech_backlog), 0, line );

cleanup:
   return;
}

#ifdef ENABLE_LOCAL_CLIENT

BOOL mobile_is_local_player( struct MOBILE* o ) {
   /*
   if( NULL != o->owner && 0 == bstrcmp( o->owner.nick, main_client->nick ) ) {
   */
   if( NULL != o->owner && o->owner == main_client ) {
      return TRUE;
   }
   return FALSE;
}

#endif /* ENABLE_LOCAL_CLIENT */

#ifdef USE_DUKTAPE

static void mobile_vm_debug( duk_context* vm ) {
   char* text = duk_to_string( vm, -1 );

   scaffold_print_debug( &module, "%s\n", text  );
}

static void mobile_vm_update( duk_context* vm ) {
   MOBILE_UPDATE action = (MOBILE_UPDATE)duk_to_int( vm, -1 );

   /* TODO: Call mobile_walk(); */
}

const duk_number_list_entry mobile_update_enum[] = {
   { "moveUp", MOBILE_UPDATE_MOVEUP },
   { "moveDown", MOBILE_UPDATE_MOVEDOWN },
   { "moveLeft", MOBILE_UPDATE_MOVELEFT },
   { "moveRight", MOBILE_UPDATE_MOVERIGHT },
   { "attack", MOBILE_UPDATE_ATTACK },
   { NULL, 0.0 }
};

static void mobile_vm_global_set_cb( bstring key, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)arg;
   bstring value = (bstring)iter;
   duk_idx_t idx;

   scaffold_assert( NULL != value );
   scaffold_assert( NULL != arg );

   duk_push_global_object( o->vm );
   duk_push_string( o->vm, bdata( value ) );
   duk_put_prop_string( o->vm, -2, bdata( key ) );
   duk_pop( o->vm );

   return NULL;
}

/*
static void mobile_vm_global_get_cb( bstring key, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)arg;
   bstring value = (bstring)iter;
   char* new_value = NULL;

   scaffold_assert( NULL != value );
   scaffold_assert( NULL != arg );

   //new_value = duk_require_string( o->vm, 0 );



   return NULL;
}
*/

static duk_ret_t mobile_vm_unsafe( duk_context* vm, void* udata ) {
   //SCRIPT_CADDY* caddy = (SCRIPT_CADDY*)udata;
   const char* code_c = (const char*)udata;

   duk_push_string( vm, code_c );
   duk_eval( vm );

   return 0;
}

static void mobile_vm_run( struct MOBILE* o, const bstring code ) {
   /*

   duk_push_global_object( o->vm );
   duk_push_int( o->vm, MOBILE_UPDATE_MOVEUP );
   duk_put_prop_string( o->vm, -2, "WALK_UP" );
   duk_pop( o->vm );

   duk_push_global_object( o->vm );
   duk_push_int( o->vm, MOBILE_UPDATE_MOVEDOWN );
   duk_put_prop_string( o->vm, -2, "WALK_DOWN" );
   duk_pop( o->vm );


   duk_push_global_object( o->vm );
   duk_push_int( o->vm, MOBILE_UPDATE_MOVELEFT );
   duk_put_prop_string( o->vm, -2, "WALK_LEFT" );
   duk_pop( o->vm );

   duk_push_global_object( o->vm );
   duk_push_int( o->vm, MOBILE_UPDATE_MOVERIGHT );
   duk_put_prop_string( o->vm, -2, "WALK_RIGHT" );
   duk_pop( o->vm );
   */
   const char* code_c = bdata( code );
   int duk_result = 0;

   scaffold_assert( NULL != code_c );

   duk_push_global_object( o->vm );
   duk_push_object( o->vm );
   duk_put_number_list( o->vm, -1, mobile_update_enum );
   duk_put_prop_string( o->vm, -2, "MobileUpdate" );
   duk_pop( o->vm );

   duk_push_global_object( o->vm );
   duk_push_c_function( o->vm, mobile_vm_debug, 1 );
   duk_put_prop_string( o->vm, -2, "debug" );
   duk_pop( o->vm );

   duk_push_global_object( o->vm );
   duk_push_c_function( o->vm, mobile_vm_update, 1 );
   duk_put_prop_string( o->vm, -2, "update" );
   duk_pop( o->vm );

#if 0
   //duk_peval_string( o->vm, code_c );
   //o->vm_started = TRUE;
   duk_push_string( o->vm, code_c );
   //duk_call( o->vm, -1 );
   duk_result = duk_peval( o->vm );
#endif // 0

   duk_result = duk_safe_call( o->vm, mobile_vm_unsafe, code_c, 0, 1 );

   if( 0 == duk_result ) {
      //hashmap_iterate( &(o->vm_globals), mobile_vm_global_get_cb, o );
      goto cleanup;
   }

   /* An error happened. */
   duk_get_prop_string( o->vm, 0, "name" );
   duk_get_prop_string( o->vm, 0, "message" );
   duk_get_prop_string( o->vm, 0, "fileName" );
   duk_get_prop_string( o->vm, 0, "lineNumber" );
   duk_get_prop_string( o->vm, 0, "stack" );

   scaffold_print_error(
      &module, "Script error: %s: %s (%s:%s)\n",
      duk_safe_to_string( o->vm, 1 ), duk_safe_to_string( o->vm, 2 ),
      duk_safe_to_string( o->vm, 3 ), duk_safe_to_string( o->vm, 4 )
   );
   scaffold_print_error(
      &module, "Script stack: %s\n",
      duk_safe_to_string( o->vm, 5 )
   );

   duk_pop( o->vm );

cleanup:
   return;
}

#endif /* USE_DUKTAPE */

void mobile_vm_start( struct MOBILE* o ) {
#ifdef USE_DUKTAPE
   const char* code_c = NULL;
   int duk_result = 0;

   scaffold_check_not_null( o->vm );

   o->vm_started = FALSE;
   o->vm_caddy = scaffold_alloc( 1, struct SCRIPT_CADDY );
   scaffold_check_null( o->vm_caddy );

   scaffold_print_debug(
      &module, "Starting script VM for mobile: %d (%b)\n",
      o->serial, o->mob_id
   );

   /* Setup the script caddy. */
   ((struct SCRIPT_CADDY*)o->vm_caddy)->exec_start = 0;
   ((struct SCRIPT_CADDY*)o->vm_caddy)->caller = o;
   ((struct SCRIPT_CADDY*)o->vm_caddy)->caller_type = SCRIPT_CALLER_MOBILE;

   /* Setup the VM. */
   o->vm = duk_create_heap(
      NULL, NULL, NULL,
      o->vm_caddy,
      duktape_helper_mobile_crash
   );

   hashmap_iterate( &(o->vm_globals), mobile_vm_global_set_cb, o );

cleanup:
   return;
#endif /* USE_DUKTAPE */
}

void mobile_vm_tick( struct MOBILE* o ) {
#ifdef USE_DUKTAPE
   int duk_result = 0;
   bstring tick_script = NULL;

   if( NULL == o->vm ) {
      goto cleanup; /* Silently. Not all mobs have scripts. */
   }

   ((struct SCRIPT_CADDY*)o->vm_caddy)->exec_start = graphics_get_ticks();

   tick_script = hashmap_get_c( &(o->vm_scripts), "tick" );
   const char* tick_script_c = bdata( tick_script );
   if( NULL != tick_script ) {
      mobile_vm_run( o, tick_script );
   }

   //if( FALSE == o->vm_started ) {
      //o->vm_started = TRUE;
      //duk_result = duk_safe_call( o->vm, duktape_eval_line_string, o->vm_caddy, 1, 1 );

      //duk_push_string( o->vm, "tick" );
      //duk_call( o->vm, 0 );

      //if( 1 != duk_result ) {
      //   goto cleanup;
      //}
   /* } else {
      duk_resume( o->vm,  )
   } */

   /* An error happened. */
   /*duk_get_prop_string( o->vm, 0, "name" );
   duk_get_prop_string( o->vm, 0, "message" );
   duk_get_prop_string( o->vm, 0, "fileName" );
   duk_get_prop_string( o->vm, 0, "lineNumber" );
   duk_get_prop_string( o->vm, 0, "stack" );

   scaffold_print_error(
      &module, "Script error: %s: %s (%s:%s)\n",
      duk_safe_to_string( o->vm, 1 ), duk_safe_to_string( o->vm, 2 ),
      duk_safe_to_string( o->vm, 3 ), duk_safe_to_string( o->vm, 4 )
   );
   scaffold_print_error(
      &module, "Script stack: %s\n",
      duk_safe_to_string( o->vm, 5 )
   ); */

cleanup:
   return;
#endif /* USE_DUKTAPE */
}

void mobile_vm_end( struct MOBILE* o ) {
#ifdef USE_DUKTAPE
   scaffold_print_debug(
      &module, "Stopping script VM for mobile: %d (%b)\n", o->serial, o->mob_id
   );

   if( NULL != o->vm ) {
      duk_destroy_heap( o->vm );
      o->vm = NULL;
   }
   if( NULL != o->vm_caddy ) {
      free( o->vm_caddy );
      o->vm_caddy = NULL;
   }
#endif /* USE_DUKTAPE */
}

BOOL mobile_has_event( struct MOBILE* o, const char* event ) {
   if(
      NULL != o &&
      HASHMAP_SENTINAL == o->vm_scripts.sentinal &&
      NULL != hashmap_get_c( &(o->vm_scripts), event )
   ) {
      return TRUE;
   } else {
      return FALSE;
   }
}
