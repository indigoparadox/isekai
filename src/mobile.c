
#include "mobile.h"

static void mobile_cleanup( const struct REF* ref ) {
   struct MOBILE* o = scaffold_container_of( ref, struct MOBILE, refcount );
   if( NULL != o->owner ) {
      client_clear_puppet( o->owner );
      o->owner = NULL;
   }
   free( o );
}

void mobile_free( struct MOBILE* o ) {
   ref_dec( &(o->refcount) );
}

void mobile_init( struct MOBILE* o ) {
   ref_init( &(o->refcount), mobile_cleanup );
   o->serial = 0;
}

void mobile_animate( struct MOBILE* o ) {

}

inline void mobile_get_spritesheet_pos_ortho(
   GRAPHICS* g_tileset, MOBILE_FACING facing, MOBILE_FRAME frame,
   MOBILE_FRAME_ALT frame_alt, size_t* x, size_t* y
) {
   assert( frame < 0 || frame_alt < 0 );

   scaffold_check_null( g_tileset );

   if( 0 > frame ) {
      *y = (facing * MOBILE_SPRITE_SIZE) + (4 * MOBILE_SPRITE_SIZE);
   } else {
      *y = facing * MOBILE_SPRITE_SIZE;
   }
   *x = frame * MOBILE_SPRITE_SIZE;

cleanup:
   return;
}

void mobile_draw_ortho( struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* window ) {
   size_t max_x, max_y, sprite_x, sprite_y, pix_x, pix_y;
#ifdef DEBUG_TILES
   bstring bnum = NULL;
#endif /* DEBUG_TILES */

   max_x = window->x + window->width;
   max_y = window->y + window->height;

   if( o->x > max_x || o->y > max_y || o->x < window->x || o->y < window->y ) {
      goto cleanup;
   }

   /* Figure out the window position to draw to. */
   /* TODO: Support variable sprite size. */
   pix_x = MOBILE_SPRITE_SIZE * (o->x - window->x);
   pix_y = MOBILE_SPRITE_SIZE * (o->y - window->y);

   /* Figure out the graphical sprite to draw from. */
   /* TODO: Support varied spritesheets. */
   mobile_get_spritesheet_pos_ortho(
      o->sprites, o->facing, o->frame, o->frame_alt, &sprite_x, &sprite_y
   );

   graphics_blit_partial(
      window->g,
      pix_x, pix_y,
      sprite_x, sprite_y,
      MOBILE_SPRITE_SIZE, MOBILE_SPRITE_SIZE,
      o->sprites
   );

cleanup:
   return;
}
