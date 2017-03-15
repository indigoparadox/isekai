
#include "../datafile.h"

#include "../b64.h"
#include "../hashmap.h"

ezxml_t datafile_mobile_ezxml_peek_mob_id(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring mob_id_buffer
) {
   ezxml_t xml_data = NULL;
   const char* mob_id_c = NULL;
   int bstr_retval;

   scaffold_assert( NULL != mob_id_buffer );
   scaffold_check_null( tmdata );

   xml_data = ezxml_parse_str( tmdata, datasize );
   scaffold_check_null( xml_data );

   mob_id_c = ezxml_attr( xml_data, "id" );
   scaffold_check_null( mob_id_c );

   bstr_retval = bassigncstr( mob_id_buffer, mob_id_c );
   scaffold_check_nonzero( bstr_retval );

cleanup:
   return xml_data;

}

static void datafile_mobile_parse_sprite_ezxml(
   struct MOBILE* o, ezxml_t xml_sprite, BOOL local_images
) {
   ezxml_t xml_frame_iter = NULL;
   const char* xml_attr = NULL;
   int bstr_retval;
   struct MOBILE_SPRITE_DEF* sprite = NULL;

   scaffold_check_null( xml_sprite );

   sprite =
      (struct MOBILE_SPRITE_DEF*)calloc( 1, sizeof( struct MOBILE_SPRITE_DEF* ) );

   /* TODO: Case insensitivity. */
   xml_attr = ezxml_attr( xml_sprite, "id" );
   scaffold_check_null( xml_attr );
   sprite->id = atoi( xml_attr );

   vector_set( &(o->sprite_defs), sprite->id, sprite, TRUE );
   sprite = NULL;

cleanup:
   if( NULL != sprite ) {
      free( sprite );
   }
   return;
}

static void datafile_mobile_parse_animation_ezxml(
   struct MOBILE* o, ezxml_t xml_animation
) {
   ezxml_t xml_frame_iter = NULL;
   const char* xml_attr = NULL;
   int bstr_retval,
      frame_id;
   struct MOBILE_ANI_DEF* animation = NULL;
   bstring name_dir = NULL;
   struct MOBILE_SPRITE_DEF* sprite = NULL;

   scaffold_check_null( xml_animation );

   name_dir = bfromcstralloc( 10, "" );
   scaffold_check_null( name_dir );

   animation =
      (struct MOBILE_ANI_DEF*)calloc( 1, sizeof( struct MOBILE_ANI_DEF ) );

   /* TODO: Case insensitivity. */
   xml_attr = ezxml_attr( xml_animation, "facing" );
   scaffold_check_null( xml_attr );
   if( 0 == strncmp( "down", xml_attr, 4 ) ) {
      animation->facing = MOBILE_FACING_DOWN;
   } else if( 0 == strncmp( "up", xml_attr, 2 ) ) {
      animation->facing = MOBILE_FACING_UP;
   } else if( 0 == strncmp( "left", xml_attr, 4 ) ) {
      animation->facing = MOBILE_FACING_LEFT;
   } else if( 0 == strncmp( "right", xml_attr, 5 ) ) {
      animation->facing = MOBILE_FACING_RIGHT;
   } else {
      goto cleanup;
   }
   bstr_retval =
      bassignformat( name_dir, "%s-%s", bdata( animation->name ), xml_attr );
   scaffold_check_nonzero( bstr_retval );

   xml_attr = ezxml_attr( xml_animation, "name" );
   scaffold_check_null( xml_attr );
   animation->name = bfromcstr( xml_attr );

   xml_attr = ezxml_attr( xml_animation, "speed" );
   scaffold_check_null( xml_attr );
   animation->speed = atoi( xml_attr );

   vector_init( &(animation->frames) );

   xml_frame_iter = ezxml_child( xml_animation, "frame" );
   while( NULL != xml_frame_iter ) {
      xml_attr = ezxml_attr( xml_frame_iter, "id" );
      scaffold_check_null( xml_attr );
      frame_id = atoi( xml_attr );

      sprite = vector_get( &(o->sprite_defs), frame_id );
      if( NULL == sprite ) {
         scaffold_print_error(
            "Bad frame in parsed mobile animation: %d\n", frame_id
         );
      }

      vector_add( &(animation->frames), sprite );

      xml_frame_iter = ezxml_next( xml_frame_iter );
   }

   hashmap_put( &(o->ani_defs), name_dir, animation );
   animation = NULL;

cleanup:
   if( NULL != animation ) {
      free( animation );
   }
   bdestroy( name_dir );
   return;
}

static void datafile_mobile_parse_script_ezxml(
   struct MOBILE* o, ezxml_t xml_script
) {
   /* TODO */
}

