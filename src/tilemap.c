#include "tilemap.h"

#include "ezxml/ezxml.h"
#include "b64/b64.h"
#include "callbacks.h"

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
   int i;
   struct TILEMAP* t;

   t = scaffold_container_of( ref, struct TILEMAP, refcount );

#ifdef EZXML_EMBEDDED_IMAGES
   for( i = 0 ; vector_count( &(t->freeable_chunks) ) > i ; i++ ) {
      free( vector_get( &(t->freeable_chunks), i ) );
   }
   vector_free( &(t->freeable_chunks) );
#endif /* EZXML_EMBEDDED_IMAGES */
   hashmap_remove_cb( &(t->layers), tilemap_layer_free_cb, NULL );
   hashmap_cleanup( &(t->layers) );
   for( i = 0 ; vector_count( &(t->positions) ) > i ; i++ ) {
      tilemap_position_free( vector_get( &(t->positions), i ) );
   }
   vector_free( &(t->positions) );
   hashmap_remove_cb( &(t->tilesets), tilemap_tileset_free_cb, NULL );
   hashmap_cleanup( &(t->tilesets) );
   bdestroy( t->serialize_buffer );
   bdestroy( t->serialize_filename );

   /* TODO: Free tilemap. */
}

void tilemap_init( struct TILEMAP* t, BOOL local_images ) {
   ref_init( &(t->refcount), tilemap_cleanup );

   hashmap_init( &(t->layers) );
   vector_init( &(t->positions) );
   hashmap_init( &(t->tilesets) );

   t->orientation = TILEMAP_ORIENTATION_ORTHO;

   t->serialize_buffer = NULL;
   t->serialize_filename = bfromcstralloc( 30, "" );

   t->local_images = local_images;
}

void tilemap_free( struct TILEMAP* t ) {
   ref_dec( &(t->refcount) );
}

void tilemap_layer_init( struct TILEMAP_LAYER* layer ) {
   vector_init( &(layer->tiles) );
}

void tilemap_layer_cleanup( struct TILEMAP_LAYER* layer ) {
   vector_free( &(layer->tiles ) );
}

void tilemap_position_cleanup( struct TILEMAP_POSITION* position ) {

}

void tilemap_tileset_free( struct TILEMAP_TILESET* tileset ) {
   hashmap_remove_cb( &(tileset->images), callback_free_graphics, NULL );
}

void tilemap_iterate_screen_row(
   struct TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( struct TILEMAP* t, uint32_t x, uint32_t y )
) {

}

struct TILEMAP_TILESET* tilemap_get_tileset( struct TILEMAP* t, size_t gid ) {
   return hashmap_iterate( &(t->tilesets), callback_search_tilesets_gid, &gid );
}

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))

inline void tilemap_get_tile_tileset_pos(
   struct TILEMAP_TILESET* set, GRAPHICS* g_set, size_t gid, size_t* x, size_t* y
) {
   size_t tiles_wide = 0;
   size_t tiles_high = 0;
#ifdef USE_MATH
   float fx;
   float fy;
#endif /* USE_PATH */

   scaffold_check_null( g_set );

   tiles_wide = g_set->w / set->tilewidth;
   tiles_high = g_set->h / set->tileheight;

   gid -= set->firstgid - 1;

#ifdef USE_MATH
   fx = ((gid - 1) % tiles_wide);
   fy = ((gid - 1) / tiles_wide);
   *y = floor( fy ) * set->tileheight;
   *x = floor( fx ) * set->tilewidth;
#else
   *y = ((gid - 1) / tiles_wide) * set->tileheight;
   *x = ((gid - 1) % tiles_wide) * set->tilewidth;
#endif /* USE_MATH */

   assert( *y < (set->tileheight * tiles_high) );
   assert( *x < (set->tilewidth * tiles_wide) );
cleanup:
   return;
}

inline uint32_t tilemap_get_tile( struct TILEMAP_LAYER* layer, size_t x, size_t y ) {
   size_t index = (y * layer->width) + x;
   return vector_get_scalar( &(layer->tiles), index );
}

static void* tilemap_layer_draw_cb( bstring key, void* iter, void* arg ) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct GRAPHICS_TILE_WINDOW* window = (struct GRAPHICS_TILE_WINDOW*)arg;
   struct TILEMAP* t = window->t;
   struct TILEMAP_TILESET* set = NULL;
   GRAPHICS* g_tileset = NULL;
   size_t x, y, max_x, max_y, tileset_x, tileset_y, pix_x, pix_y;
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

void tilemap_draw_ortho( struct TILEMAP* t, GRAPHICS* g, struct GRAPHICS_TILE_WINDOW* window ) {

   if( NULL == window->t ) {
      window->t = t;
   }
   if( NULL == window->g ) {
      window->g = g;
   }

   hashmap_iterate( &(t->layers), tilemap_layer_draw_cb, window );

}
