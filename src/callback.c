
#define CALLBACKS_C
#include "callback.h"

#include "scaffold.h"

#include "client.h"
#include "server.h"
#include "proto.h"
#include "chunker.h"
#include "tilemap.h"
#include "ui.h"
#include "vm.h"
#include "backlog.h"
#ifdef USE_EZXML
#include "datafile.h"
#include "ezxml.h"
#endif /* USE_EZXML */
#include "rng.h"
#include "ipc.h"
#include "channel.h"
#include "files.h"

#ifdef DEBUG
extern struct UI* last_ui;
#endif /* DEBUG */

extern struct CLIENT* main_client;

/* Append all clients to the bstrlist arg. */
void* callback_concat_clients( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct VECTOR* list = (struct VECTOR*)arg;
   vector_add( list, bstrcpy( client_get_nick( c ) ) );
   return NULL;
}

void* callback_concat_strings( size_t idx, void* iter, void* arg ) {
   bstring str_out = (bstring)arg;
   bstring str_cat = (bstring)iter;

   bconcat( str_out, str_cat );

   return NULL;
}

/* Return all clients EXCEPT the client arg. */
void* callback_search_clients_r( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( NULL == nick || 0 != bstrcmp( nick, client_get_nick( c ) ) ) {
      return c;
   }
   return NULL;
}

void* callback_search_bstring_i( size_t idx, void* iter, void* arg ) {
   bstring str = (bstring)iter;
   bstring compare = (bstring)arg;

   if( 0 == bstricmp( str, compare ) ) {
      return iter;
   }

   return NULL;
}

/** \brief If the iterated spawner is of the ID specified in arg, then return
 *         it. Otherwise, return NULL.
 */
void* callback_search_spawners( size_t idx, void* iter, void* arg ) {
   bstring spawner_id = (bstring)arg;
   struct TILEMAP_SPAWNER* spawner = (struct TILEMAP_SPAWNER*)iter;

   if( NULL == arg || 0 == bstrcmp( spawner->id, spawner_id ) ) {
      return spawner;
   }

   return NULL;
}

#ifdef USE_ITEMS

void* callback_search_item_caches( size_t idx, void* iter, void* arg ) {
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)arg;
   struct TILEMAP_ITEM_CACHE* cache = (struct TILEMAP_ITEM_CACHE*)iter;

   if(
      NULL == arg || (
         pos->x == cache->position.x &&
         pos->y == cache->position.y
      )
   ) {
      return cache;
   }

   return NULL;
}

void* callback_search_items( size_t idx, void* iter, void* arg ) {
   struct ITEM* e_search = (struct ITEM*)arg;
   struct ITEM* e_iter = (struct ITEM*)iter;

   if( NULL == arg || e_search == e_iter ) {
      return e_iter;
   }

   return NULL;
}

#endif // USE_ITEMS

/*
void* callback_search_channels( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring name = (bstring)arg;
   if( 0 == bstrcmp( l->name, name ) ) {
      return l;
   }
   return NULL;
}
*/

void* callback_get_tile_stack_l( size_t idx, void* iter, void* arg ) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)arg;
   struct TILEMAP* t = layer->tilemap;
   struct TILEMAP_TILESET* set = NULL;
   uint32_t gid = 0;
   struct TILEMAP_TILE_DATA* tdata = NULL;

   assert( TILEMAP_SENTINAL == t->sentinal );

   gid = tilemap_layer_get_tile_gid( layer, pos->x, pos->y );
   set = tilemap_get_tileset( t, gid, NULL );
   if( NULL != set ) {
      tdata = tilemap_tileset_get_tile( set, gid - 1 );
   }

/* cleanup: */
   if( NULL == tdata ) {
#ifdef DEBUG_TILES
      /* lg_debug(
         "Unable to get tileset for: %d, %d\n", pos->x, pos->y
      ); */
#endif /* DEBUG_TILES */
   }
   return tdata;
}

/**
 * \brief Add any tilesets that have not yet been downloaded to the list of
 *        files to be requested later (so as not to violate iterator lock).
 */