void datafile_parse_mobile_ezxml_t(
   struct MOBILE* o, ezxml_t xml_data, BOOL local_images
) {
   ezxml_t xml_sprites = NULL,
      xml_sprite_iter = NULL,
      xml_props = NULL,
      xml_animations = NULL,
      xml_animation_iter = NULL,
      xml_scripts = NULL,
      xml_script_iter = NULL,
      xml_image = NULL;
   const char* xml_attr = NULL;
   int bstr_retval;

   scaffold_check_null( xml_data );

   xml_attr = ezxml_attr( xml_data, "id" );
   scaffold_check_null( xml_attr );
   if( NULL == o->mob_id ) {
      o->mob_id = bfromcstr( xml_attr );
   } else {
      bassigncstr( o->mob_id, xml_attr );
   }

   xml_attr = ezxml_attr( xml_data, "spritewidth" );
   scaffold_check_null( xml_attr );
   o->sprite_width = atoi( xml_attr );

   xml_attr = ezxml_attr( xml_data, "spriteheight" );
   scaffold_check_null( xml_attr );
   o->sprite_height = atoi( xml_attr );

   xml_sprites = ezxml_child( xml_data, "sprites" );
   scaffold_check_null( xml_sprites );

   xml_sprite_iter = ezxml_child( xml_sprites, "sprite" );
   while( NULL != xml_sprite_iter ) {
      datafile_mobile_parse_sprite_ezxml( o, xml_sprite_iter, local_images );
      xml_sprite_iter = ezxml_next( xml_sprite_iter );
   }

   xml_animations = ezxml_child( xml_data, "animations" );
   scaffold_check_null( xml_animations );

   xml_animation_iter = ezxml_child( xml_animations, "animation" );
   while( NULL != xml_animation_iter ) {
      datafile_mobile_parse_animation_ezxml( o, xml_animation_iter );
      xml_animation_iter = ezxml_next( xml_animation_iter );
   }

   xml_scripts = ezxml_child( xml_data, "scripts" );
   scaffold_check_null( xml_scripts );

   xml_script_iter = ezxml_child( xml_scripts, "script" );
   while( NULL != xml_script_iter ) {
      datafile_mobile_parse_script_ezxml( o, xml_script_iter );
      xml_script_iter = ezxml_next( xml_script_iter );
   }

   xml_image = ezxml_child( xml_data, "image" );
   scaffold_check_null( xml_image );
   xml_attr = ezxml_attr( xml_image, "src" );
   scaffold_check_null( xml_attr );
   if( NULL == o->sprites_filename ) {
      o->sprites_filename = bfromcstr( xml_attr );
   } else {
      bstr_retval = bassigncstr( o->sprites_filename, xml_attr );
      scaffold_check_nonzero( bstr_retval );
   }

cleanup:
   return;
}

void datafile_parse_mobile_ezxml_string(
   struct MOBILE* o, BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images
) {
   ezxml_t xml_data = NULL;
#ifdef EZXML_STRICT
   SCAFFOLD_SIZE datasize_check = 0;
#endif /* EZXML_STRICT */

   scaffold_check_null( tmdata );

#ifdef EZXML_STRICT
   datasize_check = strlen( (const char*)o );
   scaffold_assert( datasize_check == datasize );
#endif /* EZXML_STRICT */

   xml_data = ezxml_parse_str( (char*)tmdata, datasize );
   scaffold_check_null( xml_data );

   datafile_parse_mobile_ezxml_t( o, xml_data, local_images );

cleanup:
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
   return;
}

ezxml_t datafile_tilemap_ezxml_peek_lname(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring lname_buffer
) {
   ezxml_t xml_props,
      xml_prop_iter = NULL,
      xml_data = NULL;
   const char* channel_c = NULL;
   int bstr_retval;

   scaffold_assert( NULL != lname_buffer );
   scaffold_check_null( tmdata );

   xml_data = ezxml_parse_str( tmdata, datasize );
   scaffold_check_null( xml_data );

   xml_props = ezxml_child( xml_data, "properties" );
   scaffold_check_null( xml_props );

   xml_prop_iter = ezxml_child( xml_props, "property" );

   while( NULL != xml_prop_iter ) {
      if( 0 == strcmp( ezxml_attr( xml_prop_iter, "name" ), "channel" ) ) {
         channel_c = ezxml_attr( xml_prop_iter, "value" );
         scaffold_check_null( channel_c );
         bstr_retval = bassigncstr( lname_buffer, channel_c );
         scaffold_check_nonzero( bstr_retval );
         break;
      }
      xml_prop_iter = ezxml_next( xml_prop_iter );
   }

cleanup:
   return xml_data;
}

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

