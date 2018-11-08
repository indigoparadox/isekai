
#define DATAFILE_C
#include "datafile.h"

#include "callback.h"
#include "proto.h"
#include "channel.h"

void datafile_handle_stream(
   DATAFILE_TYPE type, const bstring filename, BYTE* data, size_t length,
   struct CLIENT* c
) {
   struct CHANNEL* l = NULL;
   struct MOBILE* o = NULL;
   struct ITEM_SPRITESHEET* catalog = NULL;
   struct TILEMAP_TILESET* set = NULL;
   bstring mob_id = NULL;
   GRAPHICS* g = NULL;
#ifdef USE_EZXML
   ezxml_t xml_data = NULL;
#endif /* USE_EZXML */

   switch( type ) {
   case DATAFILE_TYPE_TILESET:
      client_load_tileset_data( c, filename, data, length );
      break;

   case DATAFILE_TYPE_TILEMAP:
      client_load_tilemap_data( c, filename, data, length );
      break;

   case DATAFILE_TYPE_TILESET_TILES:

      l = client_iterate_channels( c, callback_search_channels_tilemap_img_name, filename );
      scaffold_assert( NULL != l );

      graphics_surface_new( g, 0, 0, 0, 0 );
      lgc_null( g );
      graphics_set_image_data( g, data, length );
      lgc_null_msg( g->surface, "Unable to load tileset image." );

      set = vector_iterate(
         &(l->tilemap->tilesets), callback_search_tilesets_img_name, filename
      );
      lgc_null( set );

      if( tilemap_tileset_set_image( set, filename, g ) ) {
         lg_error(
            __FILE__, "Attempted to double-add file: %b\n", filename );
         graphics_surface_free( g );
      } else {
         lg_debug(
            __FILE__,
            "Client: Tilemap image %s successfully loaded into tileset cache.\n",
            bdata( filename )
         );
      }

#ifdef USE_CHUNKS
      proto_client_request_mobs( c, l );
#endif /* USE_CHUNKS */
      tilemap_set_redraw_state( l->tilemap, TILEMAP_REDRAW_ALL );
      break;

   case DATAFILE_TYPE_MOBILE:
      mob_id = bfromcstr( "" );

#ifdef USE_EZXML
      xml_data = datafile_mobile_ezxml_peek_mob_id( data, length, mob_id );
      lgc_null( xml_data );

      client_iterate_channels( c, callback_parse_mob_channels, xml_data );
      if( NULL != o ) {
         /* TODO: Make sure we never create receiving chunkers server-side
          *       (not strictly relevant here, but still).
          */
         g = client_get_sprite( c, o->sprites_filename );
         if( NULL == g ) {
            client_request_file_later(
               c, DATAFILE_TYPE_MOBILE_SPRITES, o->sprites_filename );
         } else {
            o->sprites = g;
         }

         lg_debug(
            __FILE__,
            "Client: Mobile def for %s successfully attached to channel.\n",
            bdata( mob_id )
         );
      }

#endif /* USE_EZXML */
      break;

#ifdef USE_ITEMS
   case DATAFILE_TYPE_ITEM_CATALOG_SPRITES: /* Fall through. */
#endif /* USE_ITEMS */
   case DATAFILE_TYPE_MOBILE_SPRITES:
      graphics_surface_new( g, 0, 0, 0, 0 );
      lgc_null_msg( g, "Unable to interpret spritesheet image." );
      graphics_set_image_data( g, data, length );
      lgc_null( g->surface );
      if( !client_set_sprite( c, filename, g ) ) {
         graphics_surface_free( g );
      }
      break;

#ifdef USE_ITEMS
   case DATAFILE_TYPE_ITEM_CATALOG:

      item_spritesheet_new( catalog, filename, c );

#ifdef USE_EZXML
      datafile_parse_ezxml_string(
         catalog, data, length, TRUE, DATAFILE_TYPE_ITEM_CATALOG,
         filename
      );
      if( hashmap_put( &(c->item_catalogs), filename, catalog, FALSE ) ) {
         lg_error( __FILE__, "Attempted to double-add catalog: %b\n",
            filename );
         item_spritesheet_free( catalog );
      } else {
         client_request_file_later(
            c, DATAFILE_TYPE_ITEM_CATALOG_SPRITES,
            catalog->sprites_filename
         );
      }
#endif /* USE_EZXML */
      break;
#endif /* USE_ITEMS */

   case DATAFILE_TYPE_MISC:
      lg_error( __FILE__, "Invalid data type specified.\n" );
      break;

   case DATAFILE_TYPE_INVALID:
      break;
   }

cleanup:
   bdestroy( mob_id );
   //bdestroy( filename );
#ifdef USE_EZXML
   if( NULL != xml_data ) {
      ezxml_free( xml_data );
   }
#endif /* USE_EZXML */
   return;
}
