
#define CHANNEL_C
#include "channel.h"

#include "ui.h"
#include "callback.h"
#include "tilemap.h"
#include "datafile.h"
#include "server.h"
#include "proto.h"
#include "backlog.h"
#ifdef USE_TINYPY
#include "tinypy/tinypy.h"
#endif /* USE_TINYPY */
#ifdef USE_DUKTAPE
#include "duktape/duktape.h"
#endif /* USE_DUKTAPE */
#include "rng.h"
#include "vm.h"
#include "ipc.h"
#include "files.h"

static void channel_free_final( const struct REF *ref ) {
   struct CHANNEL* l = scaffold_container_of( ref, struct CHANNEL, refcount );

   lg_debug(
      __FILE__,
      "Destroying channel: %b\n",
      l->name
   );

   #ifdef USE_VM
   vm_caddy_end( l->vm_caddy );
   #endif /* USE_VM */

   /* These need to be freed first! */
   if( hashmap_is_valid( l->clients ) ) {
      scaffold_assert( 0 == hashmap_count( l->clients ) );
      hashmap_free( &(l->clients) );
   }

   if( vector_is_valid( l->mobiles ) ) {
      vector_remove_cb( l->mobiles, callback_free_mobiles, NULL );
      scaffold_assert( 0 == vector_count( l->mobiles ) );
      vector_free( &(l->mobiles) );
   }

   bdestroy( l->name );
   bdestroy( l->topic );

   if( NULL != l->error ) {
      bdestroy( l->error );
      l->error = NULL;
   }

   tilemap_free( l->tilemap );
   l->tilemap = NULL;

   /* Free channel. */
   mem_free( l );
}

void channel_free( struct CHANNEL* l ) {
   /* FIXME: Things still holding open channel refs. */
   refcount_dec( l, "channel" );
}

/**
 * \param[in] server The server (or local client, if this is a local client
 *                   mirror of a server-side channel.)
 */
void channel_init(
   struct CHANNEL* l, const bstring name, BOOL local_images,
   struct CLIENT* server
) {
   ref_init( &(l->refcount), channel_free_final );
   hashmap_new( l->clients );
   vector_new( l->mobiles );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   l->sentinal = CHANNEL_SENTINAL;
   l->client_or_server = server;
   l->error = NULL;
   lgc_null( l->name );
   lgc_null( l->topic );
   tilemap_new( l->tilemap, local_images, server, l );
#ifdef USE_VM
   vm_caddy_new( l->vm_caddy );
#endif /* USE_VM */
cleanup:
   return;
}

void channel_add_client( struct CHANNEL* l, struct CLIENT* c, BOOL spawn ) {
   struct MOBILE* o = NULL;
   struct TILEMAP* t = NULL;
   struct TILEMAP_SPAWNER* spawner = NULL;
   struct VECTOR* player_spawns = NULL;

   lgc_null( c );

   if( NULL != channel_get_client_by_name( l, client_get_nick( c ) ) ) {
      goto cleanup;
   }

   if( FALSE != spawn ) {
      scaffold_assert_server();
      t = l->tilemap;

      /* Make a list of player-capable spawns and pick one at pseudo-random. */
      player_spawns = vector_iterate_v(
         &(t->spawners), callback_search_spawners, &str_player
      );
      lgc_null_msg( player_spawns, "No player spawns available." );
      spawner =
         vector_get( player_spawns, rand() % vector_count( player_spawns ) );
   }

   if( NULL != spawner ) {
      lg_debug(
         __FILE__, "Creating player mobile for client: %p\n", c
      );

      /* Create a basic mobile for the new client. */
      /* TODO: Get the desired mobile data ID from client. */
      mobile_new(
         o, (const bstring)&str_mobile_def_id_default,
         spawner->pos.x, spawner->pos.y
      );
      mobile_load_local( o );

      rng_gen_serial( o, l->mobiles, SERIAL_MIN, SERIAL_MAX );

      client_set_puppet( c, o );
      mobile_set_channel( o, l );
      channel_add_mobile( l, o );

      lg_debug(
         __FILE__,
         "Spawning %s (%d) at: %d, %d\n",
         bdata( client_get_nick( c ) ), o->serial, o->x, o->y
      );
   } else if( FALSE != spawn ) {
      lg_error(
         __FILE__,
         "Unable to find mobile spawner for this map.\n"
      );
   }

   client_add_channel( c, l  );

   if( hashmap_put( l->clients, client_get_nick( c ), c, FALSE ) ) {
      lg_error( __FILE__, "Attempted to double-add client.\n" );
      //client_free( c );
   }

   client_add_ref( c );

cleanup:
   if( NULL != player_spawns) {
      /* These don't have garbage refs, so just free the vector forcibly. */
      vector_free_force( &player_spawns );
   }
   return;
}

