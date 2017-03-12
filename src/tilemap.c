#include "tilemap.h"

#include "callback.h"

#include <memory.h>
#include <string.h>

static BOOL tilemap_layer_free_cb( bstring res, void* iter, void* arg ) {
   tilemap_layer_free( (struct TILEMAP_LAYER*)iter );
   return TRUE;
}

static BOOL tilemap_tileset_free_cb( bstring res, void* iter, void* arg ) {
   tilemap_tileset_free( (struct TILEMAP_TILESET*)iter );
   return TRUE;
}

static void tilemap_cleanup( const struct REF* ref ) {
   struct TILEMAP* t;

   t = scaffold_container_of( ref, struct TILEMAP, refcount );

   hashmap_remove_cb( &(t->layers), tilemap_layer_free_cb, NULL );
   hashmap_cleanup( &(t->layers) );
   hashmap_remove_cb( &(t->player_spawns), callback_free_generic, NULL );
   hashmap_cleanup( &(t->player_spawns) );
   hashmap_remove_cb( &(t->tilesets), tilemap_tileset_free_cb, NULL );
   hashmap_cleanup( &(t->tilesets) );

   /* TODO: Free tilemap. */
}

void tilemap_init( struct TILEMAP* t, BOOL local_images ) {
   ref_init( &(t->refcount), tilemap_cleanup );

   hashmap_init( &(t->layers) );
   hashmap_init( &(t->tilesets) );
   hashmap_init( &(t->player_spawns) );

   bfromcstralloc( TILEMAP_NAME_ALLOC, "" );

   t->orientation = TILEMAP_ORIENTATION_ORTHO;
   t->lname = bfromcstr( "" );
}

void tilemap_free( struct TILEMAP* t ) {
   refcount_dec( t, "tilemap" );
}

void tilemap_layer_init( struct TILEMAP_LAYER* layer ) {
   vector_init( &(layer->tiles) );
}

void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer ) {
   vector_free( &(layer->tiles ) );
}

void tilemap_tileset_free( struct TILEMAP_TILESET* tileset ) {
   hashmap_remove_cb( &(tileset->images), callback_free_graphics, NULL );
}

void tilemap_iterate_screen_row(
   struct TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( struct TILEMAP* t, uint32_t x, uint32_t y )
) {

}

SCAFFOLD_INLINE struct TILEMAP_TILESET* tilemap_get_tileset( struct TILEMAP* t, SCAFFOLD_SIZE gid ) {
   return hashmap_iterate( &(t->tilesets), callback_search_tilesets_gid, &gid );
}

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))

SCAFFOLD_INLINE void tilemap_get_tile_tileset_pos(
   struct TILEMAP_TILESET* set, GRAPHICS* g_set, SCAFFOLD_SIZE gid, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
) {
   SCAFFOLD_SIZE tiles_wide = 0;
   SCAFFOLD_SIZE tiles_high = 0;

   scaffold_check_null( g_set );

   tiles_wide = g_set->w / set->tilewidth;
   tiles_high = g_set->h / set->tileheight;

   gid -= set->firstgid - 1;

   *y = ((gid - 1) / tiles_wide) * set->tileheight;
   *x = ((gid - 1) % tiles_wide) * set->tilewidth;

   scaffold_assert( *y < (set->tileheight * tiles_high) );
   scaffold_assert( *x < (set->tilewidth * tiles_wide) );
cleanup:
   return;
}

SCAFFOLD_INLINE uint32_t tilemap_get_tile( struct TILEMAP_LAYER* layer, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y ) {
   SCAFFOLD_SIZE index = (y * layer->width) + x;
   return vector_get_scalar( &(layer->tiles), index );
}

static void* tilemap_layer_draw_cb( bstring key, void* iter, void* arg ) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct GRAPHICS_TILE_WINDOW* window = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP* t = window->t;
   struct TILEMAP_TILESET* set = NULL;
   GRAPHICS* g_tileset = NULL;
   SCAFFOLD_SIZE
      x = 0,
      y = 0,
      max_x = 0,
      max_y = 0,
      tileset_x = 0,
      tileset_y = 0,
      pix_x = 0,
      pix_y = 0;
   uint32_t tile;
   struct VECTOR* tiles = NULL;
#ifdef DEBUG_TILES
   bstring bnum = NULL;
#endif /* DEBUG_TILES */

   max_x = window->x + window->width;
   max_y = window->y + window->height;
   tiles = &(layer->tiles);

   if( NULL == tiles || 0 == vector_count( tiles ) ) {
      goto cleanup;
   }

   for( x = window->x ; max_x > x ; x++ ) {
      for( y = window->y ; max_y > y ; y++ ) {
         tile = tilemap_get_tile( layer, x, y );
         if( 0 == tile ) {
            continue;
         }

         set = tilemap_get_tileset( t, tile );
         scaffold_check_null( set );
         scaffold_check_zero( set->tilewidth );
         scaffold_check_zero( set->tileheight );

         /* Figure out the window position to draw to. */
         pix_x = set->tilewidth * (x - window->x);
         pix_y = set->tileheight * (y - window->y);

         /* Figure out the graphical tile to draw from. */
         /* TODO: Support multiple images. */
         g_tileset = (GRAPHICS*)hashmap_get_first( &(set->images) );
         if( NULL == g_tileset ) {
            /* TODO: Use a built-in placeholder tileset. */
            goto cleanup;
         }
         tilemap_get_tile_tileset_pos( set, g_tileset, tile, &tileset_x, &tileset_y );

         graphics_blit_partial(
            window->g,
            pix_x, pix_y,
            tileset_x, tileset_y,
            set->tilewidth, set->tileheight,
            g_tileset
         );

#ifdef DEBUG_TILES
         bnum = bformat( "%d,", tileset_x );
         graphics_draw_text( window->g, pix_x + 16, pix_y + 10, bnum );
         bassignformat( bnum, "%d", tileset_y );
         graphics_draw_text( window->g, pix_x + 16, pix_y + 22, bnum );
         bdestroy( bnum );
#endif /* DEBUG_TILES */
      }
   }

cleanup:
   return NULL;
}

void tilemap_draw_ortho( struct GRAPHICS_TILE_WINDOW* twindow ) {
   hashmap_iterate( &(twindow->t->layers), tilemap_layer_draw_cb, twindow );
}
