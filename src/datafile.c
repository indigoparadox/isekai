
#define DATAFILE_C
#include "datafile.h"

#include "callback.h"
#include "proto.h"
#include "channel.h"

void datafile_handle_stream(
   DATAFILE_TYPE type, bstring filename, BYTE* data, SCAFFOLD_SIZE length,
   struct CLIENT* c
) {
   struct CHANNEL* l = NULL;
   struct MOBILE* o = NULL;
   struct ITEM_SPRITESHEET* catalog = NULL;
   struct TILEMAP_TILESET* set = NULL;
   bstring lname = NULL,
      mob_id = NULL;
   GRAPHICS* g = NULL;
#ifdef USE_EZXML
   ezxml_t xml_data = NULL;
#endif /* USE_EZXML */

   switch( type ) {
   case DATAFILE_TYPE_TILESET:

      /* TODO: Fetch this tileset from other tilemaps, too. */
      set = hashmap_get( &(c->tilesets), filename );
      scaffold_assert( NULL != set );
      datafile_parse_ezxml_string(
         set, data, length, TRUE, DATAFILE_TYPE_TILESET, filename
      );
      c->tilesets_loaded++;
      break;

   case DATAFILE_TYPE_TILEMAP:
      lname = bfromcstr( "" );
#ifdef USE_EZXML
      xml_data = datafile_tilemap_ezxml_peek_lname(
         data, length, lname
      );
#endif /* USE_EZXML */

      l = client_get_channel_by_name( c, lname );
      scaffold_check_null_msg(
         l, "Unable to find channel to attach loaded tileset."
      );

#ifdef USE_EZXML
      scaffold_assert( TILEMAP_SENTINAL != l->tilemap.sentinal );
      datafile_parse_tilemap_ezxml_t(
         &(l->tilemap), xml_data, filename, TRUE
      );
      scaffold_assert( TILEMAP_SENTINAL == l->tilemap.sentinal );

      /* Download missing tilesets. */
      hashmap_iterate( &(c->tilesets), callback_download_tileset, c );

#endif /* USE_EZXML */

      /* Go through the parsed tilemap and load graphics. */
      proto_client_request_mobs( c, l );

      scaffold_print_debug(
         &module,
         "Client: Tilemap for %s successfully attached to channel.\n",
         bdata( l->name )
      );
      break;

   case DATAFILE_TYPE_TILESET_TILES:

      l = hashmap_iterate(
         &(c->channels),
         callback_search_channels_tilemap_img_name,
         filename
      );
      scaffold_assert( NULL != l );

      graphics_surface_new( g, 0, 0, 0, 0 );
      scaffold_check_null( g );
      graphics_set_image_data( g, data, length );
      scaffold_check_null_msg( g->surface, "Unable to load tileset image." );

      set = vector_iterate(
         &(l->tilemap.tilesets), callback_search_tilesets_img_name, filename
      );
      scaffold_check_null( set );

      hashmap_put( &(set->images), filename, g );
      scaffold_print_debug(
         &module,
         "Client: Tilemap image %s successfully loaded into tileset cache.\n",
         bdata( filename )
      );

#ifdef USE_CHUNKS
      proto_client_request_mobs( c, l );
#endif /* USE_CHUNKS */
      tilemap_set_redraw_state( &(l->tilemap), TILEMAP_REDRAW_ALL );
      break;

   case DATAFILE_TYPE_MOBILE:
      mob_id = bfromcstr( "" );

#ifdef USE_EZXML
      xml_data = datafile_mobile_ezxml_peek_mob_id( data, length, mob_id );
      scaffold_check_null( xml_data );

      o = hashmap_iterate(
         &(c->channels), callback_parse_mob_channels, xml_data
      );
      if( NULL != o ) {
         /* TODO: Make sure we never create receiving chunkers server-side (not strictly relevant here, but still). */
         g = hashmap_get( &(c->sprites), o->sprites_filename );
         if( NULL == g ) {
            client_request_file_later( c, DATAFILE_TYPE_MOBILE_SPRITES, o->sprites_filename );
         } else {
            o->sprites = g;
         }

         scaffold_print_debug(
            &module,
            "Client: Mobile def for %s successfully attached to channel.\n",
            bdata( mob_id )
         );
      }

#endif /* USE_EZXML */
      break;

   case DATAFILE_TYPE_ITEM_CATALOG_SPRITES: /* Fall through. */
   case DATAFILE_TYPE_MOBILE_SPRITES:
      graphics_surface_new( g, 0, 0, 0, 0 );
      scaffold_check_null_msg( g, "Unable to interpret spritesheet image." );
      graphics_set_image_data( g, data, length );
      scaffold_check_null( g->surface );
      hashmap_put( &(c->sprites), filename, g );
      hashmap_iterate( &(c->channels), callback_attach_channel_mob_sprites, c );
      scaffold_print_debug(
         &module,
         "Client: Spritesheet %b successfully loaded into cache.\n",
         filename
      );
      break;

   case DATAFILE_TYPE_ITEM_CATALOG:

      item_spritesheet_new( catalog, filename, c );

#ifdef USE_EZXML
      datafile_parse_ezxml_string(
         catalog, data, length, TRUE, DATAFILE_TYPE_ITEM_CATALOG,
         filename
      );
      hashmap_put( &(c->item_catalogs), filename, catalog );

      client_request_file_later(
         c, DATAFILE_TYPE_ITEM_CATALOG_SPRITES,
         catalog->sprites_filename
      );
      goto cleanup;
#endif /* USE_EZXML */
      break;

   case DATAFILE_TYPE_MISC:
      scaffold_print_error( &module, "Invalid data type specified.\n" );
      break;
   }

cleanup:
   bdestroy( lname );
   bdestroy( mob_id );
#ifdef USE_EZXML
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
#endif /* USE_EZXML */
   return;
}