void channel_remove_client( struct CHANNEL* l, struct CLIENT* c ) {
   struct CLIENT* c_test = NULL;
   struct MOBILE* o = NULL;

   c_test = hashmap_get( l->clients, client_get_nick( c ) );
   if( NULL != c_test && FALSE != hashmap_remove( l->clients, client_get_nick( c ) ) ) {
      o = client_get_puppet( c );
      if( NULL != o ) {
         channel_remove_mobile( l, o->serial );
      }

      lg_debug(
         __FILE__,
         "Removed 1 clients from channel %s. %d clients, %d refs remaining.\n",
         bdata( l->name ), hashmap_count( l->clients ), l->refcount.count
      );
   }
}

struct CLIENT* channel_get_client_by_name(
   const struct CHANNEL* l, const bstring nick
) {
   return hashmap_get( l->clients, nick );
}

void channel_add_mobile( struct CHANNEL* l, struct MOBILE* o ) {
   mobile_set_channel( o, l );
   assert( 0 != o->serial );
   vector_set( l->mobiles, o->serial, o, TRUE );
}

void channel_set_mobile(
   struct CHANNEL* l, uint8_t serial, const bstring mob_id,
   const bstring def_filename, const bstring mob_nick,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
) {
   struct MOBILE* o = NULL;
   int bstr_res = 0;
   struct CLIENT* mobile_c = NULL;
   const char* nick_c = NULL,
      * lname_c = NULL;

   nick_c = bdata( mob_nick );
   lname_c = bdata( l->name );
   scaffold_assert( NULL != nick_c );
   scaffold_assert( NULL != lname_c );

   lg_debug(
      __FILE__, "Adding player mobile to channel: %b (%d)\n", mob_nick, serial
   );

   scaffold_assert( 0 != serial );

   o = vector_get( l->mobiles, serial );
   if( NULL == o ) {
      mobile_new( o, mob_id, x, y );
      o->serial = serial;
      lg_debug(
         __FILE__, "Player mobile does not exist. Creating with serial: %d\n",
         o->serial
      );
      scaffold_assert( NULL != o->def_filename );
      mobile_set_channel( o, l );
      vector_set( l->mobiles, o->serial, o, TRUE );
#ifdef ENABLE_LOCAL_CLIENT
      if( client_is_local( l->client_or_server ) ) {
         client_request_file(
            l->client_or_server, DATAFILE_TYPE_MOBILE, o->def_filename
         );
      }
#endif /* ENABLE_LOCAL_CLIENT */
   } else {
      lg_debug(
         __FILE__, "Resetting position for existing player mobile: %d, %d\n",
         x, y
      );
      o->x = x;
      o->y = y;
      o->prev_x = x;
      o->prev_y = y;
   }

   scaffold_assert( 0 < hashmap_count( l->clients ) );
   mobile_c = channel_get_client_by_name( l, mob_nick );
   if( NULL != mobile_c && 0 == bstrcmp( client_get_nick( mobile_c ), mob_nick ) ) {
      client_set_puppet( mobile_c, o );
   }

   bstr_res = bassign( o->display_name, mob_nick );
   scaffold_assert( NULL != o->display_name );
   lgc_nonzero( bstr_res );

   tilemap_set_redraw_state( l->tilemap, TILEMAP_REDRAW_ALL );

cleanup:
   return;
}

