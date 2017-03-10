
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"

#define MOBILE_SPRITE_SIZE 32

const struct tagbstring str_mobile_spritesheet_path_default = bsStatic( "mobs/sprites_maid_black" GRAPHICS_RASTER_EXTENSION );

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
   ref_dec( &(o->refcount) );
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

}

SCAFFOLD_INLINE void mobile_get_spritesheet_pos_ortho(
   GRAPHICS* g_tileset, MOBILE_FACING facing, MOBILE_FRAME frame,
   MOBILE_FRAME_ALT frame_alt, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
) {
   scaffold_assert( frame < 0 || frame_alt < 0 );

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

void mobile_draw_ortho( struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow ) {
   SCAFFOLD_SIZE max_x, max_y, sprite_x, sprite_y, pix_x, pix_y;
   struct CLIENT* c = NULL;
   struct CHANNEL* l = NULL;
   struct GAMEDATA* d = NULL;

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

#if 0
   /* If the current mobile spritesheet doesn't exist, then load it. */
   if( NULL == o->sprites && NULL == hashmap_get( &(d->mob_sprites), o->sprites_filename ) ) {
      /* No sprites and no request yet, so make one! */

      /* Make some assumptions to tie us to the downloading client. */
      l = scaffold_container_of( d, struct CHANNEL, gamedata );
      scaffold_check_null( l );
      c = hashmap_get_first( &(l->clients) );
      scaffold_check_null( c );
      client_request_file( c, l, CHUNKER_DATA_TYPE_MOBSPRITES, o->sprites_filename );
      goto cleanup;
   } else if( NULL == o->sprites && NULL != hashmap_get( &(d->mob_sprites), o->sprites_filename ) ) {
      o->sprites = (GRAPHICS*)hashmap_get( &(d->mob_sprites), o->sprites_filename );
      ref_inc( &(o->sprites->refcount) );
   } else if( NULL == o->sprites ) {
      /* Sprites must not be ready yet. */
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
#endif
cleanup:
   return;
}

void mobile_set_channel( struct MOBILE* o, struct CHANNEL* l ) {
   if( NULL != o->channel ) {
      channel_free( o->channel );
   }
   o->channel = l;
   if( NULL != o->channel ) {
      ref_inc( &(l->refcount) );
   }
}
