
#define DATAFILE_C
#include "../datafile.h"

#include "../vm.h"
#include "../callback.h"
#include "../channel.h"

#include <stdlib.h>

#define ezxml_node( target, parent, id ) \
   target = ezxml_child( parent, id ); \
   scaffold_check_null( target );

#define ezxml_int( target, string, parent, attr ) \
   string = ezxml_attr( parent, attr ); \
   scaffold_check_null( string ); \
   target = atoi( string );

#define ezxml_string( target, parent, attr ) \
   target = ezxml_attr( parent, attr ); \
   scaffold_check_null( target );

void datafile_parse_item_sprites_ezxml_t(
   struct ITEM_SPRITESHEET* spritesheet, ezxml_t xml_sprites,
   bstring def_path, BOOL local_images
) {
   ezxml_t xml_sprite = NULL;
   const char* xml_attr = NULL;
   struct ITEM_SPRITE* sprite = NULL;
   SCAFFOLD_SIZE sprite_id = 0;

   scaffold_check_null( xml_sprites );

   assert( NULL == spritesheet->sprites_filename );
   xml_attr = ezxml_attr( xml_sprites, "image" );
   scaffold_check_null( xml_attr );
   spritesheet->sprites_filename = bfromcstr( xml_attr );

   ezxml_int( spritesheet->spritewidth, xml_attr, xml_sprites, "spritewidth" );
   ezxml_int( spritesheet->spriteheight, xml_attr, xml_sprites, "spriteheight" );

   vector_init( &(spritesheet->sprites) );

   xml_sprite = ezxml_child( xml_sprites, "sprite" );
   while( NULL != xml_sprite ) {
      sprite = mem_alloc( 1, struct ITEM_SPRITE );

      ezxml_int( sprite_id, xml_attr, xml_sprite, "id" );

      ezxml_string( xml_attr, xml_sprite, "display" );
      sprite->display_name = bfromcstr( xml_attr );
      scaffold_check_null( sprite->display_name );

      /* TODO: Parse item type. */
      xml_attr = ezxml_attr( xml_sprite, "type" );
      scaffold_check_null_continue( xml_attr );
      sprite->type = item_type_from_c( xml_attr );

      scaffold_print_debug(
         &module, "Loading proto-item: %b\n", sprite->display_name
      );

      scaffold_assert( NULL == vector_get( &(spritesheet->sprites), sprite_id ) );
      vector_set( &(spritesheet->sprites), sprite_id, sprite, TRUE );
      sprite = NULL;

      xml_sprite = ezxml_next( xml_sprite );
   }

cleanup:
   return;
}

void datafile_parse_item_ezxml_t(
   struct ITEM* e, ezxml_t xml_data, bstring def_path, BOOL local_images
) {
}

/* = Mobiles = */