void* callback_download_tileset( bstring idx, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   if(
      0 == tilemap_tileset_get_tile_height( set ) &&
      0 == tilemap_tileset_get_tile_width( set )
   ) {
      client_request_file_later( c, DATAFILE_TYPE_TILESET, idx );
   }

   return NULL;
}

/** \brief Same idea as callback_download_tileset(), but server-side.
 */
void* callback_load_local_tilesets( bstring idx, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   size_t bytes_read = 0,
      setdata_size = 0;
   BYTE* setdata_buffer = NULL;
   bstring setdata_path = NULL;

   lgc_null_msg( idx, "Invalid tileset key provided." );
   if(
      0 == tilemap_tileset_get_tile_height( set ) &&
      0 == tilemap_tileset_get_tile_width( set )
   ) {

      setdata_path = files_root( idx );

      lg_debug(
         __FILE__, "Loading tileset XML data from: %s\n",
         bdata( setdata_path )
      );
      bytes_read = files_read_contents(
         setdata_path, &setdata_buffer, &setdata_size );
      lgc_null_msg(
         setdata_buffer, "Unable to load tileset data." );
      lgc_zero_msg( bytes_read, "Unable to load tilemap data." );

      datafile_parse_ezxml_string(
         set, setdata_buffer, setdata_size, false,
         DATAFILE_TYPE_TILESET, idx
      );

      mem_free( setdata_buffer );
   }

cleanup:
   bdestroy( setdata_path );
   return NULL;
}

#ifdef USE_ITEMS

void* callback_load_spawner_catalogs(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_SPAWNER* spawner = (struct TILEMAP_SPAWNER*)iter;
   struct CLIENT* client_or_server = (struct CLIENT*)arg;
   bstring catdata_path = NULL;
   BYTE* catdata = NULL;
   size_t catdata_length = 0,
      bytes_read;
   struct ITEM_SPRITESHEET* catalog = NULL;

   if(
      TILEMAP_SPAWNER_TYPE_ITEM == spawner->type &&
      NULL == client_get_catalog( client_or_server, spawner->catalog )
   ) {
      lg_debug(
         __FILE__, "Loading item catalog: %b\n", spawner->catalog
      );

      lgc_null_msg( spawner->catalog, "Invalid catalog provided." );
      catdata_path = files_root( spawner->catalog );
      bytes_read = files_read_contents(
         catdata_path, &catdata, &catdata_length );
      lgc_null_msg(
         catdata, "Unable to load catalog data." );
      lgc_zero_msg( bytes_read, "Unable to load catalog data." );

      item_spritesheet_new( catalog, spawner->catalog, client_or_server );

#ifdef USE_EZXML

      datafile_parse_ezxml_string(
         catalog, catdata, catdata_length, false, DATAFILE_TYPE_ITEM_CATALOG,
         catdata_path
      );

#ifdef USE_ITEMS
      if( client_set_catalog( client_or_server, spawner->catalog, catalog ) ) {
         lg_error( __FILE__, "Attempted to double-add catalog: %b\n",
            spawner->catalog );
         item_spritesheet_free( catalog );
      }
#endif /* USE_ITEMS */

#endif /* USE_EZXML */

      /* TODO: Load catalog. */
   }

cleanup:
   bdestroy( catdata_path );
   mem_free( catdata );
   return NULL;
}

#endif /* USE_ITEMS */

void* callback_search_mobs_by_pos( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)arg;
   struct MOBILE* o_out = NULL;

   if( NULL != o && mobile_get_x( o ) == pos->x && mobile_get_y( o ) == pos->y ) {
      o_out = o;
   }

   return o_out;
}

#ifdef ENABLE_LOCAL_CLIENT

/* Searches for a tileset containing the image named in bstring arg. */
/** \brief
 *
 * \param
 * \param
 * \return
 *
 */

void* callback_search_tilesets_img_name( size_t idx, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   if( tilemap_tileset_has_image( set, (bstring)arg ) ) {
      /* This is the tileset that contains this image. */
      return set;
   }
   return NULL;
}

