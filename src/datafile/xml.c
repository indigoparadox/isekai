
#include "../datafile.h"

#include "../b64.h"
#include "../hashmap.h"

static void datafile_tilemap_parse_properties_ezxml( struct TILEMAP* t, ezxml_t xml_props ) {
   ezxml_t xml_prop_iter = NULL;
   const char* channel_c = NULL;
   int bstr_retval;

   scaffold_check_null( xml_props );

   xml_prop_iter = ezxml_child( xml_props, "property" );

   while( NULL != xml_prop_iter ) {
      if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "channel" ) ) {
         channel_c = ezxml_attr( xml_prop_iter, "value" );
         scaffold_check_null( channel_c );
         bstr_retval = bassigncstr( t->lname, channel_c );
         scaffold_check_nonzero( bstr_retval );
      }

#if 0
      if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "light_str" ) ) {
          map_out->light_str = atoi( ezxml_attr( xml_prop_iter, "value" ) );

      if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "music_path" ) ) {
          map_out->music_path = bfromcstr( ezxml_attr( xml_prop_iter, "value" ) );

      if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "time_moves" ) ) {
          if( 0 == strcmp( ezxml_attr( xml_prop_iter, "value" ), "true" ) ) {
             map_out->time_moves = TRUE;
          } else {
             map_out->time_moves = FALSE;
          }
#endif

      xml_prop_iter = xml_prop_iter->next;
   }

cleanup:
   return;
}

static void datafile_tilemap_parse_tileset_ezxml_image(
   struct TILEMAP_TILESET* set, ezxml_t xml_image, BOOL local_images
) {
   GRAPHICS* g_image = NULL;
   const char* xml_attr;
   bstring buffer = NULL;
   int bstr_res;
#ifndef USE_REQUESTED_GRAPHICS_EXT
   SCAFFOLD_SIZE dot_pos = 0;
#endif /* USE_REQUESTED_GRAPHICS_EXT */

   scaffold_error = 0;

   scaffold_check_null( set );

   if( !vector_ready( &(set->images) ) ) {
      hashmap_init( &(set->images) );
   }

   buffer = bfromcstralloc( 1024, "" );
   scaffold_check_null( buffer );

   xml_attr = ezxml_attr( xml_image, "source" );
   bstr_res = bassigncstr( buffer, xml_attr );
   scaffold_check_nonzero( bstr_res );

#ifndef USE_REQUESTED_GRAPHICS_EXT
   dot_pos = bstrrchr( buffer, '.' );
   bstr_res = btrunc( buffer, dot_pos );
   scaffold_check_nonzero( bstr_res );
   bstr_res = bcatcstr( buffer, GRAPHICS_RASTER_EXTENSION );
   scaffold_check_nonzero( bstr_res );
   scaffold_print_debug(
      "Tilemap: Tileset filename adjusted to: %s\n", bdata( buffer )
   );
#endif /* USE_REQUESTED_GRAPHICS_EXT */

   /* The key with NULL means we need to load this image. */
   hashmap_put( &(set->images), buffer, NULL );

cleanup:
   bdestroy( buffer );
   if( NULL != g_image ) {
      graphics_surface_free( g_image );
   }
   return;
}

