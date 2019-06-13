
#define DATAFILE_C
#include "datafile.h"

#include "callback.h"
#include "proto.h"
#include "channel.h"


static void* callback_parse_mobs( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
#ifdef USE_EZXML
   ezxml_t xml_data = (ezxml_t)arg;
   const char* mob_id_test = NULL;

   /* Since the vector index is set by serial, there will be a number of      *
    * NULLs before we find one that isn't.                                    */
   if( NULL == iter ) {
      goto cleanup;
   }

   mob_id_test = ezxml_attr( xml_data, "id" );
   lgc_null( mob_id_test );

   if( 1 == biseqcstrcaseless( mobile_get_id( o ), mob_id_test ) ) {
      lg_debug(
         __FILE__, "Client: Found mobile with ID: %b\n", mobile_get_id( o )
      );
      datafile_parse_mobile_ezxml_t( o, xml_data, NULL, true );

      return o;
   }


#endif /* USE_EZXML */
cleanup:
   return NULL;
}

/*
static bool cb_datafile_prune_spriteless_mobs( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct MOBILE* o_player = NULL;
   struct CLIENT* local_client = (struct CLIENT*)arg;

   o_player = client_get_puppet( local_client );

   if(
      o_player != o &&
      NULL != o &&
      NULL == mobile_get_sprites_filename( o )
   ) {
      lg_debug( __FILE__, "Removing spriteless mobile: %b (%d)\n", o->display_name, o->serial );
      return true;
   }
   return false;
}
*/

static void* callback_parse_mob_channels( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
#ifdef USE_EZXML
   ezxml_t xml_data = (ezxml_t)arg;

   /* TODO: Return a condensed vector, somehow? */

   vector_iterate( l->mobiles, callback_parse_mobs, xml_data );
   //vector_remove_cb( l->mobiles, cb_datafile_prune_spriteless_mobs, l->client_or_server );

#endif /* USE_EZXML */
   return NULL;
}

void datafile_handle_stream(
   DATAFILE_TYPE type, const bstring filename, BYTE* data, size_t length,
   struct CLIENT* c
) {
   struct CHANNEL* l = NULL;
   struct MOBILE* o = NULL;
#ifdef USE_ITEMS
   struct ITEM_SPRITESHEET* catalog = NULL;
#endif // USE_ITEMS
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
      assert( NULL != l );

      graphics_surface_new( g, 0, 0, 0, 0 );
      lgc_null( g );
      graphics_set_image_data( g, data, length );
      lgc_null_msg( g->surface, "Unable to load tileset image." );

      set = vector_iterate(
         l->tilemap->tilesets, callback_search_tilesets_img_name, filename
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
         g = client_get_sprite( c, mobile_get_sprites_filename( o ) );
         if( NULL == g ) {
            client_request_file_later(
               c, DATAFILE_TYPE_MOBILE_SPRITES,
               mobile_get_sprites_filename( o ) );
         } else {
            mobile_set_sprites( o, g );
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
         catalog, data, length, true, DATAFILE_TYPE_ITEM_CATALOG,
         filename
      );
      if( hashmap_put( &(c->item_catalogs), filename, catalog, false ) ) {
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
