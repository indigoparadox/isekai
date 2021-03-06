
#define ITEM_C
#include "item.h"

#include "callback.h"
#include "chunker.h"
#include "rng.h"

void* callback_search_item_type(
   size_t idx, void* iter, void* arg
) {
   ITEM_TYPE type = *((ITEM_TYPE*)arg);
   struct ITEM_SPRITE* sprite = (struct ITEM_SPRITE*)iter;

   if( NULL != sprite && type == sprite->type ) {
      return sprite;
   }

   return NULL;
}

static void item_free_final( const struct REF* ref ) {
   struct ITEM* e = scaffold_container_of( ref, struct ITEM, refcount );

   bdestroy( e->display_name );
   bdestroy( e->catalog_name );

}

void item_init(
   struct ITEM* e, const bstring display_name,
   SCAFFOLD_SIZE count, const bstring catalog_name,
   SCAFFOLD_SIZE sprite_id, struct CLIENT* c
) {
   int retval;

   ref_init( &(e->refcount), item_free_final );

   client_set_item( c, e->serial, e );

   e->client_or_server = c;
   e->sprite_id = sprite_id;
   e->catalog_name = bstrcpy( catalog_name );
   e->content.container = NULL;
   e->count = count;

   scaffold_assign_or_cpy_c( e->display_name, bdata( display_name ), retval );

cleanup:
   return;
}

void item_random_init(
   struct ITEM* e, ITEM_TYPE type, const bstring catalog_name,
   struct CLIENT* c
) {
   SCAFFOLD_SIZE sprite_id = 0;
   struct ITEM_SPRITESHEET* catalog = NULL;
   struct ITEM_SPRITE* sprite = NULL;

   catalog = client_get_catalog( c, catalog_name );
   lgc_null_msg( catalog, "Unable to find catalog." );

   sprite_id = item_random_sprite_id_of_type( type, catalog );
   /* TODO: Sprite 0 should be reserved. */
   sprite = item_spritesheet_get_sprite( catalog, sprite_id );
   lgc_null_msg( sprite, "Unable to find item sprite." );

   item_init( e, sprite->display_name, 1, catalog_name, sprite_id, c );
   rng_gen_serial(
      e, client_get_unique_items( catalog->client_or_server ), SERIAL_MIN, BIG_SERIAL_MAX
   );

cleanup:
   return;
}

void item_free( struct ITEM* e ) {
   refcount_dec( e, "item" );
}

void item_sprite_free( struct ITEM_SPRITE* sprite ) {
   if( NULL != sprite ) {
      bdestroy( sprite->display_name );
      mem_free( sprite );
   }
}

static void item_spritesheet_free_final( const struct REF* ref ) {
   struct ITEM_SPRITESHEET* catalog =
      scaffold_container_of( ref, struct ITEM_SPRITESHEET, refcount );
   if( NULL != catalog ) {
      vector_remove_cb( catalog->sprites, callback_free_sprites, NULL );
      vector_free( &(catalog->sprites) );

      graphics_surface_free( catalog->sprites_image );
      catalog->sprites_image = NULL;

      bdestroy( catalog->sprites_filename );
      catalog->sprites_filename = NULL;

      mem_free( catalog );
   }
}

void item_spritesheet_init(
   struct ITEM_SPRITESHEET* catalog,
   const bstring name,
   struct CLIENT* client_or_server
) {
   ref_init( &(catalog->refcount), item_spritesheet_free_final );
   catalog->client_or_server = client_or_server;
   catalog->sprites_image = NULL;
}

void item_spritesheet_free( struct ITEM_SPRITESHEET* catalog ) {
   refcount_dec( catalog, "catalog" );
}

struct ITEM_SPRITE* item_spritesheet_get_sprite(
   struct ITEM_SPRITESHEET* catalog,
   SCAFFOLD_SIZE sprite_id
) {
   struct ITEM_SPRITE* sprite_out = NULL;

   sprite_out = vector_get(  catalog->sprites, sprite_id );

   return sprite_out;
}