static void datafile_tilemap_parse_tileset_ezxml_terrain(
   struct TILEMAP_TILESET* set, ezxml_t xml_terrain, SCAFFOLD_SIZE id
) {
   struct TILEMAP_TERRAIN_DATA* terrain_info = NULL;
   const char* xml_attr;
   ezxml_t xml_props = NULL,
      xml_prop_iter = NULL;

   terrain_info = (struct TILEMAP_TERRAIN_DATA*)calloc(
      1, sizeof( struct TILEMAP_TERRAIN_DATA )
   );
   scaffold_check_null( terrain_info );

   xml_attr = ezxml_attr( xml_terrain, "name" );
   terrain_info->name = bfromcstr( xml_attr );
   scaffold_check_null( terrain_info->name );

   xml_props = ezxml_child( xml_terrain, "properties" );
   scaffold_check_null( xml_props );

   xml_prop_iter = ezxml_child( xml_props, "property" );
   while( NULL != xml_prop_iter ) {
      xml_attr = ezxml_attr( xml_prop_iter, "name" );

      if( 0 == strncmp( "movement", xml_attr, 8 ) ) {
         xml_attr = ezxml_attr( xml_prop_iter, "value" );
         if( NULL != xml_attr ) {
            terrain_info->movement = atoi( xml_attr );
         }
      } else if( 0 == strncmp( "cutoff", xml_attr, 6 ) ) {
         xml_attr = ezxml_attr( xml_prop_iter, "value" );
         if( NULL != xml_attr ) {
            terrain_info->cutoff = atoi( xml_attr );
         }
      }

      xml_prop_iter = ezxml_next( xml_prop_iter );
   }

   scaffold_print_debug(
      "Loaded terrain %d: %s: %d\n",
      id, bdata( terrain_info->name ), terrain_info->movement
   );

   xml_attr = ezxml_attr( xml_terrain, "tile" );
   scaffold_check_null( xml_attr );
   terrain_info->tile = atoi( xml_attr );

   terrain_info->id = id;

   vector_add( &(set->terrain), terrain_info );
   terrain_info = NULL;

cleanup:
   /* TODO: Don't scrap the whole tileset for a bad tile or two. */
   if( NULL != terrain_info ) {
      bdestroy( terrain_info->name );
      free( terrain_info );
   }
}

#define TERRAIN_ID_C_BUFFER_LENGTH 4

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
   int i,
      terrain_id_c_i,
      bstr_retval;
   SCAFFOLD_SIZE terrain_id = 0;
   struct TILEMAP_TERRAIN_DATA* terrain_info = NULL;
   const char* terrain_c = NULL;
   char terrain_id_c[TERRAIN_ID_C_BUFFER_LENGTH + 1] = { 0 };
#ifdef DEBUG
   SCAFFOLD_SIZE dbg_terrain_id[4];
   const char* dbg_terrain_name[4];
#endif /* DEBUG */

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

   /* Parse the terrain information. */

   if( !vector_ready( &(set->terrain) ) ) {
      vector_init( &(set->terrain) );
   }

   /*
   terrain_info = (struct TILEMAP_TERRAIN_DATA*)
      calloc( 1, sizeof( struct TILEMAP_TERRAIN_DATA ) );
   terrain_info->movement = TILEMAP_MOVEMENT_NORMAL;
   terrain_info->name = bfromcstr( "Blank" );
   vector_add( &(set->terrain), terrain_info );
   terrain_info = NULL;
   */

   xml_terraintypes = ezxml_child( xml_tileset, "terraintypes" );
   scaffold_check_null( xml_terraintypes );
   xml_terrain = ezxml_child( xml_terraintypes, "terrain" );
   while( NULL != xml_terrain ) {

      datafile_tilemap_parse_tileset_ezxml_terrain(
         set, xml_terrain, terrain_id
      );

      xml_terrain = ezxml_next( xml_terrain );
      terrain_id++;
   }

   /* Parse the tile information. */

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
      scaffold_check_null( xml_attr );
      terrain_c = xml_attr; i = 0; terrain_id_c_i = 0; terrain_id_c[0] = '\0';
      while( 4 > i ) {
         if( ',' == *terrain_c || '\0' == *terrain_c) {
            if( '\0' != terrain_id_c[0] ) {
               terrain_info = vector_get( &(set->terrain), atoi( terrain_id_c ) );
               tile_info->terrain[i] = terrain_info;
            } else {
               tile_info->terrain[i] = NULL;
            }

            /* Next terrain cell. */
            terrain_id_c_i = 0;
            terrain_id_c[0] = '\0';
            i++;
         } else {
            if( TERRAIN_ID_C_BUFFER_LENGTH > i ) {
               /* Add a digit to the terrain info char string. */
               terrain_id_c[terrain_id_c_i] = *terrain_c;
               terrain_id_c[terrain_id_c_i + 1] = '\0';
            }
         }
         terrain_c++;
      }

