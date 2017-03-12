
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"
#include "hashmap.h"

#define MOBILE_SPRITE_SIZE 32
#define MOBILE_FRAME_MAX 500

const struct tagbstring str_mobile_spritesheet_path_default = bsStatic( "mobs/sprites_maid_black" GRAPHICS_RASTER_EXTENSION );

/* FIXME: Replace with a proper frame limiter. */
static uint8_t mobile_frame_counter = 0;

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
   if( NULL == o || 0 != mobile_frame_counter ) {
      return;
   }
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

void mobile_frame_count() {
   if( mobile_frame_counter++ > MOBILE_FRAME_MAX ) {
      mobile_frame_counter = 0;
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

void mobile_draw_ortho( struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow ) {
   SCAFFOLD_SIZE max_x, max_y, sprite_x, sprite_y, pix_x, pix_y;

#ifdef DEBUG_TILES
   bstring bnum = NULL;
#endif /* DEBUG_TILES */

   scaffold_assert_client();

   if( NULL == o ) {
      return;
   }

   max_x = twindow->x + twindow->width;
   max_y = twindow->y + twindow->height;

   if( o->x > max_x || o->y > max_y || o->x < twindow->x || o->y < twindow->y ) {
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
   pix_x = MOBILE_SPRITE_SIZE * (o->x - twindow->x);
   pix_y = MOBILE_SPRITE_SIZE * (o->y - twindow->y);

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

/** \brief
 *
 * \param update - Packet containing update information.
 * \param
 * \return What the update becomes to send to the clients. MOBILE_UPDATE_NONE
 *         if the update failed to occur.
 *
 */
 MOBILE_UPDATE mobile_apply_update( struct MOBILE_UPDATE_PACKET* update, BOOL instant ) {
   struct MOBILE* o = update->o;

   /* TODO: Collision detection. */

   switch( update->update ) {
   case MOBILE_UPDATE_MOVEUP:
      o->y--;
      if( TRUE == instant ) {
         o->prev_y = o->y;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_MOVEDOWN:
      o->y++;
      if( TRUE == instant ) {
         o->prev_y = o->y;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_MOVELEFT:
      o->x--;
      if( TRUE == instant ) {
         o->prev_x = o->x;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_MOVERIGHT:
      o->x++;
      if( TRUE == instant ) {
         o->prev_x = o->x;
      } else {
         o->steps_remaining = MOBILE_STEPS_MAX;
      }
      break;

   case MOBILE_UPDATE_NONE:
      break;
   }

   return update->update;
}
