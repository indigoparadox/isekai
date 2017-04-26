
#define ITEM_C
#include "item.h"

#include "callback.h"
#include "chunker.h"

void item_init( struct ITEM* e ) {
   e->display_name = NULL;
   e->serial = 0;
   e->sprite = NULL;
}

void item_random_init(
   struct ITEM* e, ITEM_TYPE type, struct ITEM_SPRITESHEET* catalog
) {

}

void item_sprite_free( struct ITEM_SPRITE* sprite ) {
   if( NULL != sprite ) {
      bdestroy( sprite->display_name );
      scaffold_free( sprite );
   }
}

void item_spritesheet_free( struct ITEM_SPRITESHEET* catalog ) {
   if( NULL != catalog ) {
      vector_remove_cb( &(catalog->sprites), callback_free_sprites, NULL );
      vector_cleanup( &(catalog->sprites) );

      graphics_surface_free( catalog->sprites_image );

      bdestroy( catalog->sprites_filename );

      scaffold_free( catalog );
   }
}

ITEM_TYPE item_type_from_c( const char* c_string ) {
   SCAFFOLD_SIZE_SIGNED i;
   ITEM_TYPE type_out = ITEM_TYPE_GENERIC;

   for( i = 0 ; ITEM_TYPE_MAX > i ; i++ ) {
      if( 0 == scaffold_strcmp_caseless(
         item_type_strings[i].data, c_string
      ) ) {
         type_out = i;
         break;
      }
   }

   return type_out;
}

void item_draw_ortho(
   struct ITEM* e, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y, GRAPHICS* g
) {
   GFX_COORD_PIXEL
      sprite_x,
      sprite_y;
   struct MOBILE* o_player = NULL;
   //struct CLIENT* local_client = twindow->local_client;
   struct ITEM_SPRITESHEET* catalog = NULL;

   scaffold_assert_client();

   if( NULL == e || NULL == e->catalog || NULL == e->sprite || NULL == g ) {
      return;
   }

   catalog = e->catalog;

   /*
   if(
      x > twindow->max_x ||
      y > twindow->max_y ||
      x < twindow->min_x ||
      y < twindow->min_y
   ) {
      goto cleanup;
   }
   */

   /* Figure out the window position to draw to. */

   /* TODO: Support variable sprite size. */

   /* TODO: Figure out the graphical sprite to draw from. */
   /* mobile_get_spritesheet_pos_ortho(
      o, current_frame->id, &sprite_x, &sprite_y
   ); */

   /* Add dirty tiles to list before drawing. */
   /* TODO: This goes in tilemap function if that's calling us. */
   /* tilemap_add_dirty_tile( twindow->t, o->x, o->y );
   tilemap_add_dirty_tile( twindow->t, o->prev_x, o->prev_y ); */

   graphics_blit_partial(
      g,
      x, y,
      sprite_x, sprite_y,
      catalog->spritewidth, catalog->spriteheight,
      catalog->sprites_image
   );

cleanup:
   return;
}

void item_set_contents( struct ITEM* e, union ITEM_CONTENT content ) {
   e->content = content;
}
