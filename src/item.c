
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
   struct ITEM_SPRITE* sprite = NULL;



   sprite = item_random_sprite_of_type( type, catalog );
   scaffold_check_null_msg( sprite, "Unable to create random item." );

   e->display_name = sprite->display_name;
   e->serial = rand();
   e->catalog = catalog;
   e->sprite = sprite;

cleanup:
   return;
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

struct ITEM_SPRITE* item_random_sprite_of_type(
   ITEM_TYPE type, struct ITEM_SPRITESHEET* catalog
) {
   struct ITEM_SPRITE* sprite_out = NULL;
   struct VECTOR* candidates = NULL;
   SCAFFOLD_SIZE selection = 0;

   candidates = vector_iterate_v(
      &(catalog->sprites), callback_search_item_type, &type
   );
   scaffold_check_null_msg( candidates, "No sprite candidates found." );

   selection = rand() % vector_count( candidates );
   sprite_out = vector_get( candidates, selection );

cleanup:
   if( NULL != candidates ) {
      /* Sprites are not garbage collected. */
      candidates->count = 0;
      vector_cleanup( candidates );
      scaffold_free( candidates );
   }
   return sprite_out;
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
