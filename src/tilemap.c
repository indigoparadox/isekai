#include "tilemap.h"

#include "ezxml/ezxml.h"

#include <memory.h>
#include <string.h>

void tilemap_init( TILEMAP* t ) {
#ifdef INIT_ZEROES
   memset( t, '\0', sizeof( TILEMAP ) );
#endif /* INIT_ZEROES */

   vector_init( &(t->layers) );
   vector_init( &(t->positions) );
}

void tilemap_cleanup( TILEMAP* t ) {
   int i;
   for( i = 0 ; vector_count( &(t->layers) ) > i ; i++ ) {
      tilemap_layer_free( vector_get( &(t->layers), i ) );
   }
   vector_free( &(t->layers) );
   for( i = 0 ; vector_count( &(t->positions) ) > i ; i++ ) {
      tilemap_position_free( vector_get( &(t->positions), i ) );
   }
   vector_free( &(t->positions) );
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

static void tilemap_parse_properties( TILEMAP* t, ezxml_t xml_props ) {
   ezxml_t xml_prop_iter = NULL;

   scaffold_check_null( xml_props );

   xml_prop_iter = ezxml_child( xml_props, "property" );

   while( NULL != xml_prop_iter ) {
      //if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "light_str" ) ) {
      //    map_out->light_str = atoi( ezxml_attr( xml_prop_iter, "value" ) );

      //if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "music_path" ) ) {
      //    map_out->music_path = bfromcstr( ezxml_attr( xml_prop_iter, "value" ) );

      //if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "time_moves" ) ) {
      //    if( 0 == strcmp( ezxml_attr( xml_prop_iter, "value" ), "true" ) ) {
      //       map_out->time_moves = TRUE;
      //    } else {
      //       map_out->time_moves = FALSE;
      //    }

      xml_prop_iter = xml_prop_iter->next;
   }

cleanup:
   return;
}

void tilemap_parse_tileset( TILEMAP* t, ezxml_t xml_tileset ) {
   TILEMAP_TILESET* set = NULL;
   ezxml_t
   xml_image,
   xml_tile,
   xml_terraintypes,
   xml_terrain;
   const char* xml_attr;
   bstring buffer = NULL;
   TILEMAP_TILESET_IMAGE* image_info = NULL;
   TILEMAP_TILE_DATA* tile_info = NULL;
   TILEMAP_TERRAIN_DATA* terrain_info = NULL;
   struct bstrList* terrain_list = NULL;
   int i;

   scaffold_check_null( xml_tileset );

   buffer = bfromcstralloc( 30, "" );
   scaffold_check_null( buffer );

   /* Try to grab the image list early. If it's missing, just get out. */
   xml_image = ezxml_child( xml_tileset, "image" );
   scaffold_check_null( xml_image );

   set = calloc( 1, sizeof( TILEMAP_TILESET ) );
   scaffold_check_null( set );
   /*vector_init( &(set->images) );
   vector_init( &(set->tiles) );
   vector_init( &(set->terrain) );*/

   while( NULL != xml_image ) {
      image_info = calloc( 1, sizeof( TILEMAP_TILESET_IMAGE ) );
      scaffold_check_null( image_info );

      /* See if this image has an external image file. */
      xml_attr = ezxml_attr( xml_image, "source" );
      if( NULL != xml_attr ) {
         bassigncstr( buffer, xml_attr );
         scaffold_check_null( buffer );

         graphics_surface_new( image_info->image, 0, 0, 0, 0 );
         graphics_set_image_path( image_info->image, buffer );
      }

      /* TODO: Decode serialized image data if present. */

      vector_add( &(set->images), image_info );
      image_info = NULL;

      xml_image = ezxml_next( xml_image );
   }

   xml_terraintypes = ezxml_child( xml_tileset, "terraintypes" );
   scaffold_check_null( xml_terraintypes );
   xml_terrain = ezxml_child( xml_terraintypes, "terrain" );
   while( NULL != xml_terrain ) {
      terrain_info = calloc( 1, sizeof( TILEMAP_TILE_DATA ) );
      scaffold_check_null( terrain_info );

      xml_attr = ezxml_attr( xml_terrain, "name" );
      scaffold_check_null( xml_attr );
      bassigncstr( buffer, xml_attr );

      terrain_info->name = bstrcpy( buffer );

      scaffold_print_debug( "Loaded terrain: %s\n", bdata( buffer ) );

      xml_attr = ezxml_attr( xml_terrain, "tile" );
      scaffold_check_null( xml_attr );
      terrain_info->tile = atoi( xml_attr );

      vector_add( &(set->terrain), terrain_info );
      terrain_info = NULL;

      xml_terrain = ezxml_next( xml_terrain );
   }

   xml_tile = ezxml_child( xml_tileset, "tile" );
   scaffold_check_null( xml_tile );
   while( NULL != xml_tile ) {
      tile_info = calloc( 1, sizeof( TILEMAP_TILE_DATA ) );
      scaffold_check_null( tile_info );

      xml_attr = ezxml_attr( xml_tile, "id" );
      scaffold_check_null( xml_attr );
      tile_info->id = atoi( xml_attr );

      xml_attr = ezxml_attr( xml_tile, "terrain" );
      scaffold_check_null( xml_attr );

      /* Parse the terrain attribute. */
      bassigncstr( buffer, xml_attr );
      terrain_list = bsplit( buffer, ',' );
      scaffold_check_null( terrain_list );
      for( i = 0 ; 4 > i ; i++ ) {
         xml_attr = bdata( terrain_list->entry[i] );
         scaffold_check_null( xml_attr );
         tile_info->terrain[i] = atoi( xml_attr );
      }
      bstrListDestroy( terrain_list );
      terrain_list = NULL;

      vector_add( &(set->tiles), tile_info );
      tile_info = NULL;

      xml_tile = ezxml_next( xml_tile );
   }

cleanup:
   bdestroy( buffer );
   if( NULL != image_info ) {
      graphics_surface_cleanup( image_info->image );
      free( image_info );
   }
   if( NULL != tile_info ) {
      /* TODO: Delete tile info. */
   }
   if( NULL != terrain_info ) {
      bdestroy( terrain_info->name );
      free( terrain_info );
   }
   bstrListDestroy( terrain_list );
   return;
}

void tilemap_parse_layer( TILEMAP* t, ezxml_t xml_layer ) {
   TILEMAP_LAYER* layer = NULL;
   ezxml_t xml_layer_data = NULL;
   bstring buffer = NULL;
   struct bstrList* tiles_list = NULL;
   int i;
   const char* xml_attr = NULL;

   scaffold_check_null( xml_layer );

   tilemap_layer_new( layer );

   layer->width = atoi( ezxml_attr( xml_layer, "width" ) );
   layer->height = atoi( ezxml_attr( xml_layer, "height" ) );

   xml_layer_data = ezxml_child( xml_layer, "data" );
   scaffold_check_null( xml_layer_data );

   buffer = bfromcstr( ezxml_txt( xml_layer_data ) );
   scaffold_check_null( buffer );
   tiles_list = bsplit( buffer, ',' );
   scaffold_check_null( tiles_list );
   layer->tiles_alloc = tiles_list->qty;
   layer->tiles = calloc( layer->tiles_alloc, sizeof( uint16_t ) );
   scaffold_check_null( layer->tiles );

   for( i = 0 ; tiles_list->qty > i ; i++ ) {
      xml_attr = bdata( tiles_list->entry[i] );
      scaffold_check_null( xml_attr );
      layer->tiles[i] = atoi( xml_attr );
      layer->tiles_count++;
   }

   vector_add( &(t->layers), layer );

cleanup:
   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      tilemap_layer_free( layer );
   }
   bdestroy( buffer );
   bstrListDestroy( tiles_list );
   return;
}