/**
 * \brief  Searches for a channel with an attached tilemap containing a tileset
 *         containing the image named in bstring arg.
 *
 * \param
 * \param arg - A bstring containing the image filename.
 * \return The channel with the map containing the specified image.
 */
void* callback_search_channels_tilemap_img_name( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   struct TILEMAP* t = l->tilemap;
   if( NULL != vector_iterate( t->tilesets, callback_search_tilesets_img_name, arg ) ) {
      return l;
   }
   return NULL;
}

void* callback_search_graphics( bstring idx, void* iter, void* arg ) {
   GRAPHICS* g = (GRAPHICS*)iter;
   bstring s_key = (bstring)arg;

   if( 0 == bstrcmp( idx, s_key ) ) {
      if( NULL == g ) {
         /* This image hasn't been set yet, so return a blank.*/
         /* graphics_surface_new( g, 0, 0, 0, 0 ); */
         /* Just fudge this for now. */
         g = (void*)1;
      }
      return g;
   }
   return NULL;
}

void* callback_search_tilesets_name( bstring idx, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   bstring name = (bstring)arg;

   if( 0 == bstrcmp( idx, name ) ) {
      return set;
   }
   return NULL;
}

#ifdef USE_CHUNKS

void* callback_proc_client_chunkers( bstring idx, void* iter, void* arg ) {
   struct CHUNKER* h = (struct CHUNKER*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   if( chunker_unchunk_finished( h ) ) {
      /* Cached file found, so abort transfer. */
      if( chunker_unchunk_cached( h ) ) {
         proto_abort_chunker( c, h );
      }

      return h;
   }

   return NULL;
}

#endif /* USE_CHUNKS */

#endif /* ENABLE_LOCAL_CLIENT */

#ifdef USE_VM

void* callback_proc_mobile_vms( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;

   scaffold_assert_server();

   if(
      NULL != o &&
      !mobile_is_occupied( o ) &&
      vm_caddy_has_event( o->vm_caddy, &str_vm_tick ) &&
#ifdef USE_TURNS
      vm_get_tick( o->vm_tick_prev )
#else
      vm_get_tick( 0 )
#endif /* USE_TURNS */
   ) {
      vm_caddy_do_event( o->vm_caddy, &str_vm_tick );
#ifdef USE_TURNS
      o->vm_tick_prev++;
#endif /* USE_TURNS */
   }

   return NULL;
}

void* callback_proc_channel_vms( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;

   scaffold_assert_server();

   /*
   if( channel_vm_can_step( l ) ) {
      channel_vm_step( l );
   }
   */

   vector_iterate( l->mobiles, callback_proc_mobile_vms, NULL );

   return NULL;
}

#endif /* USE_VM */

void* callback_proc_channel_spawners(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_SPAWNER* ts = (struct TILEMAP_SPAWNER*)iter;
   struct TILEMAP* t = ts->tilemap;
   struct CHANNEL* l = NULL;
   struct MOBILE* o = NULL;
#ifdef USE_ITEMS
   struct ITEM* e = NULL;
   struct ITEM_SPRITESHEET* catalog = NULL;
   struct TILEMAP_ITEM_CACHE* cache = NULL;
#endif /* USE_ITEMS */

   l = tilemap_get_channel( t );
   assert( CHANNEL_SENTINAL == l->sentinal );

   if( -1 >= ts->countdown_remaining ) {
      /* Spawner has been disabled. */
      /* TODO: If the mob has been destroyed, restart the spawner countdown. */
      goto cleanup;
   }

   if( 0 < ts->countdown_remaining && NULL != ts->id ) {
      /* TODO: Decrement the countdown timer in seconds. */
      goto cleanup;
   }

   /* By process of elimination, we must be at zero, so perform a spawn! */
   ts->countdown_remaining = -1;
   switch( ts->type ) {
   case TILEMAP_SPAWNER_TYPE_PLAYER:
   case TILEMAP_SPAWNER_TYPE_MOBILE:
      lg_debug( __FILE__, "Spawning mobile: %b\n", ts->id );
      o = mobile_new( ts->id, ts->pos.x, ts->pos.y );
      mobile_load_local( o );
      mobile_gen_serial( o, l->mobiles );
      channel_add_mobile( l, o );
      if( TILEMAP_SPAWNER_TYPE_PLAYER == ts->type ) {
         mobile_set_type( o, MOBILE_TYPE_PLAYER );
#ifdef DEBUG_NO_CLIENT_SERVER_MODEL
         client_set_puppet( main_client, o );
#endif /* DEBUG_NO_CLIENT_SERVER_MODEL */
      } else {
         mobile_set_type( o, MOBILE_TYPE_GENERIC );
      }
      break;

#ifdef USE_ITEMS
   case TILEMAP_SPAWNER_TYPE_ITEM:
      lg_debug(
         __FILE__, "Spawning item: %b, Catalog: %b\n", ts->id, ts->catalog
      );
      catalog = server_get_catalog( s, ts->catalog );
      /* Catalog should be loaded by tilemap datafile loader. */
      lgc_null( catalog );
      while( NULL == e || 0 == e->sprite_id ) {
         item_random_new(
            e, item_type_from_c( bdata( ts->id ) ), ts->catalog,
            client_get_spritesheet( s ), server_get_unique_items( s )
         );
      }
      assert( 0 != e->sprite_id );
      assert( 0 < blength( e->catalog_name ) );
      cache = tilemap_drop_item( l->tilemap, e, ts->pos.x, ts->pos.y );
      assert( NULL != cache );
      /* TODO: Only send item updates to those nearby. */
      if( NULL != cache ) {
         proto_send_tile_cache_channel( l, cache );
      }
      e = NULL;
      break;
#endif /* USE_ITEMS */
   }

   /* TODO: We didn't get shunted to cleanup, so reset the timer if needed. */

cleanup:
   return NULL;
}

void* callback_proc_server_spawners( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   struct SERVER* s = (struct SERVER*)arg;

   scaffold_assert_server();

   vector_iterate( l->tilemap->spawners, callback_proc_channel_spawners, s );

   return NULL;
}

#ifdef USE_ITEMS

void* callback_search_item_type(
   size_t idx, void* iter, void* arg
) {
   ITEM_TYPE type = *((ITEM_TYPE*)arg);
   struct ITEM_SPRITE* sprite = (struct ITEM_SPRITE*)iter;

   if( NULL != sprite && type == sprite->type ) {
      return sprite;
   }

   return NULL;
}

#endif /* USE_ITEMS */

#ifdef ENABLE_LOCAL_CLIENT

void* callback_search_tilesets_small(
   size_t idx, void* iter, void* arg
) {
   struct TILEMAP_POSITION* temp = (struct TILEMAP_POSITION*)arg;
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;

   if( NULL != iter && temp->x < tilemap_tileset_get_tile_width( set ) ) {
      temp->x = tilemap_tileset_get_tile_width( set );
   }

   if( NULL != iter && temp->y < tilemap_tileset_get_tile_height( set ) ) {
      temp->y = tilemap_tileset_get_tile_height( set );
   }

   return NULL;
}

void* callback_proc_tileset_img_gs( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;

   lgc_not_null( iter );

   client_request_file( c, DATAFILE_TYPE_TILESET_TILES, idx );

cleanup:
   return NULL;
}

#endif /* ENABLE_LOCAL_CLIENT */

void* callback_send_mobs_to_client( size_t idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct MOBILE* o = (struct MOBILE*)iter;

   if( NULL == c || NULL == o ) {
      return NULL;
   }

   proto_send_mob( c, o );

   return NULL;
}

void* callback_send_mobs_to_channel( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)arg;
   struct CLIENT* c = (struct CLIENT*)iter;

   vector_iterate( l->mobiles, callback_send_mobs_to_client, c );

   return NULL;
}

