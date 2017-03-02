#include "tilemap.h"

#include "ezxml/ezxml.h"
#include "b64/b64.h"

#include <memory.h>
#include <string.h>

static void tilemap_cleanup( const struct _REF* ref ) {
   int i;
   TILEMAP* t;

   t = scaffold_container_of( ref, TILEMAP, refcount );

   for( i = 0 ; vector_count( &(t->freeable_chunks) ) > i ; i++ ) {
      free( vector_get( &(t->freeable_chunks), i ) );
   }
   vector_free( &(t->freeable_chunks) );
   for( i = 0 ; vector_count( &(t->layers) ) > i ; i++ ) {
      tilemap_layer_free( vector_get( &(t->layers), i ) );
   }
   vector_free( &(t->layers) );
   for( i = 0 ; vector_count( &(t->positions) ) > i ; i++ ) {
      tilemap_position_free( vector_get( &(t->positions), i ) );
   }
   vector_free( &(t->positions) );
   for( i = 0 ; vector_count( &(t->tilesets) ) > i ; i++ ) {
      tilemap_tileset_free( vector_get( &(t->tilesets), i ) );
   }
   vector_free( &(t->tilesets) );
   bdestroy( t->serialize_buffer );
   bdestroy( t->serialize_filename );

   /* TODO: Free tilemap. */
}

void tilemap_init( TILEMAP* t ) {
   ref_init( &(t->refcount), tilemap_cleanup );

   vector_init( &(t->layers) );
   vector_init( &(t->positions) );
   vector_init( &(t->tilesets) );
   vector_init( &(t->freeable_chunks ) );

   t->orientation = TILEMAP_ORIENTATION_ORTHO;

   t->serialize_buffer = NULL;
   t->serialize_filename = bfromcstralloc( 30, "" );
}

void tilemap_free( TILEMAP* t ) {
   ref_dec( &(t->refcount) );
}

void tilemap_layer_init( TILEMAP_LAYER* layer ) {
   layer->tiles = NULL;
}

void tilemap_layer_cleanup( TILEMAP_LAYER* layer ) {
   if( NULL != layer->tiles ) {
      free( layer->tiles );
   }
}

void tilemap_position_cleanup( TILEMAP_POSITION* position ) {

}

static BOOL tilemap_tileset_free_i_cb( bstring res, void* iter, void* arg ) {
   graphics_surface_free( (GRAPHICS*)iter );
   return TRUE;
}

void tilemap_tileset_free( TILEMAP_TILESET* tileset ) {
   vector_remove_cb( &(tileset->images), tilemap_tileset_free_i_cb, NULL );
}

void tilemap_iterate_screen_row(
   TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( TILEMAP* t, uint32_t x, uint32_t y )
) {

}