ITEM_TYPE item_type_from_c( const char* c_string ) {
   SCAFFOLD_SIZE_SIGNED i;
   ITEM_TYPE type_out = ITEM_TYPE_GENERIC;

   for( i = 0 ; ITEM_TYPE_MAX > i ; i++ ) {
      if( 0 == scaffold_strcmp_caseless(
         (const char*)(item_type_strings[i].data), c_string
      ) ) {
         type_out = i;
         break;
      }
   }

   return type_out;
}

SCAFFOLD_SIZE item_random_sprite_id_of_type(
   ITEM_TYPE type, struct ITEM_SPRITESHEET* catalog
) {
   struct VECTOR* candidates = NULL;
   SCAFFOLD_SIZE selection = 0;

   candidates = vector_iterate_v(
      catalog->sprites, callback_search_item_type, &type
   );
   lgc_null_msg( candidates, "No sprite candidates found." );

   selection = rng_max( vector_count( candidates ) );
   /* sprite_out = vector_get( candidates, selection ); */

cleanup:
   if( NULL != candidates ) {
      /* Sprites are not garbage collected. */
      vector_cleanup_force( candidates );
      mem_free( candidates );
   }
   return selection;
}

void item_draw_ortho(
   struct ITEM* e, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y, GRAPHICS* g
) {
   struct ITEM_SPRITESHEET* catalog = NULL;
   GRAPHICS_RECT sprite_rect = { 0, 0, 0, 0 };
   struct ITEM_SPRITE* sprite = NULL;

   scaffold_assert_client();

   if( NULL == e || NULL == g ) {
      goto cleanup;
   }

   catalog = client_get_catalog( e->client_or_server, e->catalog_name );
   if( NULL == catalog ) {
      goto cleanup;
   }

   sprite = item_spritesheet_get_sprite( catalog, e->sprite_id );
   if( NULL == sprite ) {
      goto cleanup;
   }

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
   sprite_rect.w = catalog->spritewidth;
   sprite_rect.h = catalog->spriteheight;

   /* TODO: Figure out the graphical sprite to draw from. */
   /*graphics_get_spritesheet_pos_ortho(
      catalog->sprites_image, &sprite_rect, e->sprite_id
   );*/

   /* Add dirty tiles to list before drawing. */
   /* TODO: This goes in tilemap function if that's calling us. */
   /* tilemap_add_dirty_tile( twindow->t, o->x, o->y );
   tilemap_add_dirty_tile( twindow->t, o->prev_x, o->prev_y ); */
   if(
      NULL == catalog->sprites_image &&
      NULL != client_get_sprite( e->client_or_server, catalog->sprites_filename )
   ) {
      catalog->sprites_image =
         client_get_sprite( e->client_or_server, catalog->sprites_filename );
   }

   graphics_blit_partial(
      g,
      x, y,
      sprite_rect.x, sprite_rect.y,
      catalog->spritewidth, catalog->spriteheight,
      catalog->sprites_image
   );

cleanup:
   return;
}

void item_set_contents( struct ITEM* e, union ITEM_CONTENT content ) {
   e->content = content;
}

bool item_is_container( struct ITEM* e ) {
   struct ITEM_SPRITE* sprite;
   struct ITEM_SPRITESHEET* catalog;

   if( NULL == e ) {
      return false;
   }

   catalog = client_get_catalog( e->client_or_server, e->catalog_name );
   if( NULL == catalog ) { return false; }
   sprite = item_spritesheet_get_sprite( catalog, e->sprite_id );
   if( NULL == sprite ) { return false; }

   return ITEM_TYPE_CONTAINER == sprite->type &&
      NULL != e->content.container;
}

void item_cache_init(
   struct ITEM_CACHE* cache,
   struct TILEMAP* t,
   TILEMAP_COORD_TILE x,
   TILEMAP_COORD_TILE y
) {
   cache->items = vector_new();
   cache->position.x = x;
   cache->position.y = y;
   cache->tilemap = t;
}

void item_cache_free( struct ITEM_CACHE* cache ) {
   vector_remove_cb( cache->items, callback_free_item_cache_items, NULL );
   vector_free( &(cache->items) );
   mem_free( cache );
}

void item_stack_push( struct ITEM* item_stack, struct ITEM* new_item ) {
   /* TODO */
}

BIG_SERIAL item_get_serial( struct ITEM* e ) {
   return e->serial;
}
