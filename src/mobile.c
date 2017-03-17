
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"
#include "hashmap.h"
#include "callback.h"
#include "tinypy/tinypy.h"

/* FIXME: Replace with a proper frame limiter. */
static uint8_t mobile_frame_counter = 0;
static uint8_t mobile_move_counter = 0;

static void mobile_cleanup( const struct REF* ref ) {
   struct MOBILE* o = scaffold_container_of( ref, struct MOBILE, refcount );

   if( NULL != o->sprites ) {
      graphics_surface_free( o->sprites );
   }
   if( NULL != o->owner ) {
      client_clear_puppet( o->owner );
      o->owner = NULL;
   }
   if( NULL != o->channel ) {
      channel_free( o->channel );
      o->channel = NULL;
   }
   bdestroy( o->display_name );
   free( o );
}

void mobile_free( struct MOBILE* o ) {
   refcount_dec( o, "mobile" );
}

void mobile_init( struct MOBILE* o ) {
   ref_init( &(o->refcount), mobile_cleanup );
   o->sprites_filename = NULL;
      /* bstrcpy( &str_mobile_spritesheet_path_default ); */
   o->serial = 0;
   o->channel = NULL;
   o->display_name = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   o->current_frame = 0;
   o->steps_inc_default = MOBILE_STEPS_INCREMENT;

   vector_init( &(o->sprite_defs) );
   hashmap_init( &(o->ani_defs) );
   hashmap_init( &(o->script_defs) );
}

void mobile_animate( struct MOBILE* o ) {
   if( mobile_frame_counter++ > MOBILE_FRAME_DIVISOR ) {
      mobile_frame_counter = 0;
   }

   if( NULL != o && 0 == mobile_frame_counter && TRUE == o->initialized ) {
      scaffold_assert( NULL != o->current_animation );
      if( o->current_frame >= vector_count( &(o->current_animation->frames) ) - 1 ) {
         o->current_frame = 0;
      } else {
         o->current_frame++;
      }
   }

   if( mobile_move_counter++ > MOBILE_MOVE_DIVISOR ) {
      mobile_move_counter = 0;
   }
   if( NULL != o && 0 == mobile_move_counter ) {
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
}

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

   if(
      TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_x( o->x + 1, twindow ) ||
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_x( o->x - 1, twindow )
   ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, FALSE );
      pix_x += steps_remaining_x;
   }

   if(
      TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_y( o->y + 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_window_deadzone_y( o->y - 1, twindow )
   ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o, FALSE );
      pix_y += steps_remaining_y;
   }

#ifdef DEBUG_TILES
   if( TILEMAP_DEBUG_TERRAIN_OFF != tilemap_dt_state ) {
      bstring pos = bformat(
         "%d (%d)[%d], %d (%d)[%d]",
         o->x, o->prev_x, steps_remaining_x, o->y, o->prev_y, steps_remaining_y
      );
      graphics_set_color( twindow->g, GRAPHICS_COLOR_MAGENTA );
      graphics_draw_text( twindow->g, 10, 30, GRAPHICS_TEXT_ALIGN_LEFT, pos );
   }
#endif /* DEBUG_TILES */

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
cleanup:
   return;
}

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
      vector_free( tiles_end );
      free( tiles_end );
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
      vector_free( tiles_end );
      free( tiles_end );
   }
   return steps_inc_out;
}

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
      vector_free( tiles_end );
      free( tiles_end );
   }
   return sprite_height_out;
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

   case MOBILE_UPDATE_NONE:
      goto cleanup;
   }

   if( FALSE == instant ) {
      o->sprite_display_height =
         mobile_calculate_terrain_sprite_height(
            &(l->tilemap), o->sprite_height,
            o->x, o->y );
   }

cleanup:
   bdestroy( animation_key );
   return update->update;
}