void* callback_send_updates_to_client( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct ACTION_PACKET* update = (struct ACTION_PACKET*)arg;

   if( NULL == c || NULL == action_packet_get_mobile( update ) ) {
      return NULL;
   }

   proto_server_send_update( c, update );

   return NULL;
}

void* callback_attach_mob_sprites( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;
   struct GRAPHICS* g = NULL;

   /* Since the vector index is set by serial, there will be a number of      *
    * NULLs before we find one that isn't.                                    */
   if(
      NULL == o ||
      NULL != mobile_get_sprites( o ) ||
      NULL == mobile_get_sprites_filename( o )
   ) {
      goto cleanup;
   }

   g = client_get_sprite( c, mobile_get_sprites_filename( o ) );
   lgc_null( g );
   mobile_set_sprites( o, g );

cleanup:
   return NULL;
}

void* callback_attach_channel_mob_sprites(
   bstring idx, void* iter, void* arg
) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   return vector_iterate( l->mobiles, callback_attach_mob_sprites, c );
}


void* callback_stop_clients( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( NULL == arg || 0 == bstrcmp( nick, client_get_nick( c ) ) ) {
      lg_debug( __FILE__, "Stopping client: %p\n", c );
      client_stop( c );
      return NULL;
   }
   return NULL;
}

