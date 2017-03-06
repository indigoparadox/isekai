
#include "../datafile.h"

#include "../b64/b64.h"
#include "../hashmap.h"
#include "../gamedata.h"

static void datafile_tilemap_parse_properties( TILEMAP* t, ezxml_t xml_props ) {
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

static void datafile_tilemap_parse_tileset_image(
   TILEMAP_TILESET* set, ezxml_t xml_image, BOOL local_images
) {
   GRAPHICS* g_image = NULL;
   const char* xml_attr;
   bstring buffer = NULL;

   scaffold_error = 0;

   scaffold_check_null( set );

   if( !vector_ready( &(set->images) ) ) {
      hashmap_init( &(set->images) );
   }

   buffer = bfromcstralloc( 1024, "" );
   scaffold_check_null( buffer );

   xml_attr = ezxml_attr( xml_image, "source" );
   bassigncstr( buffer, xml_attr );

   /* The key with NULL means we need to load this image. */
   hashmap_put( &(set->images), bstrcpy( buffer ), NULL );

cleanup:
   bdestroy( buffer );
#ifdef EZXML_EMBEDDED_IMAGES
   bdestroy( image_buffer );
   if( NULL != image_ezxml_export ) {
      free( image_ezxml_export );
   }
   if( NULL != image_export ) {
      free( image_export );
   }
#endif /* EZXML_EMBEDDED_IMAGES */
   if( NULL != g_image ) {
      graphics_surface_free( g_image );
   }
   return;
}

static void datafile_tilemap_parse_tileset( TILEMAP* t, ezxml_t xml_tileset ) {
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

   xml_attr = ezxml_attr( xml_tileset, "firstgid" );
   scaffold_check_null( xml_attr );
   set->firstgid = atoi( xml_attr );

   xml_attr = ezxml_attr( xml_tileset, "tilewidth" );
   scaffold_check_null( xml_attr );
   set->tilewidth = atoi( xml_attr );

   xml_attr = ezxml_attr( xml_tileset, "tileheight" );
   scaffold_check_null( xml_attr );
   set->tileheight = atoi( xml_attr );

   bassigncstr( buffer, ezxml_attr( xml_tileset, "name" ) );
   hashmap_put( &(t->tilesets), buffer, set );

   while( NULL != xml_image ) {
      datafile_tilemap_parse_tileset_image( set, xml_image, t->local_images );
      scaffold_check_nonzero( scaffold_error ); /* Need an image! */
      xml_image = ezxml_next( xml_image );
   }

   if( !hashmap_ready( &(set->terrain) ) ) {
      hashmap_init( &(set->terrain) );
   }

   xml_terraintypes = ezxml_child( xml_tileset, "terraintypes" );
   scaffold_check_null( xml_terraintypes );
   xml_terrain = ezxml_child( xml_terraintypes, "terrain" );
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

      hashmap_put( &(set->terrain), buffer, terrain_info );
      terrain_info = NULL;

      xml_terrain = ezxml_next( xml_terrain );
   }


   if( !vector_ready( &(set->tiles) ) ) {
      vector_init( &(set->tiles) );
   }

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

static void datafile_tilemap_parse_layer( TILEMAP* t, ezxml_t xml_layer ) {
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

   /* Split the tiles list CSV into an array of uint16. */
   buffer = bfromcstr( ezxml_txt( xml_layer_data ) );
   scaffold_check_null( buffer );
   tiles_list = bsplit( buffer, ',' );
   scaffold_check_null( tiles_list );

   /* Convert each tile terrain ID string into an int16 and stow in array. */

   /* TODO: Lock the list while adding, somehow. */
   for( i = 0 ; tiles_list->qty > i ; i++ ) {
      xml_attr = bdata( tiles_list->entry[i] );
      scaffold_check_null( xml_attr );
      vector_add_scalar( &(layer->tiles), atoi( xml_attr ), TRUE );
   }

   bassigncstr( buffer, ezxml_attr( xml_layer, "name" ) );
   hashmap_put( &(t->layers), buffer, layer );

cleanup:
   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      tilemap_layer_free( layer );
   }
   bdestroy( buffer );
   bstrListDestroy( tiles_list );
   return;
}

void datafile_parse_tilemap( void* targ, bstring filename, const BYTE* tmdata, size_t datasize ) {
   ezxml_t xml_layer = NULL,
      xml_props = NULL,
      xml_tileset = NULL,
      xml_data;
   TILEMAP* t = (TILEMAP*)targ;

   bassign( t->serialize_filename, filename );
   scaffold_check_null( t->serialize_filename );

   xml_data = ezxml_parse_str( (char*)tmdata, datasize );
   scaffold_check_null( xml_data );

   xml_tileset = ezxml_child( xml_data, "tileset" );
   scaffold_check_null( xml_tileset );
   while( NULL != xml_tileset ) {
      datafile_tilemap_parse_tileset( t, xml_tileset );
      //scaffold_check_nonzero( scaffold_error );
      xml_tileset = ezxml_next( xml_tileset );
   }

   xml_props = ezxml_child( xml_data, "properties" );
   datafile_tilemap_parse_properties( t, xml_props );

   xml_layer = ezxml_child( xml_data, "layer" );
   scaffold_check_null( xml_layer );
   while( NULL != xml_layer ) {
      datafile_tilemap_parse_layer( t, xml_layer );
      //scaffold_check_nonzero( scaffold_error );
      xml_layer = ezxml_next( xml_layer );
   }

   /* Shortcut to serialize for later. */
   scaffold_check_null( t );
   scaffold_print_debug( "Serializing map data to XML...\n" );
   t->serialize_buffer = ezxml_toxml( xml_data );
   scaffold_check_null( t->serialize_buffer );

   t->sentinal = TILEMAP_SENTINAL;

cleanup:
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
   return;
}

void datafile_reserialize_tilemap( TILEMAP* t ) {
}

void datafile_load_file(
   void* targ_struct, bstring filename, BOOL local_images, datafile_cb cb
) {
   BYTE* tmdata = NULL;
   size_t tmsize = 0;

   scaffold_read_file_contents( filename, &tmdata, &tmsize );

   cb( targ_struct, filename, tmdata, tmsize );

   if( NULL != tmdata ) {
      free( tmdata );
   }
   return;
}