void channel_remove_mobile( struct CHANNEL* l, SCAFFOLD_SIZE serial ) {
   vector_remove( l->mobiles, serial );
}

void channel_load_tilemap( struct CHANNEL* l ) {
   bstring mapdata_filename = NULL,
      mapdata_path = NULL;
   BYTE* mapdata_buffer = NULL;
   int bstr_retval;
   size_t bytes_read = 0,
      mapdata_size = 0;

   lg_debug(
      __FILE__, "Loading tilemap for channel: %s\n", bdata( l->name )
   );
   mapdata_filename = bstrcpy( l->name );
   lgc_null( mapdata_filename );
   bdelete( mapdata_filename, 0, 1 ); /* Get rid of the # */

   mapdata_path = files_root( mapdata_filename );

   /* TODO: Different map filename extensions. */

#ifdef USE_EZXML

   bstr_retval = bcatcstr( mapdata_path, ".tmx" );
   lgc_nonzero( bstr_retval );

   lg_debug(
      __FILE__, "Loading tilemap XML data from: %s\n", bdata( mapdata_path ) );
   bytes_read = files_read_contents(
      mapdata_path, &mapdata_buffer, &mapdata_size );
   lgc_null_msg(
      mapdata_buffer, "Unable to load tilemap data." );
   lgc_zero_msg( bytes_read, "Unable to load tilemap data." );

   datafile_parse_ezxml_string(
      l->tilemap, mapdata_buffer, mapdata_size, FALSE,
      DATAFILE_TYPE_TILEMAP, mapdata_path
   );

#ifdef USE_ITEMS
   vector_iterate(
      &(l->tilemap->spawners), callback_load_spawner_catalogs,
      l->client_or_server
   );
#endif // USE_ITEMS

#endif /* USE_EZXML */

cleanup:
   bdestroy( mapdata_filename );
   bdestroy( mapdata_path );
   if( NULL != mapdata_buffer ) {
      mem_free( mapdata_buffer );
   }
   return;
}

BOOL channel_has_error( const struct CHANNEL* l ) {
   if( NULL != l && NULL != l->error ) {
      return TRUE;
   }
   return FALSE;
}

void channel_set_error( struct CHANNEL* l, const char* error ) {
   l->error = bfromcstr( error );
}

BOOL channel_is_loaded( struct CHANNEL* l ) {
   BOOL retval = FALSE;
   SCAFFOLD_SIZE tilesets_count;
   int i_test = 0; /* For debugging convenience. */
#ifdef DEBUG
   static SCAFFOLD_SIZE tilesets_loaded_last_pass = -1;
#endif /* DEBUG */

   if( NULL == l ) {
      goto cleanup;
   }

   if( NULL == l->client_or_server ) {
      goto cleanup;
   }

   if( NULL == client_get_puppet( l->client_or_server ) ) {
      goto cleanup;
   }

   if( 0 >= hashmap_count( l->clients ) ) {
      goto cleanup;
   }

   if( 0 >= vector_count( l->mobiles ) ) {
      goto cleanup;
   }

   if( !client_is_loaded( l->client_or_server ) ) {
      goto cleanup;
   }

   /* We made it this far... */
   retval = TRUE;

cleanup:
   return retval;
}

bstring channel_get_name( const struct CHANNEL* l ) {
   if( NULL == l ) {
      return NULL;
   }
   return l->name;
}

struct TILEMAP* channel_get_tilemap( const struct CHANNEL* l ) {
   if( NULL == l ) {
      return NULL;
   }
   return l->tilemap;
}