bool callback_v_free_clients( size_t idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;

   /* Since this is a refdec, it should be called anywhere clients are added
    * to a list and then the list is disposed of.
    */

   if( NULL == arg || 0 == bstrcmp( nick, client_get_nick( c ) ) ) {
#ifdef DEBUG_VERBOSE
      lg_debug( __FILE__, "Freeing client: %p\n", c );
#endif /* DEBUG_VERBOSE */
      /* This is just a refdec. */
      client_free( c );
      return true;
   }
   return false;
}

bool callback_h_free_clients( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;

   /* Since this is a refdec, it should be called anywhere clients are added
    * to a list and then the list is disposed of.
    */

   if( NULL == arg || 0 == bstrcmp( nick, client_get_nick( c ) ) ) {
#ifdef DEBUG_VERBOSE
      lg_debug( __FILE__, "Freeing client: %p\n", c );
#endif /* DEBUG_VERBOSE */
      /* This is just a refdec. */
      client_free( c );
      return true;
   }
   return false;
}

bool callback_free_channels( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring name = (bstring)arg;

   if( NULL == arg || 0 == bstrcmp( l->name, name ) ) {

      /* Free channel constituents, first. */
      /* if( HASHMAP_SENTINAL == l->clients.sentinal ) {
         hashmap_remove_cb( &(l->clients), callback_free_clients, NULL );
         hashmap_cleanup( &(l->clients) );
      } */
      /* if( vector_is_valid( l->mobiles ) ) {
         vector_remove_cb( l->mobiles, callback_free_mobiles, NULL );
         vector_cleanup( l->mobiles );
      } */

      channel_free( l );
      return true;
   }

   return false;
}

bool callback_free_empty_channels( bstring idx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;

   if( 0 >= hashmap_count( l->clients ) ) {
      channel_free( l );
      return true;
   }

   return false;
}

bool callback_free_mobiles( size_t idx, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   size_t* serial = (size_t*)arg;
   /*if( NULL == 0 ) {
      return true;
   }*/
   if( NULL == arg || *serial == mobile_get_serial( o ) ) {
      mobile_free( o );
      return true;
   }
   return false;
}

bool callback_free_tilesets( bstring idx, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   if( NULL == arg ) {
      tilemap_tileset_free( set );
      return true;
   }
   return false;
}

#ifdef USE_ITEMS

bool callback_free_sprites( size_t idx, void* iter, void* arg ) {
   struct ITEM_SPRITE* sprite = (struct ITEM_SPRITE*)iter;
   if( NULL == arg ) {
      item_sprite_free( sprite );
      return true;
   }
   return false;
}

bool callback_free_catalogs( bstring idx, void* iter, void* arg ) {
   struct ITEM_SPRITESHEET* cat = (struct ITEM_SPRITESHEET*)iter;
   if( NULL == arg ) {
      item_spritesheet_free( cat );
      return true;
   }
   return false;
}

