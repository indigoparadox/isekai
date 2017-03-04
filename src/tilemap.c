#include "tilemap.h"

#include "ezxml/ezxml.h"
#include "b64/b64.h"
#include "callbacks.h"

#include <memory.h>
#include <string.h>

static BOOL tilemap_tileset_free_i_cb( bstring res, void* iter, void* arg ) {
   graphics_surface_free( (GRAPHICS*)iter );
   return TRUE;
}

static BOOL tilemap_layer_free_cb( bstring res, void* iter, void* arg ) {
   tilemap_layer_free( (TILEMAP_LAYER*)iter );
   return TRUE;
}

static BOOL tilemap_tileset_free_cb( bstring res, void* iter, void* arg ) {
   tilemap_tileset_free( (TILEMAP_TILESET*)iter );
   return TRUE;
}


static void tilemap_cleanup( const struct _REF* ref ) {
   int i;
   TILEMAP* t;

   t = scaffold_container_of( ref, TILEMAP, refcount );

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

void tilemap_init( TILEMAP* t ) {
   ref_init( &(t->refcount), tilemap_cleanup );

   hashmap_init( &(t->layers) );
   vector_init( &(t->positions) );
   hashmap_init( &(t->tilesets) );
   //vector_init( &(t->freeable_chunks ) );

   t->orientation = TILEMAP_ORIENTATION_ORTHO;

   t->serialize_buffer = NULL;
   t->serialize_filename = bfromcstralloc( 30, "" );
}

void tilemap_free( TILEMAP* t ) {
   ref_dec( &(t->refcount) );
}

void tilemap_layer_init( TILEMAP_LAYER* layer ) {
   vector_init( &(layer->tiles) );
}

void tilemap_layer_cleanup( TILEMAP_LAYER* layer ) {
   vector_free( &(layer->tiles ) );
}

void tilemap_position_cleanup( TILEMAP_POSITION* position ) {

}

void tilemap_tileset_free( TILEMAP_TILESET* tileset ) {
   hashmap_remove_cb( &(tileset->images), tilemap_tileset_free_i_cb, NULL );
}

void tilemap_iterate_screen_row(
   TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( TILEMAP* t, uint32_t x, uint32_t y )
) {

}

TILEMAP_TILESET* tilemap_get_tileset( TILEMAP* t, size_t gid ) {
   return hashmap_iterate( &(t->tilesets), callback_search_tilesets, &gid );
}

#define CEILING_POS(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))

inline void tilemap_get_tile_tileset_pos( TILEMAP_TILESET* set, size_t gid, size_t* x, size_t* y ) {
   GRAPHICS* g = NULL;
   size_t tiles_wide = 0;
   size_t tiles_high = 0;
   float fx;
   float fy;

   /* TODO: Support multiple images. */
   g = (GRAPHICS*)hashmap_get_first( &(set->images) );
   tiles_wide = g->w / set->tilewidth;
   tiles_high = g->h / set->tileheight;

   gid -= set->firstgid - 1;
   fx = ((gid - 1) % tiles_wide);
   fy = ((gid - 1) / tiles_wide);

   *y = floor( fy ) * set->tileheight;
   *x = floor( fx ) * set->tilewidth;

   assert( *y < (set->tileheight * tiles_high) );
   assert( *x < (set->tilewidth * tiles_wide) );

   //*x = *x < 0 ? 0 : *x;
}

inline uint32_t tilemap_get_tile( TILEMAP_LAYER* layer, size_t x, size_t y ) {
   size_t index = (y * layer->width) + x;
   return vector_get_scalar( &(layer->tiles), index );
}

void* tilemap_layer_draw_cb( bstring key, void* iter, void* arg ) {
   TILEMAP_LAYER* layer = (TILEMAP_LAYER*)iter;
   TILEMAP_WINDOW* window = (TILEMAP_WINDOW*)arg;
   TILEMAP* t = window->t;
   TILEMAP_TILESET* set = NULL;
   GRAPHICS* g_tileset = NULL;
   size_t x, y, max_x, max_y, tileset_x, tileset_y, pix_x, pix_y;
   uint32_t tile;
   VECTOR* tiles = NULL;

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
         tilemap_get_tile_tileset_pos( set, tile, &tileset_x, &tileset_y );

         graphics_blit_partial(
            window->g,
            pix_x, pix_y,
            tileset_x, tileset_y,
            set->tilewidth, set->tileheight,
            g_tileset
         );

         bstring bnum = bformat( "%d,", tileset_x );
         graphics_draw_text( window->g, pix_x + 16, pix_y + 10, bnum );
         bassignformat( bnum, "%d", tileset_y );
         graphics_draw_text( window->g, pix_x + 16, pix_y + 22, bnum );
         bdestroy( bnum );
      }
   }

cleanup:
   return NULL;
}

void tilemap_draw_ortho( TILEMAP* t, GRAPHICS* g, TILEMAP_WINDOW* window ) {

   if( NULL == window->t ) {
      window->t = t;
   }
   if( NULL == window->g ) {
      window->g = g;
   }

   hashmap_iterate( &(t->layers), tilemap_layer_draw_cb, window );

}
