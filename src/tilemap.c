#include "tilemap.h"

#include "ezxml/ezxml.h"
#include "b64/b64.h"

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

#include <memory.h>
#include <string.h>

void tilemap_init( TILEMAP* t ) {
#ifdef INIT_ZEROES
   memset( t, '\0', sizeof( TILEMAP ) );
#endif /* INIT_ZEROES */

   vector_init( &(t->layers) );
   vector_init( &(t->positions) );
   vector_init( &(t->tilesets) );
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
   for( i = 0 ; vector_count( &(t->tilesets) ) > i ; i++ ) {
      // TODO: tilemap_tileset_free( vector_get( &(t->positions), i ) );
   }
   vector_free( &(t->tilesets) );
   ezxml_free( t->xml_data );
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

static void tilemap_parse_tileset_image( TILEMAP* t, ezxml_t xml_image ) {
   TILEMAP_TILESET_IMAGE* image_info = NULL;
   bstring image_buffer = NULL;
   const char* xml_attr;
   char* image_ezxml_import = NULL;
   int bstr_result;
   size_t image_len = 0;
   BYTE* image_export = NULL;
   bstring buffer = NULL;
   TILEMAP_TILESET* set = NULL;

   scaffold_error = 0;

   set = (TILEMAP_TILESET*)vector_get( &(t->tilesets), vector_count( &(t->tilesets) ) - 1 );
   scaffold_check_null( set );

   image_buffer = bfromcstralloc( 1024, "" );
   scaffold_check_null( image_buffer );
   buffer = bfromcstralloc( 1024, "" );
   scaffold_check_null( buffer );

   image_info = (TILEMAP_TILESET_IMAGE*)calloc( 1, sizeof( TILEMAP_TILESET_IMAGE ) );
   scaffold_check_null( image_info );

   /* TODO: Decode serialized image data if present. */
   xml_attr = ezxml_attr( xml_image, "source" );
   if( NULL != xml_attr && 0 == strncmp( "inline", xml_attr, 6 ) ) {
      image_ezxml_import = ezxml_txt( xml_image );
      scaffold_check_null( image_ezxml_import );

      bstr_result = bassigncstr( image_buffer, image_ezxml_import );
      free( image_ezxml_import );
      scaffold_check_nonzero( bstr_result );

      image_ezxml_import = b64_decode( &image_len, image_buffer );
      scaffold_check_nonzero( scaffold_error );

      graphics_surface_new( image_info->image, 0, 0, 0, 0 );
      scaffold_check_nonzero( scaffold_error );
      graphics_set_image_data( image_info->image, (BYTE*)image_ezxml_import, image_len );
      free( image_ezxml_import );
      scaffold_check_nonzero( scaffold_error );

   } else if( NULL != xml_attr ) {
      /* See if this image has an external image file. */
      bassigncstr( buffer, xml_attr );
      image_len = 1024;

      graphics_surface_new( image_info->image, 0, 0, 0, 0 );
      graphics_set_image_path( image_info->image, buffer );
      scaffold_check_null( image_info->image->surface );

      /* Save the image to the XML to share later. */
      image_export = graphics_export_image_data( image_info->image, &image_len );
      scaffold_check_null( image_export );
      scaffold_check_zero( strlen( (char*)image_export ) );

      b64_encode( image_export, image_len, image_buffer, 40 );
      scaffold_check_nonzero( scaffold_error );

      /* TODO: This's a bstr2cstr() call that will be freed with free(), FYI. */
      ezxml_set_txt( xml_image, bstr2cstr( image_buffer, '\0' ) );
      ezxml_set_attr( xml_image, "source", "inline" );

      free( image_export );
   }

   scaffold_check_null( image_info->image );

   vector_add( &(set->images), image_info );
   image_info = NULL;

cleanup:
   bdestroy( image_buffer );
   bdestroy( buffer );
   if( NULL != image_info ) {
      graphics_surface_cleanup( image_info->image );
      free( image_info );
   }
   return;
}

static void tilemap_parse_tileset( TILEMAP* t, ezxml_t xml_tileset ) {
   TILEMAP_TILESET* set = NULL;
   ezxml_t
   xml_image,
   xml_tile,
   xml_terraintypes,
   xml_terrain;
   const char* xml_attr;
   bstring buffer = NULL;
   TILEMAP_TILE_DATA* tile_info = NULL;
   TILEMAP_TERRAIN_DATA* terrain_info = NULL;
   struct bstrList* terrain_list = NULL;
   int i;

   scaffold_error = 0;

   scaffold_check_null( xml_tileset );

   buffer = bfromcstralloc( 30, "" );
   scaffold_check_null( buffer );

   /* Try to grab the image list early. If it's missing, just get out. */
   xml_image = ezxml_child( xml_tileset, "image" );
   scaffold_check_null( xml_image );

   set = (TILEMAP_TILESET*)calloc( 1, sizeof( TILEMAP_TILESET ) );
   scaffold_check_null( set );

   /* vector_init( &(set->images) );
   vector_init( &(set->tiles) );
   vector_init( &(set->terrain) ); */

   vector_add( &(t->tilesets), set );

   // XXX
   //tilemap_tileset_lock_images( set, TRUE );
   while( NULL != xml_image ) {
      tilemap_parse_tileset_image( t, xml_image );
      scaffold_check_nonzero( scaffold_error ); /* Need an image! */
      xml_image = ezxml_next( xml_image );
   }
   //tilemap_tileset_lock_images( set, FALSE );

   xml_terraintypes = ezxml_child( xml_tileset, "terraintypes" );
   scaffold_check_null( xml_terraintypes );
   xml_terrain = ezxml_child( xml_terraintypes, "terrain" );

   // XXX
   //tilemap_tileset_lock_terrain( set, TRUE );
   while( NULL != xml_terrain ) {
      terrain_info = (TILEMAP_TERRAIN_DATA*)calloc( 1, sizeof( TILEMAP_TERRAIN_DATA ) );
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
   //tilemap_tileset_lock_terrain( set, FALSE );

   // XXX
   //tilemap_tileset_lock_tiles( set, TRUE );
   xml_tile = ezxml_child( xml_tileset, "tile" );
   scaffold_check_null( xml_tile );
   while( NULL != xml_tile ) {
      tile_info = (TILEMAP_TILE_DATA*)calloc( 1, sizeof( TILEMAP_TILE_DATA ) );
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
   //tilemap_tileset_lock_tiles( set, FALSE );

cleanup:
   /* TODO: Don't scrap the whole tileset for a bad tile or two. */
   bdestroy( buffer );
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

static void tilemap_parse_layer( TILEMAP* t, ezxml_t xml_layer ) {
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
   layer->tiles = (uint16_t*)calloc( layer->tiles_alloc, sizeof( uint16_t ) );
   scaffold_check_null( layer->tiles );

   // XXX
   //tilemap_layer_lock_tiles( layer, TRUE );
   for( i = 0 ; tiles_list->qty > i ; i++ ) {
      xml_attr = bdata( tiles_list->entry[i] );
      scaffold_check_null( xml_attr );
      layer->tiles[i] = atoi( xml_attr );
      layer->tiles_count++;
   }
   //tilemap_layer_lock_tiles( layer, FALSE );

   vector_add( &(t->layers), layer );

cleanup:
   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      tilemap_layer_free( layer );
   }
   bdestroy( buffer );
   bstrListDestroy( tiles_list );
   return;
}

void tilemap_serialize( TILEMAP* t ) {
#if 0
   char* ezxml_buffer = NULL;
   mz_zip_archive buffer_archive;
   mz_bool zip_result = 0;
   void* zip_buffer = NULL;
   size_t zip_buffer_size = 0;
   size_t xml_buffer_size = 0;

   /* Ensure sanity. */
   memset( &buffer_archive, 0, sizeof( mz_zip_archive ) );
#endif
   //scaffold_check_null( buffer );
   scaffold_check_null( t );

   /* Serialize and compress. */
   // XXX
   //tilemap_lock_layers( t, TRUE );
   //tilemap_lock_positions( t, TRUE );
   //tilemap_lock_tilesets( t, TRUE );

   scaffold_print_debug( "Serializing map data to XML...\n" );
   t->serialize_buffer = ezxml_toxml( t->xml_data );
   scaffold_check_null( t->serialize_buffer );
   t->serialize_len = strlen( t->serialize_buffer );

#if 0
   xml_buffer_size = strlen( ezxml_buffer );

   scaffold_print_debug( "Compressing XML data...\n" );
   zip_result = mz_zip_writer_init_heap( &buffer_archive, 0, TILEMAP_SERIALIZE_RESERVED );
   scaffold_check_zero( zip_result );
   zip_result = mz_zip_writer_add_mem(
      &buffer_archive,
      "mapdata.xml",
      ezxml_buffer,
      xml_buffer_size,
      MZ_BEST_SPEED
   );
   scaffold_check_zero( zip_result );
   zip_result = mz_zip_writer_finalize_heap_archive( &buffer_archive, &zip_buffer, &zip_buffer_size );
   scaffold_check_zero( zip_result );
   zip_result = mz_zip_writer_end( &buffer_archive );
   scaffold_check_zero( zip_result );
   b64_encode( (unsigned char*)zip_buffer, zip_buffer_size, buffer, TILEMAP_SERIALIZE_CHUNKSIZE );
   scaffold_check_nonzero( scaffold_error );

   scaffold_print_debug( "Serialization complete.\n" );
cleanup:
   if( NULL != zip_buffer ) {
      free( zip_buffer );
   }
   if( NULL != ezxml_buffer ) {
      free( ezxml_buffer );
   }
#endif
cleanup:
   return;
}

void tilemap_load_data( TILEMAP* t, const BYTE* tmdata, int datasize ) {
   ezxml_t xml_layer = NULL,
      xml_props = NULL,
      xml_tileset = NULL;

   t->xml_data = ezxml_parse_str( (char*)tmdata, datasize );
   scaffold_check_null( t->xml_data );

   xml_tileset = ezxml_child( t->xml_data, "tileset" );
   scaffold_check_null( xml_tileset );
   while( NULL != xml_tileset ) {
      tilemap_parse_tileset( t, xml_tileset );
      //scaffold_check_nonzero( scaffold_error );
      xml_tileset = ezxml_next( xml_tileset );
   }

   xml_props = ezxml_child( t->xml_data, "properties" );
   tilemap_parse_properties( t, xml_props );

   xml_layer = ezxml_child( t->xml_data, "layer" );
   scaffold_check_null( xml_layer );
   while( NULL != xml_layer ) {
      tilemap_parse_layer( t, xml_layer );
      //scaffold_check_nonzero( scaffold_error );
      xml_layer = ezxml_next( xml_layer );
   }

cleanup:
   return;
}

void tilemap_load_file( TILEMAP* t, bstring filename ) {
   FILE* tmfile = NULL;
   BYTE* tmdata = NULL;
   size_t datasize = 0;

   tmfile = fopen( bdata( filename ), "rb" );
   scaffold_check_null( tmfile );

   /* Allocate enough space to hold the map. */
   fseek( tmfile, 0, SEEK_END );
   datasize = ftell( tmfile );
   tmdata = (BYTE*)calloc( datasize, sizeof( BYTE ) + 1 ); /* +1 for term. */
   scaffold_check_null( tmdata );
   fseek( tmfile, 0, SEEK_SET );

   /* Read and close the map. */
   fread( tmdata, sizeof( BYTE ), datasize, tmfile );
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