ezxml_t datafile_mobile_ezxml_peek_mob_id(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring mob_id_buffer
) {
   ezxml_t xml_data = NULL;
   const char* mob_id_c = NULL;
   int bstr_retval;

   scaffold_assert( NULL != mob_id_buffer );
   scaffold_check_null( tmdata );

   xml_data = ezxml_parse_str( (char*)tmdata, datasize );
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
   const char* xml_attr = NULL;
   struct MOBILE_SPRITE_DEF* sprite = NULL;

   scaffold_check_null( xml_sprite );

   sprite = mem_alloc( 1, struct MOBILE_SPRITE_DEF );

   /* TODO: Case insensitivity. */
   xml_attr = ezxml_attr( xml_sprite, "id" );
   scaffold_check_null( xml_attr );
   sprite->id = atoi( xml_attr );

   vector_set( &(o->sprite_defs), sprite->id, sprite, TRUE );
   sprite = NULL;

cleanup:
   if( NULL != sprite ) {
      mem_free( sprite );
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
   SCAFFOLD_SIZE_SIGNED verr;

   scaffold_check_null( xml_animation );

   name_dir = bfromcstralloc( 10, "" );
   scaffold_check_null( name_dir );

   animation = mem_alloc( 1, struct MOBILE_ANI_DEF );

   ezxml_string( xml_attr, xml_animation, "name" );
   scaffold_assign_or_cpy_c( animation->name, xml_attr, bstr_retval );
   ezxml_int( animation->speed, xml_attr, xml_animation, "speed" );

   vector_init( &(animation->frames) );
   xml_frame_iter = ezxml_child( xml_animation, "frame" );
   while( NULL != xml_frame_iter ) {
      ezxml_int( frame_id, xml_attr, xml_frame_iter, "id" );

      sprite = vector_get( &(o->sprite_defs), frame_id );
      if( NULL == sprite ) {
         scaffold_print_error(
            &module, "Bad frame in parsed mobile animation: %d\n", frame_id
         );
      }

      verr = vector_add( &(animation->frames), sprite );
      if( 0 > verr ) {
         scaffold_print_error(
            &module, "Unable to add frame to mobile animation: %d\n", frame_id
         );
      }

      xml_frame_iter = ezxml_next( xml_frame_iter );
   }

   /* TODO: Case insensitivity. */
   xml_attr = ezxml_attr( xml_animation, "facing" );
   scaffold_check_null( xml_attr );
   if( 0 == scaffold_strcmp_caseless( "down", xml_attr ) ) {
      animation->facing = MOBILE_FACING_DOWN;
   } else if( 0 == scaffold_strcmp_caseless( "up", xml_attr ) ) {
      animation->facing = MOBILE_FACING_UP;
   } else if( 0 == scaffold_strcmp_caseless( "left", xml_attr ) ) {
      animation->facing = MOBILE_FACING_LEFT;
   } else if( 0 == scaffold_strcmp_caseless( "right", xml_attr ) ) {
      animation->facing = MOBILE_FACING_RIGHT;
   } else {
      goto cleanup;
   }
   bstr_retval =
      bassignformat( name_dir, "%s-%s", bdata( animation->name ), xml_attr );
   scaffold_check_nonzero( bstr_retval );
   scaffold_print_debug(
      &module, "Loaded mobile animation: %b\n", name_dir
   );

   if( hashmap_put( &(o->ani_defs), name_dir, animation, FALSE ) ) {
      scaffold_print_error(
         &module, "Attempted to double-add mobile animation def.\n" );
      mobile_animation_free( animation );
   }
   animation = NULL;

cleanup:
   if( NULL != animation ) {
      mem_free( animation );
   }
   bdestroy( name_dir );
   return;
}

void datafile_parse_mobile_ezxml_t(
   struct MOBILE* o, ezxml_t xml_data, bstring def_path, BOOL local_images
) {
   ezxml_t xml_sprites = NULL,
      xml_sprite_iter = NULL,
      xml_animations = NULL,
      xml_animation_iter = NULL,
      xml_scripts = NULL,
      xml_script_iter = NULL,
      xml_image = NULL;
   const char* xml_attr = NULL;
   int bstr_retval;
   bstring walk_ani_key = NULL,
      vm_val_buffer = NULL,
      vm_key_buffer = NULL;

   scaffold_check_null( xml_data );

   vm_val_buffer = bfromcstr( "" );
   vm_key_buffer = bfromcstr( "" );

   ezxml_string( xml_attr, xml_data, "id" );
   scaffold_assign_or_cpy_c( o->mob_id, xml_attr, bstr_retval );
   ezxml_int( o->sprite_width, xml_attr, xml_data, "spritewidth" );
   ezxml_int( o->sprite_height, xml_attr, xml_data, "spriteheight" );

   ezxml_string( xml_attr, xml_data, "display" );
   scaffold_assign_or_cpy_c( o->display_name, xml_attr, bstr_retval );

   ezxml_node( xml_sprites, xml_data, "sprites" );
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

#ifdef USE_VM

   xml_scripts = ezxml_child( xml_data, "scripts" );

   /* Store script globals. */
   xml_script_iter = ezxml_child( xml_scripts, "global" );
   while( NULL != xml_script_iter ) {

      /* Prepare by grabbing the key/val to be stored. */
      bstr_retval = bassigncstr(
         vm_val_buffer, ezxml_attr( xml_script_iter, "value" )
      );
      if( BSTR_ERR == bstr_retval ) {
         scaffold_print_error(
            &module, "Error parsing script component value attribute."
         );
         goto next_global;
      }

      bstr_retval = bassigncstr(
         vm_key_buffer, ezxml_attr( xml_script_iter, "name" )
      );
      if( BSTR_ERR == bstr_retval ) {
         scaffold_print_error(
            &module, "Error parsing script component with value: %b",
            vm_val_buffer
         );
         goto next_global;
      }

      /* Load the global into the hashmap. */
      if(
         0 == scaffold_strcmp_caseless( "global", xml_script_iter->name )
      ) {
         vm_caddy_put(
            o->vm_caddy, VM_MEMBER_GLOBAL, vm_key_buffer, vm_val_buffer );
      }

next_global:
      xml_script_iter = ezxml_next( xml_script_iter );
   }

   xml_script_iter = ezxml_child( xml_scripts, "script" );
   while( NULL != xml_script_iter ) {

      /* Prepare by grabbing the key/val to be stored. */
      xml_attr = ezxml_txt( xml_script_iter );
      bstr_retval = bassigncstr( vm_val_buffer, xml_attr );
      if( BSTR_ERR == bstr_retval ) {
         scaffold_print_error(
            &module, "Error parsing script component value text."
         );
         goto next_script;
      }

      bstr_retval = bassigncstr(
         vm_key_buffer, ezxml_attr( xml_script_iter, "name" )
      );
      if( BSTR_ERR == bstr_retval ) {
         scaffold_print_error(
            &module, "Error parsing script component with value: %b",
            vm_val_buffer
         );
         goto next_script;
      }

#ifdef USE_DUKTAPE
      /* Load the script into the hashmap. */
      if(
         0 == scaffold_strcmp_caseless(
            "text/javascript", ezxml_attr( xml_script_iter, "type" )
         )
      ) {
         /* TODO: Make lang per-script, not per-object/mobile/caddy. */
         if( VM_LANG_NONE != o->vm_caddy->lang ) {
            /* For now, crash if we try to mix languages in a mobile. */
            scaffold_assert( VM_LANG_JS == o->vm_caddy->lang );
         }
         o->vm_caddy->lang = VM_LANG_JS;
         vm_caddy_put(
            o->vm_caddy, VM_MEMBER_SCRIPT, vm_key_buffer, vm_val_buffer );
      }
#endif /* USE_DUKTAPE */

next_script:
      xml_script_iter = ezxml_next( xml_script_iter );
   }

   if( 0 != vm_caddy_scripts_count( o->vm_caddy ) ) {
      mobile_vm_start( o );
   }
#endif /* USE_VM */

   ezxml_node( xml_image, xml_data, "image" );
   ezxml_string( xml_attr, xml_image, "src" );
   scaffold_assign_or_cpy_c( o->sprites_filename, xml_attr, bstr_retval );

   walk_ani_key = bformat(
      "%s-%s",
      str_mobile_default_ani.data,
      str_mobile_facing[MOBILE_FACING_DOWN].data
   );

   o->initialized = TRUE;
   o->sprite_display_height = o->sprite_height;
   o->facing = MOBILE_FACING_DOWN;
   scaffold_print_debug(
      &module, "Mobile animation defaulting to: %b\n", walk_ani_key
   );
   o->current_animation = (struct MOBILE_ANI_DEF*)hashmap_get(
      &(o->ani_defs), walk_ani_key
   );
   /* TODO: Don't die if this fails. */
   scaffold_assert( NULL != o->current_animation );

cleanup:
   bdestroy( walk_ani_key );
   bdestroy( vm_key_buffer );
   bdestroy( vm_val_buffer );
   return;
}

/* = Tilemaps */

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

   xml_data = ezxml_parse_str( (char*)tmdata, datasize );
   scaffold_check_null( xml_data );

   xml_props = ezxml_child( xml_data, "properties" );
   scaffold_check_null( xml_props );

   xml_prop_iter = ezxml_child( xml_props, "property" );

   while( NULL != xml_prop_iter ) {
      if(
         0 == scaffold_strcmp_caseless(
         ezxml_attr( xml_prop_iter, "name" ), "channel"
      ) ) {
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

#ifdef ENABLE_LOCAL_CLIENT

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

   if( !hashmap_is_valid( &(set->images) ) ) {
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
      &module, "Tilemap: Tileset filename adjusted to: %s\n", bdata( buffer )
   );
#endif /* USE_REQUESTED_GRAPHICS_EXT */

   /* The key with NULL means we need to load this image. */
   scaffold_print_debug(
      &module, "Adding tile image name %b to set %b...\n",
      buffer, set->def_path
   );
   if( hashmap_put( &(set->images), buffer, NULL, FALSE ) ) {
      scaffold_print_error( &module, "Attempted to add existing image: %b\n",
         buffer );
   }

cleanup:
   bdestroy( buffer );
   if( NULL != g_image ) {
      graphics_surface_free( g_image );
   }
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

static void datafile_tilemap_parse_tileset_ezxml_terrain(
   struct TILEMAP_TILESET* set, ezxml_t xml_terrain, SCAFFOLD_SIZE id
) {
   struct TILEMAP_TERRAIN_DATA* terrain_info = NULL;
   const char* xml_attr;
   ezxml_t xml_props = NULL,
      xml_prop_iter = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   terrain_info = mem_alloc( 1, struct TILEMAP_TERRAIN_DATA );
   scaffold_check_null( terrain_info );

   xml_attr = ezxml_attr( xml_terrain, "name" );
   terrain_info->name = bfromcstr( xml_attr );
   scaffold_check_null( terrain_info->name );

   xml_props = ezxml_child( xml_terrain, "properties" );
   scaffold_check_null( xml_props );

   xml_prop_iter = ezxml_child( xml_props, "property" );
   while( NULL != xml_prop_iter ) {
      xml_attr = ezxml_attr( xml_prop_iter, "name" );

      if( 0 == scaffold_strcmp_caseless( "movement", xml_attr ) ) {
         xml_attr = ezxml_attr( xml_prop_iter, "value" );
         if( NULL != xml_attr ) {
            terrain_info->movement = atoi( xml_attr );
         }
      } else if( 0 == scaffold_strcmp_caseless( "cutoff", xml_attr ) ) {
         xml_attr = ezxml_attr( xml_prop_iter, "value" );
         if( NULL != xml_attr ) {
            terrain_info->cutoff = atoi( xml_attr );
         }
      }

      xml_prop_iter = ezxml_next( xml_prop_iter );
   }

   scaffold_print_debug(
      &module, "Loaded terrain %d: %s: %d\n",
      id, bdata( terrain_info->name ), terrain_info->movement
   );

   xml_attr = ezxml_attr( xml_terrain, "tile" );
   scaffold_check_null( xml_attr );
   terrain_info->tile = atoi( xml_attr );

   terrain_info->id = id;

   verr = vector_add( &(set->terrain), terrain_info );
   if( 0 > verr ) {
      /* Check below will destroy leftover object. */
      goto cleanup;
   }
   terrain_info = NULL;

cleanup:
   /* TODO: Don't scrap the whole tileset for a bad tile or two. */
   if( NULL != terrain_info ) {
      bdestroy( terrain_info->name );
      mem_free( terrain_info );
   }
}

#define TERRAIN_ID_C_BUFFER_LENGTH 4

BOOL datafile_tilemap_parse_tileset_ezxml(
   struct TILEMAP_TILESET* set, ezxml_t xml_tileset, bstring def_path, BOOL local_images
) {
   ezxml_t
      xml_image = NULL,
      xml_tile = NULL,
      xml_terraintypes = NULL,
      xml_terrain = NULL;
   const char* xml_attr = NULL;
   struct TILEMAP_TILE_DATA* tile_info = NULL;
   int i = 0,
      terrain_id_c_i = 0;
   SCAFFOLD_SIZE terrain_id = 0;
   struct TILEMAP_TERRAIN_DATA* terrain_info = NULL;
   const char* terrain_c = NULL;
   char terrain_id_c[TERRAIN_ID_C_BUFFER_LENGTH + 1] = { 0 };
#ifdef DEBUG_TILES_VERBOSE
   SCAFFOLD_SIZE dbg_terrain_id[4];
   const char* dbg_terrain_name[4];
#endif /* DEBUG_TILES_VERBOSE */
   BOOL loaded_fully = FALSE;

   scaffold_error = 0;

   scaffold_check_null( xml_tileset );

   xml_attr = ezxml_attr( xml_tileset, "source" );
   if( NULL != xml_attr && NULL == def_path ) {
      def_path = bfromcstr( xml_attr );
      scaffold_print_debug(
         &module, "Loading external tileset: %s\n", xml_attr );
      scaffold_check_null( def_path );
   }

   /* Try to grab the image list early. If it's missing, just get out. */
   xml_image = ezxml_child( xml_tileset, "image" );
   scaffold_check_null( xml_image );

   ezxml_int( set->tilewidth, xml_attr, xml_tileset, "tilewidth" );
   ezxml_int( set->tileheight, xml_attr, xml_tileset, "tileheight" );

#ifdef ENABLE_LOCAL_CLIENT

   while( NULL != xml_image ) {
      datafile_tilemap_parse_tileset_ezxml_image( set, xml_image, local_images );
      scaffold_check_nonzero( scaffold_error ); /* Need an image! */
      xml_image = ezxml_next( xml_image );
   }

#endif /* ENABLE_LOCAL_CLIENT */

   /* We can live without terrain information. */
   loaded_fully = TRUE;

   /* Parse the terrain information. */
   xml_terraintypes = ezxml_child( xml_tileset, "terraintypes" );
   scaffold_check_null_msg(
      xml_terraintypes, "No terraintypes found for this tileset.\n" );
   xml_terrain = ezxml_child( xml_terraintypes, "terrain" );
   while( NULL != xml_terrain ) {

      datafile_tilemap_parse_tileset_ezxml_terrain(
         set, xml_terrain, terrain_id
      );

      xml_terrain = ezxml_next( xml_terrain );
      terrain_id++;
   }

   /* Parse the tile information. */

   if( !vector_is_valid( &(set->tiles) ) ) {
      vector_init( &(set->tiles) );
   }

   xml_tile = ezxml_child( xml_tileset, "tile" );
   scaffold_check_null( xml_tile );
   while( NULL != xml_tile ) {
      tile_info = mem_alloc( 1, struct TILEMAP_TILE_DATA );
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
         &module, "Loaded tile %d: %d (%s), %d (%s), %d (%s), %d (%s)\n",
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
   if( NULL != tile_info ) {
      /* TODO: Delete tile info. */
   }
   return loaded_fully;
}

static void datafile_tilemap_parse_layer_ezxml(
   struct TILEMAP* t, ezxml_t xml_layer, SCAFFOLD_SIZE layer_index
) {
   struct TILEMAP_LAYER* layer = NULL;
   ezxml_t xml_layer_data = NULL;
   bstring buffer = NULL;
   struct VECTOR* tiles_list = NULL;
   SCAFFOLD_SIZE i;
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
   tiles_list = bgsplit( buffer, ',' );
   scaffold_check_null( tiles_list );

   /* Convert each tile terrain ID string into an int16 and stow in array. */

   /* TODO: Lock the list while adding, somehow. */
   for( i = 0 ; vector_count( tiles_list ) > i ; i++ ) {
      xml_attr = bdata( (bstring)vector_get( tiles_list, i ) );
      scaffold_check_null( xml_attr );
      vector_add_scalar( &(layer->tiles), atoi( xml_attr ), TRUE );
   }

   layer->tilemap = t;
   layer->z = layer_index;

   if( NULL != ezxml_attr( xml_layer, "name" ) ) {
      layer->name = bfromcstr( ezxml_attr( xml_layer, "name" ) );
      scaffold_check_null( layer->name );
   } else {
      layer->name = bfromcstr( "" );
   }

   vector_set( &(t->layers), layer_index, layer, TRUE );

   /* The map is as large as the largest layer. */
   if( layer->width > t->width ) { t->width = layer->width; }
   if( layer->height > t->height ) { t->height = layer->height; }

cleanup:
   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      tilemap_layer_free( layer );
   }
   bdestroy( buffer );
   vector_remove_cb( tiles_list, callback_v_free_strings, NULL );
   vector_free( &tiles_list );
   return;
}

static void datafile_tilemap_parse_object_ezxml( struct TILEMAP* t, ezxml_t xml_object ) {
   const char* xml_attr = NULL;
   ezxml_t xml_prop_iter = NULL;
   struct TILEMAP_SPAWNER* obj_out = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   tilemap_spawner_new( obj_out, t, TILEMAP_SPAWNER_TYPE_MOBILE );

   xml_attr = ezxml_attr( xml_object, "x" );
   /* TODO: _continue versions of the abort macros.*/
   scaffold_check_null( xml_attr );
   obj_out->pos.x = atoi( xml_attr ) / TILEMAP_OBJECT_SPAWN_DIVISOR;

   xml_attr = ezxml_attr( xml_object, "y" );
   scaffold_check_null( xml_attr );
   obj_out->pos.y = atoi( xml_attr ) / TILEMAP_OBJECT_SPAWN_DIVISOR;

   /* Parse spawner properties. */
   xml_prop_iter = ezxml_child( xml_object, "properties" );
   if( NULL != xml_prop_iter ) {
      xml_prop_iter = ezxml_child( xml_prop_iter, "property" );
   }
   while( NULL != xml_prop_iter ) {

      xml_attr = ezxml_attr( xml_prop_iter, "name" );
      scaffold_check_null_continue( xml_attr );

      if( 0 == strcmp( xml_attr, "countdown" ) ) {
         xml_attr = ezxml_attr( xml_prop_iter, "value" );
         scaffold_check_null_continue( xml_attr );
         obj_out->respawn_countdown = atol( xml_attr );

      } else if( 0 == scaffold_strcmp_caseless( xml_attr, "active" ) ) {
         xml_attr = ezxml_attr( xml_prop_iter, "value" );
         scaffold_check_null_continue( xml_attr );
         if( 0 == scaffold_strcmp_caseless( xml_attr, "false" ) ) {
            obj_out->active = FALSE;
         }

      }

      xml_prop_iter = ezxml_next( xml_prop_iter );
   }

   xml_attr = ezxml_attr( xml_object, "name" );
   scaffold_check_null( xml_attr );
   obj_out->id = bfromcstr( xml_attr );
   scaffold_check_null( obj_out->id );

   xml_attr = ezxml_attr( xml_object, "spritesheet" );
   if( NULL != xml_attr ) {
      obj_out->catalog = bfromcstr( xml_attr );
      scaffold_check_null( obj_out->id );
   } else {
      obj_out->catalog = NULL;
   }

   /* Figure out the spawner type. */
   xml_attr = ezxml_attr( xml_object, "type" );
   scaffold_check_null_msg( xml_attr, "Invalid or no type specified." );
   if( 0 == scaffold_strcmp_caseless( xml_attr, "spawn_player" ) ) {
      obj_out->type = TILEMAP_SPAWNER_TYPE_PLAYER;
   } else if( 0 == scaffold_strcmp_caseless( xml_attr, "spawn_mobile" ) ) {
      obj_out->type = TILEMAP_SPAWNER_TYPE_MOBILE;
   } else if( 0 == scaffold_strcmp_caseless( xml_attr, "spawn_item_random" ) ) {
      scaffold_check_null_msg(
         obj_out->catalog, "Invalid or no catalog specified."
      );
      obj_out->type = TILEMAP_SPAWNER_TYPE_ITEM;
   } else {
      /* We don't know how to handle this yet. */
      scaffold_print_error(
         &module, "Unknown object at: %d, %d\n",
         obj_out->pos.x, obj_out->pos.y
      );
      goto cleanup;
   }

   scaffold_print_debug(
      &module, "Spawner for \"%b\" at: %d, %d\n",
      obj_out->id, obj_out->pos.x, obj_out->pos.y
   );

   verr = vector_add( &(t->spawners), obj_out );
   if( 0 > verr ) {
      /* Check below will destroy leftover object. */
      goto cleanup;
   }
   obj_out = NULL;

cleanup:
   if( NULL != obj_out ) {
      /* Something went wrong. */
      mem_free( obj_out );
   }
}

static void datafile_tilemap_parse_objectgroup_ezxml( struct TILEMAP* t, ezxml_t xml_layer ) {
   ezxml_t xml_object = NULL;
   bstring buffer = NULL;

   scaffold_check_null( xml_layer );

   buffer = bfromcstralloc( TILEMAP_NAME_ALLOC, "" );

   xml_object = ezxml_child( xml_layer, "object" );
   scaffold_check_null( xml_object );
   scaffold_print_debug( &module, "Loading object spawns...\n" );
   while( NULL != xml_object ) {
      datafile_tilemap_parse_object_ezxml( t, xml_object );
      xml_object = ezxml_next( xml_object );
   }

cleanup:
   bdestroy( buffer );
   return;
}

SCAFFOLD_SIZE datafile_parse_tilemap_ezxml_t(
   struct TILEMAP* t, ezxml_t xml_data, bstring def_path, BOOL local_images
) {
   ezxml_t xml_layer = NULL,
      xml_props = NULL,
      xml_tileset = NULL;
   const char* xml_attr = NULL;
   bstring tileset_id = NULL;
   int bstr_retval = 0;
   SCAFFOLD_SIZE firstgid = 0;
   struct TILEMAP_TILESET* set = NULL;
   struct CHANNEL* l = NULL;
   SCAFFOLD_SIZE tilesets_loaded_fully = 0;

   /* Any tilesets not loaded fully here are external and might be loaded
    * later.
    */

   SCAFFOLD_SIZE z = 0;
   tileset_id = bfromcstr( "" );

   l = tilemap_get_channel( t );
   scaffold_check_equal( l->sentinal, CHANNEL_SENTINAL );

   xml_attr = ezxml_attr( xml_data, "orientation" );
   if( 0 == strncmp( "orthogonal", xml_attr, 10 ) ) {
      t->orientation = TILEMAP_ORIENTATION_ORTHO;
   } else if( 0 == strncmp( "isometric", xml_attr, 9 ) ) {
      t->orientation = TILEMAP_ORIENTATION_ISO;
   } else {
      l->error = bfromcstr( "Unrecognized map orientation." );
      scaffold_print_error(
         &module, "Unrecognized map orientation: %s\n", xml_attr
      );
      goto cleanup;
   }

   xml_tileset = ezxml_child( xml_data, "tileset" );
   scaffold_check_null( xml_tileset );
   while( NULL != xml_tileset ) {
      bstr_retval = btrunc( tileset_id, 0 );
      scaffold_check_nonzero( bstr_retval );

      /* Try to grab the string that uniquely identifies this tileset. */
      xml_attr = ezxml_attr( xml_tileset, "source" );
      if( NULL == xml_attr ) {
         /* This must be an internal tileset. */
         xml_attr = ezxml_attr( xml_tileset, "name" );
         scaffold_check_null_continue( xml_attr ); /* Unusable? */
         scaffold_print_debug(
            &module, "Found internal tileset definition: %s\n", xml_attr
         );

         /* Prevent collisions in the server-wide tileset list by creating a
          * "namespace" for this particular tilemap. This could be defeated by
          * having two different tilemaps with the same name, but just don't do
          * that, OK?
          */
         bstr_retval = bassign( tileset_id, t->lname );
         scaffold_assert( 0 == bstr_retval );
         scaffold_check_nonzero( bstr_retval );
         bstr_retval = bconchar( tileset_id, ':' );
         scaffold_check_nonzero( bstr_retval );
      }
      bstr_retval = bcatcstr( tileset_id, xml_attr );
      scaffold_check_nonzero( bstr_retval );

      set = hashmap_get( &(l->client_or_server->tilesets), tileset_id );
      if( NULL == set ) {
         /* This layer uses a tileset not seen yet. */
         scaffold_print_debug(
            &module, "New tileset found server-wide: %b\n", tileset_id
         );
         tilemap_tileset_new( set, tileset_id );
         /* Don't have to check since the hashmap_get above is a check. */
         hashmap_put( &(l->client_or_server->tilesets), tileset_id, set, TRUE );

        if( datafile_tilemap_parse_tileset_ezxml(
            set, xml_tileset, tileset_id, local_images )
         ) {
            /* This tileset was loaded enough to be considered done. */
            tilesets_loaded_fully++;
         }
      }

      ezxml_int( firstgid, xml_attr, xml_tileset, "firstgid" );
      vector_set( &(t->tilesets), firstgid, set, TRUE );
      scaffold_print_debug(
         &module, "Tileset %b assigned to tilemap %b with first GID: %d\n",
         tileset_id, t->lname, firstgid
      );

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
   bdestroy( tileset_id );
   return tilesets_loaded_fully;
}

void datafile_parse_ezxml_string(
   void* object, BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images,
   DATAFILE_TYPE type, bstring def_path
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

   switch( type ) {
   case DATAFILE_TYPE_TILESET:
      datafile_tilemap_parse_tileset_ezxml( object, xml_data, def_path, local_images );
      break;
   case DATAFILE_TYPE_TILEMAP:
      datafile_parse_tilemap_ezxml_t( object, xml_data, def_path, local_images );
      break;
   case DATAFILE_TYPE_MOBILE:
      datafile_parse_mobile_ezxml_t( object, xml_data, def_path, local_images );
      break;
   case DATAFILE_TYPE_ITEM_CATALOG:
      datafile_parse_item_sprites_ezxml_t( object, xml_data, def_path, local_images );
      break;
      /*
   case DATAFILE_TYPE_ITEM:
      datafile_parse_item_ezxml_t( object, xml_data, def_path, local_images );
      break;
      */
   default:
      scaffold_print_error( &module, "Invalid data type specified.\n" );
      break;
   }

cleanup:
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
   return;
}

void datafile_reserialize_tilemap( struct TILEMAP* t ) {
}