static void datafile_tilemap_parse_tileset_ezxml( struct TILEMAP* t, ezxml_t xml_tileset, BOOL local_images ) {
   struct TILEMAP_TILESET* set = NULL;
   ezxml_t
      xml_image,
      xml_tile,
      xml_terraintypes,
      xml_terrain;
   const char* xml_attr;
   bstring buffer = NULL;
   struct TILEMAP_TILE_DATA* tile_info = NULL;
   struct TILEMAP_TERRAIN_DATA* terrain_info = NULL;
   struct bstrList* terrain_list = NULL;
   int i,
      bstr_retval;

   scaffold_error = 0;

   scaffold_check_null( xml_tileset );

   buffer = bfromcstralloc( 30, "" );
   scaffold_check_null( buffer );

   /* Try to grab the image list early. If it's missing, just get out. */
   xml_image = ezxml_child( xml_tileset, "image" );
   scaffold_check_null( xml_image );

   set = (struct TILEMAP_TILESET*)calloc( 1, sizeof( struct TILEMAP_TILESET ) );
   scaffold_check_null( set );

   xml_attr = ezxml_attr( xml_tileset, "firstgid" );
   scaffold_check_null( xml_attr );
   set->firstgid = atoi( xml_attr );

   xml_attr = ezxml_attr( xml_tileset, "tilewidth" );
   scaffold_check_null( xml_attr );
   set->tilewidth = atoi( xml_attr );

   xml_attr = ezxml_attr( xml_tileset, "tileheight" );
   scaffold_check_null( xml_attr );
   set->tileheight = atoi( xml_attr );

   bstr_retval = bassigncstr( buffer, ezxml_attr( xml_tileset, "name" ) );
   scaffold_check_nonzero( bstr_retval );
   hashmap_put( &(t->tilesets), buffer, set );

   while( NULL != xml_image ) {
      datafile_tilemap_parse_tileset_ezxml_image( set, xml_image, local_images );
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
      terrain_info = (struct TILEMAP_TERRAIN_DATA*)calloc( 1, sizeof( struct TILEMAP_TERRAIN_DATA ) );
      scaffold_check_null( terrain_info );

      xml_attr = ezxml_attr( xml_terrain, "name" );
      scaffold_check_null( xml_attr );
      bstr_retval = bassigncstr( buffer, xml_attr );
      scaffold_check_nonzero( bstr_retval );

      terrain_info->name = bstrcpy( buffer );
      scaffold_check_null( terrain_info->name );

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
      tile_info = (struct TILEMAP_TILE_DATA*)calloc( 1, sizeof( struct TILEMAP_TILE_DATA ) );
      scaffold_check_null( tile_info );

      xml_attr = ezxml_attr( xml_tile, "id" );
      scaffold_check_null( xml_attr );
      tile_info->id = atoi( xml_attr );

      xml_attr = ezxml_attr( xml_tile, "terrain" );
      scaffold_check_null( xml_attr );

      /* Parse the terrain attribute. */
      bstr_retval = bassigncstr( buffer, xml_attr );
      scaffold_check_nonzero( bstr_retval );
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

static void datafile_tilemap_parse_layer_ezxml( struct TILEMAP* t, ezxml_t xml_layer ) {
   struct TILEMAP_LAYER* layer = NULL;
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

static void datafile_tilemap_parse_objectgroup_ezxml( struct TILEMAP* t, ezxml_t xml_layer ) {
   struct TILEMAP_LAYER* layer = NULL;
   ezxml_t xml_object = NULL;
   bstring buffer = NULL;
   const char* xml_attr = NULL;
   struct TILEMAP_POSITION* obj_out = NULL;

   scaffold_check_null( xml_layer );

   buffer = bfromcstralloc( TILEMAP_NAME_ALLOC, "" );

   xml_object = ezxml_child( xml_layer, "object" );
   scaffold_check_null( xml_object );
   scaffold_print_debug( "Loading object spawns...\n" );
   while( NULL != xml_object ) {
      obj_out = (struct TILEMAP_POSITION*)calloc(
         1, sizeof( struct TILEMAP_POSITION )
      );
      scaffold_check_null( obj_out );

      xml_attr = ezxml_attr( xml_object, "x" );
      /* TODO: _continue versions of the abort macros.*/
      scaffold_check_null( xml_attr );
      obj_out->x = atoi( xml_attr ) / TILEMAP_OBJECT_SPAWN_DIVISOR;

      xml_attr = ezxml_attr( xml_object, "y" );
      scaffold_check_null( xml_attr );
      obj_out->y = atoi( xml_attr ) / TILEMAP_OBJECT_SPAWN_DIVISOR;

      xml_attr = ezxml_attr( xml_object, "name" );
      scaffold_check_null( xml_attr );
      bassigncstr( buffer, xml_attr );
      xml_attr = ezxml_attr( xml_object, "type" );
      scaffold_check_null( xml_attr );
      if( 0 == strncmp( xml_attr, "spawn", 5 ) ) {
         scaffold_print_debug(
            "Player spawn at: %d, %d\n", obj_out->x, obj_out->y
         );
         hashmap_put( &(t->player_spawns), buffer, obj_out );
      } else {
         /* We don't know how to handle this yet. */
         scaffold_print_error(
            "Unknown object at: %d, %d\n", obj_out->x, obj_out->y
         );
         free( obj_out );
      }

      obj_out = NULL;
      xml_object = ezxml_next( xml_object );
   }

cleanup:
   if( NULL != obj_out ) {
      /* Something went wrong. */
      free( obj_out );
   }
   bdestroy( buffer );
   return;
}

void datafile_parse_tilemap_ezxml( struct TILEMAP* t, const BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images ) {
   ezxml_t xml_layer = NULL,
      xml_props = NULL,
      xml_tileset = NULL,
      xml_data = NULL;

   scaffold_assert( strlen( tmdata ) == datasize );

   xml_data = ezxml_parse_str( (char*)tmdata, datasize );
   scaffold_check_null( xml_data );

   xml_tileset = ezxml_child( xml_data, "tileset" );
   scaffold_check_null( xml_tileset );
   while( NULL != xml_tileset ) {
      datafile_tilemap_parse_tileset_ezxml( t, xml_tileset, local_images );
      xml_tileset = ezxml_next( xml_tileset );
   }

   xml_props = ezxml_child( xml_data, "properties" );
   datafile_tilemap_parse_properties_ezxml( t, xml_props );

   xml_layer = ezxml_child( xml_data, "layer" );
   scaffold_check_null( xml_layer );
   while( NULL != xml_layer ) {
      datafile_tilemap_parse_layer_ezxml( t, xml_layer );
      xml_layer = ezxml_next( xml_layer );
   }

   xml_layer = ezxml_child( xml_data, "objectgroup" );
   scaffold_check_null( xml_layer );
   while( NULL != xml_layer ) {
      datafile_tilemap_parse_objectgroup_ezxml( t, xml_layer );
      xml_layer = ezxml_next( xml_layer );
   }

   t->sentinal = TILEMAP_SENTINAL;

cleanup:
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
   return;
}

void datafile_reserialize_tilemap( struct TILEMAP* t ) {
}