#ifdef DEBUG_TILES_VERBOSE
      for( i = 0 ; 4 > i ; i++ ) {
         if( NULL == tile_info->terrain[i] ) {
            dbg_terrain_name[i] = NULL;
            dbg_terrain_id[i] = 0;
         } else {
            dbg_terrain_name[i] = bdata( tile_info->terrain[i]->name );
            dbg_terrain_id[i] = tile_info->terrain[i]->id;
         }
      }

      scaffold_print_debug(
         "Loaded tile %d: %d (%s), %d (%s), %d (%s), %d (%s)\n",
         tile_info->id,
         dbg_terrain_id[0],
         dbg_terrain_name[0],
         dbg_terrain_id[1],
         dbg_terrain_name[1],
         dbg_terrain_id[2],
         dbg_terrain_name[2],
         dbg_terrain_id[3],
         dbg_terrain_name[3]
      );
#endif /* DEBUG_TILES_VERBOSE */

      vector_set( &(set->tiles), tile_info->id, tile_info, TRUE );
      tile_info = NULL;

      xml_tile = ezxml_next( xml_tile );
   }

cleanup:
   /* TODO: Don't scrap the whole tileset for a bad tile or two. */
   bdestroy( buffer );
   if( NULL != tile_info ) {
      /* TODO: Delete tile info. */
   }
   return;
}

static void datafile_tilemap_parse_layer_ezxml(
   struct TILEMAP* t, ezxml_t xml_layer, SCAFFOLD_SIZE z
) {
   struct TILEMAP_LAYER* layer = NULL;
   ezxml_t xml_layer_data = NULL;
   bstring buffer = NULL;
   struct bstrList* tiles_list = NULL;
   int i;
   const char* xml_attr = NULL;
   int bstr_res = 0;
   struct TILEMAP_LAYER* prev_layer = NULL;

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

   layer->tilemap = t;
   layer->z = z;

   bstr_res = bassigncstr( buffer, ezxml_attr( xml_layer, "name" ) );
   scaffold_check_nonzero( bstr_res );
   hashmap_put( &(t->layers), buffer, layer );

   /* The map is as large as the largest layer. */
   if( layer->width > t->width ) {
      t->width = layer->width;
   }
   if( layer->height > t->height ) {
      t->height = layer->height;
   }

   /* Add to the layers linked list. */
   if( NULL == t->first_layer ) {
      t->first_layer = layer;
   } else {
      prev_layer = t->first_layer;
      while( NULL != prev_layer->next_layer ) {
         prev_layer = prev_layer->next_layer;
      }
      prev_layer->next_layer = layer;
   }

cleanup:
   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      tilemap_layer_free( layer );
   }
   bdestroy( buffer );
   bstrListDestroy( tiles_list );
   return;
}

static void datafile_tilemap_parse_objectgroup_ezxml( struct TILEMAP* t, ezxml_t xml_layer ) {
   ezxml_t xml_object = NULL;
   bstring buffer = NULL;
   const char* xml_attr = NULL;
   struct TILEMAP_POSITION* obj_out = NULL;
   int bstr_res = 0;

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
      bstr_res = bassigncstr( buffer, xml_attr );
      scaffold_check_nonzero( bstr_res );
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

void datafile_parse_tilemap_ezxml_t(
   struct TILEMAP* t, ezxml_t xml_data, BOOL local_images
) {
   ezxml_t xml_layer = NULL,
      xml_props = NULL,
      xml_tileset = NULL;

   SCAFFOLD_SIZE z = 0;

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
      datafile_tilemap_parse_layer_ezxml( t, xml_layer, z );
      xml_layer = ezxml_next( xml_layer );
      z++;
   }

   xml_layer = ezxml_child( xml_data, "objectgroup" );
   scaffold_check_null( xml_layer );
   while( NULL != xml_layer ) {
      datafile_tilemap_parse_objectgroup_ezxml( t, xml_layer );
      xml_layer = ezxml_next( xml_layer );
   }

#ifdef DEBUG
   t->sentinal = TILEMAP_SENTINAL;
#endif /* DEBUG */

cleanup:
   return;
}

void datafile_parse_tilemap_ezxml_string(
   struct TILEMAP* t, BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images
) {
   ezxml_t xml_data = NULL;
#ifdef EZXML_STRICT
   SCAFFOLD_SIZE datasize_check = 0;
#endif /* EZXML_STRICT */

   scaffold_check_null( tmdata );

#ifdef EZXML_STRICT
   datasize_check = strlen( (const char*)o );
   scaffold_assert( datasize_check == datasize );
#endif /* EZXML_STRICT */

   xml_data = ezxml_parse_str( (char*)tmdata, datasize );
   scaffold_check_null( xml_data );

   datafile_parse_tilemap_ezxml_t( t, xml_data, local_images );

cleanup:
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
   return;
}

void datafile_reserialize_tilemap( struct TILEMAP* t ) {
}
