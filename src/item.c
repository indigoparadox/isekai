
#define ITEM_C
#include "item.h"

#include "callback.h"

void item_init( struct ITEM* e ) {
   e->display_name = NULL;
   e->serial = 0;
   e->type = ITEM_TYPE_GENERIC;
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

      bdestroy( catalog->filename );

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

void item_set_contents( struct ITEM* e, union ITEM_CONTENT content ) {
   e->content = content;
}