bool callback_free_item_cache_items( size_t idx, void* iter, void* arg ) {
   struct ITEM* e = (struct ITEM*)iter;
   size_t serial = *((size_t*)arg);
   if( NULL == arg || serial == e->serial ) {
      item_free( e );
      return true;
   }
   return false;
}

bool callback_free_item_caches( size_t idx, void* iter, void* arg ) {
   struct TILEMAP_ITEM_CACHE* cache = (struct TILEMAP_ITEM_CACHE*)iter;
   if( NULL == arg ) {
      tilemap_item_cache_free( cache );
      return true;
   }
   return false;
}
#endif /* USE_ITEMS */

#ifdef USE_CHUNKS

bool callback_free_chunkers( bstring idx, void* iter, void* arg ) {
   bstring filename = (bstring)arg;
   if( NULL == filename || 0 == bstrcmp( idx, filename ) ) {
      struct CHUNKER* h = (struct CHUNKER*)iter;
      chunker_free( h );
      return true;
   }
   return false;
}

bool callback_free_finished_chunkers(
   bstring idx, void* iter, void* arg
) {
   struct CHUNKER* h = (struct CHUNKER*)iter;

   if( chunker_chunk_finished( h ) ) {
      lg_debug(
         __FILE__, "(Un)chunker for %b has finished. Removing...\n", idx );
      chunker_free( h );
      return true;
   }
   return false;
}

#endif /* USE_CHUNKS */

bool callback_free_generic( size_t idx, void* iter, void* arg ) {
   mem_free( iter );
   return true;
}

bool callback_v_free_strings( size_t idx, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( iter, (bstring)arg ) ) {
      bdestroy( (bstring)iter );
      return true;
   }
   return false;
}

bool callback_h_free_strings( bstring idx, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( iter, (bstring)arg ) ) {
      bdestroy( (bstring)iter );
      return true;
   }
   return false;
}

bool callback_free_backlog( size_t idx, void* iter, void* arg ) {
   struct BACKLOG_LINE* line = (struct BACKLOG_LINE*)iter;
   /* TODO: Implement retroactively deleting lines by ID or something. */
   if( NULL == arg ) {
      bdestroy( line->line );
      bdestroy( line->nick );
      mem_free( line );
      return true;
   }
   return false;
}

#ifdef ENABLE_LOCAL_CLIENT

bool callback_free_graphics( bstring idx, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( (bstring)iter, (bstring)arg ) ) {
      graphics_surface_free( (GRAPHICS*)iter );
      return true;
   }
   return false;
}

#ifdef DEBUG
void* callback_assert_windows( size_t idx, void* iter, void* arg ) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   assert( NULL != iter );
   assert( ui_get_local() == win->ui );
   return false;
}
#endif /* DEBUG */

#endif /* ENABLE_LOCAL_CLIENT */

bool callback_free_ani_defs( bstring idx, void* iter, void* arg ) {
   struct MOBILE_ANI_DEF* animation = (struct MOBILE_ANI_DEF*)iter;
   if( NULL == arg || 0 == bstrcmp( (bstring)arg, animation->name ) ) {
      mobile_animation_free( animation );
      return true;
   }
   return false;
}

bool callback_free_spawners( size_t idx, void* iter, void* arg ) {
   struct TILEMAP_SPAWNER* spawner = (struct TILEMAP_SPAWNER*)iter;
   if( NULL == arg || 0 == bstrcmp( (bstring)arg, spawner->id ) ) {
      bdestroy( spawner->id );
      return true;
   }
   return false;
}

VECTOR_SORT_ORDER callback_sort_chunker_tracks( void* a, void* b ) {
   CHUNKER_TRACK* cta = (CHUNKER_TRACK*)a;
   CHUNKER_TRACK* ctb = (CHUNKER_TRACK*)b;
   if( cta->start == ctb->start ) {
      return VECTOR_SORT_A_B_EQUAL;
   }
   return (cta->start > ctb->start) ? VECTOR_SORT_A_HEAVIER : VECTOR_SORT_A_LIGHTER;
 }