void tilemap_load_data( TILEMAP* t, const uint8_t* tmdata, int datasize ) {
   ezxml_t xml_map = NULL, xml_layer = NULL, xml_props = NULL,
           xml_tileset = NULL;

   xml_map = ezxml_parse_str( (char*)tmdata, datasize );
   scaffold_check_null( xml_map );

   xml_tileset = ezxml_child( xml_map, "tileset" );
   scaffold_check_null( xml_tileset );
   while( NULL != xml_tileset ) {
      tilemap_parse_tileset( t, xml_tileset );
      //scaffold_check_nonzero( scaffold_error );
      xml_tileset = ezxml_next( xml_tileset );
   }

   xml_props = ezxml_child( xml_map, "properties" );
   tilemap_parse_properties( t, xml_props );

   xml_layer = ezxml_child( xml_map, "layer" );
   scaffold_check_null( xml_layer );
   while( NULL != xml_layer ) {
      tilemap_parse_layer( t, xml_layer );
      //scaffold_check_nonzero( scaffold_error );
      xml_layer = ezxml_next( xml_layer );
   }

cleanup:
   ezxml_free( xml_map );
   return;
}

void tilemap_load_file( TILEMAP* t, bstring filename ) {
   FILE* tmfile = NULL;
   uint8_t* tmdata = NULL;
   uint32_t datasize = 0;

   tmfile = fopen( bdata( filename ), "rb" );
   scaffold_check_null( tmfile );

   /* Allocate enough space to hold the map. */
   fseek( tmfile, 0, SEEK_END );
   datasize = ftell( tmfile );
   tmdata = calloc( datasize, sizeof( uint8_t ) + 1 ); /* +1 for term. */
   scaffold_check_null( tmdata );
   fseek( tmfile, 0, SEEK_SET );

   /* Read and close the map. */
   fread( tmdata, sizeof( uint8_t ), datasize, tmfile );
   fclose( tmfile );
   tmfile = NULL;

   tilemap_load_data( t, tmdata, datasize );

cleanup:
   if( NULL != tmdata ) {
      free( tmdata );
   }
   return;
}

void tilemap_iterate_screen_row(
   TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
   void (*callback)( TILEMAP* t, uint32_t x, uint32_t y )
) {

}
