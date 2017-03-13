
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"
#include "hashmap.h"
#include "callback.h"

const struct tagbstring str_mobile_spritesheet_path_default = bsStatic( "mobs/sprites_maid_black" GRAPHICS_RASTER_EXTENSION );

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
   o->sprites_filename = bstrcpy( &str_mobile_spritesheet_path_default );
   o->serial = 0;
   o->channel = NULL;
   o->display_name = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   o->frame_alt = MOBILE_FRAME_ALT_NONE;
   o->frame = MOBILE_FRAME_DEFAULT;
}

void mobile_animate( struct MOBILE* o ) {
   if( mobile_frame_counter++ > MOBILE_FRAME_DIVISOR ) {
      mobile_frame_counter = 0;
   }
   if( NULL != o && 0 == mobile_frame_counter ) {
      switch( o->frame ) {
      case MOBILE_FRAME_NONE:
         o->frame = MOBILE_FRAME_RIGHT_FORWARD;
         break;
      case MOBILE_FRAME_RIGHT_FORWARD:
         o->frame = MOBILE_FRAME_DEFAULT;
         break;
      case MOBILE_FRAME_DEFAULT:
         o->frame = MOBILE_FRAME_LEFT_FORWARD;
         break;
      case MOBILE_FRAME_LEFT_FORWARD:
         o->frame = MOBILE_FRAME_NONE;
         break;
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
   GRAPHICS* g_tileset, MOBILE_FACING facing, MOBILE_FRAME frame,
   MOBILE_FRAME_ALT frame_alt, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
) {
   scaffold_assert( frame < 0 || frame_alt < 0 );

   scaffold_check_null( g_tileset );

   /* TODO: Arbitrary animations for mobiles. */
   /*
   if( 0 > frame ) {
      *y = (facing * MOBILE_SPRITE_SIZE) + (4 * MOBILE_SPRITE_SIZE);
   } else {
   */
      *y = facing * MOBILE_SPRITE_SIZE;
   //}
   *x = (0 > frame ? 0 : frame) * MOBILE_SPRITE_SIZE;

cleanup:
   return;
}

SCAFFOLD_INLINE
SCAFFOLD_SIZE mobile_get_steps_remaining_x( const struct MOBILE* o, BOOL reverse ) {
   if( o->prev_x != o->x ) {
      return TRUE != reverse ? o->steps_remaining : -1 * o->steps_remaining;
   } else {
      return 0;
   }
}

SCAFFOLD_INLINE
SCAFFOLD_SIZE mobile_get_steps_remaining_y( const struct MOBILE* o, BOOL reverse ) {
   if( o->prev_y != o->y ) {
      return TRUE != reverse ? o->steps_remaining : -1 * o->steps_remaining;
   } else {
      return 0;
   }
}

void mobile_draw_ortho( struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow ) {
   SCAFFOLD_SIZE
      sprite_x,
      sprite_y,
      pix_x,
      pix_y;

#ifdef DEBUG_TILES
   bstring bnum = NULL;
#endif /* DEBUG_TILES */

   scaffold_assert_client();

   if( NULL == o ) {
      return;
   }

   if(
      o->x > twindow->max_x ||
      o->y > twindow->max_y ||
      o->x < twindow->min_x ||
      o->y < twindow->min_y
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
   pix_x = (MOBILE_SPRITE_SIZE * (o->x - (twindow->x)));
   pix_y = (MOBILE_SPRITE_SIZE * (o->y - (twindow->y)));

   if( !tilemap_inside_inner_map_x( o->x, o->y, twindow ) ) {
      pix_x += mobile_get_steps_remaining_x( o, FALSE );
   }

   if( !tilemap_inside_inner_map_y( o->x, o->y, twindow ) ) {
      pix_y += mobile_get_steps_remaining_y( o, FALSE );
   }

   bstring pos = bformat(
      "%d (%d), %d (%d)",
      o->x, o->prev_x, o->y, o->prev_y
   );
   graphics_set_color( twindow->g, GRAPHICS_COLOR_MAGENTA );
   graphics_draw_text( twindow->g, 100, 30, GRAPHICS_TEXT_ALIGN_LEFT, pos );

   /* Figure out the graphical sprite to draw from. */
   /* TODO: Support varied spritesheets. */
   mobile_get_spritesheet_pos_ortho(
      o->sprites, o->facing, o->frame, o->frame_alt, &sprite_x, &sprite_y
   );

   graphics_blit_partial(
      twindow->g,
      pix_x, pix_y,
      sprite_x, sprite_y,
      MOBILE_SPRITE_SIZE, MOBILE_SPRITE_SIZE,
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
MOBILE_UPDATE mobile_calculate_terrain(
   struct TILEMAP* t, MOBILE_UPDATE update_in,
   SCAFFOLD_SIZE x_1, SCAFFOLD_SIZE y_1, SCAFFOLD_SIZE x_2, SCAFFOLD_SIZE y_2
) {
   struct VECTOR* tiles_start = NULL;
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_start;
   struct TILEMAP_POSITION pos_end;
   MOBILE_UPDATE update_out = update_in;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   int8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;

   pos_start.x = x_1;
   pos_start.y = y_1;
   pos_end.x = x_2;
   pos_end.y = y_2;

   /* Fetch the source tile on all layers. */
   /*tiles_start =
      hashmap_iterate_v( &(t->layers), callback_get_tile_stack_l, &pos_start );*/

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
      }

      update->update =
         mobile_calculate_terrain( &(update->l->tilemap), update->update,
            o->x, o->y, update->x, update->y );
   }

   switch( update->update ) {
   case MOBILE_UPDATE_MOVEUP:
      o->prev_x = o->x; /* Forceful reset. */
      o->y--;
      o->facing = MOBILE_FACING_UP;
      o->steps_inc = MOBILE_STEPS_INCREMENT * -1;
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
      o->steps_inc = MOBILE_STEPS_INCREMENT;
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
      o->steps_inc = MOBILE_STEPS_INCREMENT * -1;
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
      o->steps_inc = MOBILE_STEPS_INCREMENT;
      if( TRUE == instant ) {
         o->prev_x = o->x;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX * -1;
      }
      break;

   case MOBILE_UPDATE_NONE:
      break;
   }

   return update->update;
}
