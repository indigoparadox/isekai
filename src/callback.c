
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

#ifdef DEBUG
extern struct UI* last_ui;
#endif /* DEBUG */

extern struct CLIENT* main_client;

#if 0
void* callback_ingest_commands( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   SCAFFOLD_SIZE last_read_count = 0;
   struct SERVER* s = (struct SERVER*)arg;
   static bstring buffer = NULL;
   struct CLIENT* c = (struct CLIENT*)iter;
   IRC_COMMAND* cmd = NULL;
   const IRC_COMMAND* table = proto_table_server;
   int bstr_result;

#ifdef ENABLE_LOCAL_CLIENT
   /* Figure out if we're being called from a client or server. */
   if( FALSE != ipc_is_local_client( c->link ) ) {
      table = proto_table_client;
   } else {
#endif /* ENABLE_LOCAL_CLIENT */
   scaffold_assert( NULL != s );
#ifdef ENABLE_LOCAL_CLIENT
   }
#endif /* ENABLE_LOCAL_CLIENT */

   /* Make sure a buffer is present. */
   if( NULL == buffer ) {
      buffer = bfromcstralloc( IPC_BUFFER_LEN, "" );
   } else {
      bwriteallow( (*buffer) ); /* Unprotect the buffer. */
      bstr_result = btrunc( buffer, 0 );
      scaffold_check_nonzero( bstr_result );
   }

   /* Get the next line and clean it up. */
   last_read_count = ipc_read(
      c->link,
      buffer
   );

   /* Everything is fine, so tidy up the buffer. */
   btrimws( buffer );
   bwriteprotect( (*buffer) ); /* Protect the buffer until next read. */

   if( SCAFFOLD_ERROR_CONNECTION_CLOSED == scaffold_error ) {
      /* Return an empty command to force abortion of the iteration. */
      cmd = mem_alloc( 1, IRC_COMMAND );
      cmd->callback = NULL;
      cmd->line = bstrcpy( c->nick );
      goto cleanup;
   }

   /* The -1 is not bubbled up; the scaffold_error should suffice. */
   if( 0 >= last_read_count ) {
      goto cleanup;
   }

#ifdef DEBUG_NETWORK
   scaffold_print_debug(
      &module,
      "Server: Line received from %d: %s\n",
      c->link.socket, bdata( buffer )
   );
#endif /* DEBUG_NETWORK */

   cmd = irc_dispatch( table, s, c, buffer );

cleanup:
   return cmd;
}
#endif

/* Append all clients to the bstrlist arg. */
void* callback_concat_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct VECTOR* list = (struct VECTOR*)arg;
   vector_add( list, bstrcpy( c->nick ) );
   return NULL;
}

void* callback_concat_strings( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   bstring str_out = (bstring)arg;
   bstring str_cat = (bstring)iter;

   bconcat( str_out, str_cat );

   return NULL;
}

/* Return only the client arg if present. */
void* callback_search_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( NULL == arg || 0 == bstrcmp( nick, c->nick ) ) {
      return c;
   }
   return NULL;
}

/* Return all clients EXCEPT the client arg. */
void* callback_search_clients_r( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( NULL == nick || 0 != bstrcmp( nick, c->nick ) ) {
      return c;
   }
   return NULL;
}

void* callback_search_bstring_i(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   bstring str = (bstring)iter;
   bstring compare = (bstring)arg;

   if( 0 == bstricmp( str, compare ) ) {
      return iter;
   }

   return NULL;
}

/* Return any client that is in the bstrlist arg. */
void* callback_search_clients_l( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct VECTOR* list = (struct VECTOR*)arg;
   struct CLIENT* c = (struct CLIENT*)iter;
   return vector_iterate( list, callback_search_bstring_i, c->nick );
}

/** \brief If the iterated spawner is of the ID specified in arg, then return
 *         it. Otherwise, return NULL.
 */
void* callback_search_spawners( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   bstring spawner_id = (bstring)arg;
   struct TILEMAP_SPAWNER* spawner = (struct TILEMAP_SPAWNER*)iter;

   if( NULL == arg || 0 == bstrcmp( spawner->id, spawner_id ) ) {
      return spawner;
   }

   return NULL;
}

void* callback_search_item_caches(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
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

void* callback_search_items(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct ITEM* e_search = (struct ITEM*)arg;
   struct ITEM* e_iter = (struct ITEM*)iter;

   if( NULL == arg || e_search == e_iter ) {
      return e_iter;
   }

   return NULL;
}

void* callback_search_channels( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring name = (bstring)arg;
   if( 0 == bstrcmp( l->name, name ) ) {
      return l;
   }
   return NULL;
}

void* callback_get_tile_stack_l( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)arg;
   struct TILEMAP* t = layer->tilemap;
   struct TILEMAP_TILESET* set = NULL;
   uint32_t gid = 0;
   struct TILEMAP_TILE_DATA* tdata = NULL;

   scaffold_assert( TILEMAP_SENTINAL == t->sentinal );

   gid = tilemap_get_tile( layer, pos->x, pos->y );
   set = tilemap_get_tileset( t, gid, NULL );
   if( NULL != set ) {
      tdata = vector_get( &(set->tiles), gid - 1 );
   }

/* cleanup: */
   if( NULL == tdata ) {
#ifdef DEBUG_TILES
      /* scaffold_print_debug(
         "Unable to get tileset for: %d, %d\n", pos->x, pos->y
      ); */
#endif /* DEBUG_TILES */
   }
   return tdata;
}

/** \brief Try to download any tilesets that have not yet been downloaded.
 */
void* callback_download_tileset( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   scaffold_check_null_msg( idx->value.key, "Invalid tileset key provided." );
   if( 0 == set->tileheight && 0 == set->tilewidth ) {
      client_request_file_later( c, DATAFILE_TYPE_TILESET, idx->value.key );
   }

cleanup:
   return NULL;
}

/** \brief Same idea as callback_download_tileset(), but server-side.
 */
void* callback_load_local_tilesets( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   SCAFFOLD_SIZE bytes_read = 0,
      setdata_size = 0;
   BYTE* setdata_buffer = NULL;
   bstring setdata_path = NULL;

   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   scaffold_check_null_msg( idx->value.key, "Invalid tileset key provided." );
   if( 0 == set->tileheight && 0 == set->tilewidth ) {

      setdata_path = files_root( idx->value.key );

      scaffold_print_debug(
         &module, "Loading tileset XML data from: %s\n",
         bdata( setdata_path )
      );
      bytes_read = files_read_contents(
         setdata_path, &setdata_buffer, &setdata_size );
      scaffold_check_null_msg(
         setdata_buffer, "Unable to load tileset data." );
      scaffold_check_zero_msg( bytes_read, "Unable to load tilemap data." );

      datafile_parse_ezxml_string(
         set, setdata_buffer, setdata_size, FALSE,
         DATAFILE_TYPE_TILESET, idx->value.key
      );

      mem_free( setdata_buffer );
   }

cleanup:
   bdestroy( setdata_path );
   return NULL;
}

void* callback_load_spawner_catalogs(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_SPAWNER* spawner = (struct TILEMAP_SPAWNER*)iter;
   struct CLIENT* client_or_server = (struct CLIENT*)arg;
   bstring catdata_path = NULL;
   BYTE* catdata = NULL;
   SCAFFOLD_SIZE catdata_length = 0,
      bytes_read;
   struct ITEM_SPRITESHEET* catalog = NULL;

   if(
      TILEMAP_SPAWNER_TYPE_ITEM == spawner->type &&
      NULL == hashmap_get( &(client_or_server->item_catalogs), spawner->catalog )
   ) {
      scaffold_print_debug(
         &module, "Loading item catalog: %b\n", spawner->catalog
      );

      scaffold_check_null_msg( spawner->catalog, "Invalid catalog provided." );
      catdata_path = files_root( spawner->catalog );
      bytes_read = files_read_contents(
         catdata_path, &catdata, &catdata_length );
      scaffold_check_null_msg(
         catdata, "Unable to load catalog data." );
      scaffold_check_zero_msg( bytes_read, "Unable to load catalog data." );

      item_spritesheet_new( catalog, spawner->catalog, client_or_server );

#ifdef USE_EZXML

      datafile_parse_ezxml_string(
         catalog, catdata, catdata_length, FALSE, DATAFILE_TYPE_ITEM_CATALOG,
         catdata_path
      );

      if( hashmap_put(
         &(client_or_server->item_catalogs), spawner->catalog, catalog, FALSE
      ) ) {
         scaffold_print_error( &module, "Attempted to double-add catalog: %b\n",
            spawner->catalog );
         item_spritesheet_free( catalog );
      }

#endif /* USE_EZXML */

      /* TODO: Load catalog. */
   }

cleanup:
   bdestroy( catdata_path );
   mem_free( catdata );
   return NULL;
}

void* callback_search_mobs_by_pos( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)arg;
   struct MOBILE* o_out = NULL;

   if( NULL != o && o->x == pos->x && o->y == pos->y ) {
      o_out = o;
   }

   return o_out;
}

#ifdef ENABLE_LOCAL_CLIENT

void* callback_search_windows( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   bstring wid = (bstring)arg;
   if( 0 == bstrcmp( win->id, wid ) ) {
      return win;
   }
   return NULL;
}

/* Searches for a tileset containing the image named in bstring arg. */
/** \brief
 *
 * \param
 * \param
 * \return
 *
 */

void* callback_search_tilesets_img_name( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   if(
      NULL != set &&
      NULL != hashmap_iterate_nolock( &(set->images), callback_search_graphics, arg )
   ) {
      /* This is the tileset that contains this image. */
      return set;
   }
   return NULL;
}

/** \brief Searches for a channel with an attached tilemap containing a tileset
 *         containing the image named in bstring arg.
 *
 * \param
 * \param arg - A bstring containing the image filename.
 * \return The channel with the map containing the specified image.
 *
 */
void* callback_search_channels_tilemap_img_name( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   struct TILEMAP* t = l->tilemap;
   if( NULL != vector_iterate( &(t->tilesets), callback_search_tilesets_img_name, arg ) ) {
      return l;
   }
   return NULL;
}

void* callback_search_graphics( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   GRAPHICS* g = (GRAPHICS*)iter;
   bstring s_key = (bstring)arg;

   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   if( 0 == bstrcmp( idx->value.key, s_key ) ) {
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

void* callback_search_tilesets_name( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   bstring name = (bstring)arg;

   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   if( 0 == bstrcmp( idx->value.key, name ) ) {
      return set;
   }
   return NULL;
}

#ifdef USE_CHUNKS

void* callback_proc_client_chunkers(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
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

BOOL callback_proc_client_delayed_files(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CLIENT_DELAYED_REQUEST* req = (struct CLIENT_DELAYED_REQUEST*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   scaffold_print_debug(
      &module, "Processing delayed request: %b\n", req->filename
   );
   client_request_file( c, req->type, req->filename );

   bdestroy( req->filename );
   mem_free( req );
   return TRUE;
}

#endif /* ENABLE_LOCAL_CLIENT */

#ifdef USE_CHUNKS

void* callback_send_chunkers_l( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct CHUNKER* h = (struct CHUNKER*)iter;
   bstring chunk_out = NULL;
   SCAFFOLD_SIZE start_pos = 0;

   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   if( chunker_chunk_finished( h ) ) {
      goto cleanup;
   }

   chunk_out = bfromcstralloc( CHUNKER_DEFAULT_CHUNK_SIZE, "" );
   start_pos = chunker_chunk_pass( h, chunk_out );
   proto_send_chunk( c, h, start_pos, idx->value.key, chunk_out );

cleanup:
   bdestroy( chunk_out );
   return NULL;
}

void* callback_proc_chunkers( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;

   /* Process some compression chunks. */
   hashmap_iterate_v( &(c->chunkers), callback_send_chunkers_l, c );

   /* Removed any finished chunkers. */
   hashmap_remove_cb(
      &(c->chunkers), callback_free_finished_chunkers, NULL
   );

   return NULL;
}

#endif /* USE_CHUNKS */

#ifdef USE_VM

void* callback_proc_mobile_vms( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
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

void* callback_proc_channel_vms( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
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
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_SPAWNER* ts = (struct TILEMAP_SPAWNER*)iter;
   struct SERVER* s = (struct SERVER*)arg;
   struct TILEMAP* t = ts->tilemap;
   struct CHANNEL* l = NULL;
   struct MOBILE* o = NULL;
   struct ITEM* e = NULL;
   struct ITEM_SPRITESHEET* catalog = NULL;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   l = tilemap_get_channel( t );
   scaffold_assert( CHANNEL_SENTINAL == l->sentinal );

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
      scaffold_print_debug( &module, "Spawning mobile: %b\n", ts->id );
      mobile_new( o, ts->id, ts->pos.x, ts->pos.y );
      mobile_load_local( o );
      rng_gen_serial( o, l->mobiles, SERIAL_MIN, SERIAL_MAX );
      channel_add_mobile( l, o );
      if( TILEMAP_SPAWNER_TYPE_PLAYER == ts->type ) {
         o->type = MOBILE_TYPE_PLAYER;
#ifdef DEBUG_NO_CLIENT_SERVER_MODEL
         client_set_puppet( main_client, o );
#endif /* DEBUG_NO_CLIENT_SERVER_MODEL */
      } else {
         o->type = MOBILE_TYPE_GENERIC;
      }
      break;

   case TILEMAP_SPAWNER_TYPE_ITEM:
      scaffold_print_debug(
         &module, "Spawning item: %b, Catalog: %b\n", ts->id, ts->catalog
      );
      catalog = hashmap_get( &(s->self.item_catalogs), ts->catalog );
      /* Catalog should be loaded by tilemap datafile loader. */
      scaffold_check_null( catalog );
      while( NULL == e || 0 == e->sprite_id ) {
         item_random_new(
            e, item_type_from_c( bdata( ts->id ) ), ts->catalog, &(s->self)
         );
      }
      scaffold_assert( 0 != e->sprite_id );
      scaffold_assert( 0 < blength( e->catalog_name ) );
      cache = tilemap_drop_item( l->tilemap, e, ts->pos.x, ts->pos.y );
      scaffold_assert( NULL != cache );
      /* TODO: Only send item updates to those nearby. */
      if( NULL != cache ) {
         proto_send_tile_cache_channel( l, cache );
      }
      e = NULL;
      break;
   }

   /* TODO: We didn't get shunted to cleanup, so reset the timer if needed. */

cleanup:
   return NULL;
}

void* callback_proc_server_spawners(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   struct SERVER* s = (struct SERVER*)arg;

   scaffold_assert_server();

   vector_iterate( &(l->tilemap->spawners), callback_proc_channel_spawners, s );

   return NULL;
}

void* callback_search_item_type(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   ITEM_TYPE type = *((ITEM_TYPE*)arg);
   struct ITEM_SPRITE* sprite = (struct ITEM_SPRITE*)iter;

   if( NULL != sprite && type == sprite->type ) {
      return sprite;
   }

   return NULL;
}

void* callback_search_tilesets_gid( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   SCAFFOLD_SIZE* gid = (SCAFFOLD_SIZE*)arg;
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;

   scaffold_assert( CONTAINER_IDX_NUMBER == idx->type );

   if( NULL != set && idx->value.index <= *gid ) {
      *gid = idx->value.index;
      return set;
   }

   return NULL;
}

#ifdef ENABLE_LOCAL_CLIENT

void* callback_search_tilesets_small(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_POSITION* temp = (struct TILEMAP_POSITION*)arg;
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;

   if( NULL != iter && temp->x < set->tilewidth ) {
      temp->x = set->tilewidth;
   }

   if( NULL != iter && temp->y < set->tileheight ) {
      temp->y = set->tileheight;
   }

   return NULL;
}

void* callback_search_tileset_img_gid(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CLIENT* c = (struct CLIENT*)arg;

   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   if(
      NULL == iter
#ifdef USE_CHUNKS
      && NULL == hashmap_get( &(c->chunkers), idx->value.key )
#endif /* USE_CHUNKS */
   ) {
      client_request_file_later( c, DATAFILE_TYPE_TILESET_TILES, idx->value.key );
   }
   if( NULL != iter ) {
      return iter;
   }

   return NULL;

#if 0
   if( NULL == o->sprites && NULL == hashmap_get( &(twindow->c->sprites), o->sprites_filename ) ) {
      /* No sprites and no request yet, so make one! */
      client_request_file( twindow->c, CHUNKER_DATA_TYPE_TILESET_IMG, key );
      goto cleanup;
   } else if( NULL == o->sprites && NULL != hashmap_get( &(twindow->c->sprites), o->sprites_filename ) ) {
      o->sprites = (GRAPHICS*)hashmap_get( &(twindow->c->sprites), o->sprites_filename );
      refcount_inc( o->sprites, "spritesheet" );
   } else if( NULL == o->sprites ) {
      /* Sprites must not be ready yet. */
      goto cleanup;
   }
#endif
}

void* callback_proc_tileset_img_gs(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CLIENT* c = (struct CLIENT*)arg;

   scaffold_check_not_null( iter );
   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   client_request_file( c, DATAFILE_TYPE_TILESET_TILES, idx->value.key );

cleanup:
   return NULL;
}

void* callback_proc_tileset_imgs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;

   return hashmap_iterate( &(set->images), callback_proc_tileset_img_gs, c );
}

#endif /* ENABLE_LOCAL_CLIENT */

void* callback_send_mobs_to_client( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct MOBILE* o = (struct MOBILE*)iter;

   if( NULL == c || NULL == o ) {
      return NULL;
   }

   proto_send_mob( c, o );

   return NULL;
}

void* callback_send_mobs_to_channel( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)arg;
   struct CLIENT* c = (struct CLIENT*)iter;

   vector_iterate( l->mobiles, callback_send_mobs_to_client, c );

   return NULL;
}

void* callback_send_updates_to_client( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct MOBILE_UPDATE_PACKET* update = (struct MOBILE_UPDATE_PACKET*)arg;

   if( NULL == c || NULL == update->o ) {
      return NULL;
   }

   proto_server_send_update( c, update );

   return NULL;
}

void* callback_parse_mobs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
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
   scaffold_check_null( mob_id_test );

   if( 1 == biseqcstrcaseless( o->mob_id, mob_id_test ) ) {
      scaffold_print_debug(
         &module, "Client: Found mobile with ID: %b\n", o->mob_id
      );
      datafile_parse_mobile_ezxml_t( o, xml_data, NULL, TRUE );

      return o;
   }


#endif /* USE_EZXML */
cleanup:
   return NULL;
}

void* callback_parse_mob_channels( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
#ifdef USE_EZXML
   ezxml_t xml_data = (ezxml_t)arg;

   /* TODO: Return a condensed vector, somehow? */

   return vector_iterate( l->mobiles, callback_parse_mobs, xml_data );
#endif /* USE_EZXML */
   return NULL;
}

void* callback_attach_mob_sprites(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;
   struct GRAPHICS* g = NULL;

   /* Since the vector index is set by serial, there will be a number of      *
    * NULLs before we find one that isn't.                                    */
   if( NULL == o || NULL != o->sprites || NULL == o->sprites_filename ) {
      goto cleanup;
   }

   g = hashmap_get( &(c->sprites), o->sprites_filename );
   o->sprites = g;

cleanup:
   return NULL;
}

void* callback_attach_channel_mob_sprites(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   return vector_iterate( l->mobiles, callback_attach_mob_sprites, c );
}


void* callback_stop_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( NULL == arg || 0 == bstrcmp( nick, c->nick ) ) {
      scaffold_print_debug( &module, "Stopping client: %p\n", c );
      client_stop( c );
      return NULL;
   }
   return NULL;
}

BOOL callback_free_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;

   /* Since this is a refdec, it should be called anywhere clients are added
    * to a list and then the list is disposed of.
    */

   if( NULL == arg || 0 == bstrcmp( nick, c->nick ) ) {
#ifdef DEBUG_VERBOSE
      scaffold_print_debug( &module, "Freeing client: %p\n", c );
#endif /* DEBUG_VERBOSE */
      /* This is just a refdec. */
      //client_free( c );
      return TRUE;
   }
   return FALSE;
}

/** \brief Kick and free clients on all channels in the given list.
 **/
void* callback_remove_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring nick = (bstring)arg;

   hashmap_iterate( l->clients, callback_stop_clients, nick );
   hashmap_remove_cb( l->clients, callback_free_clients, nick );

   return NULL;
}

BOOL callback_free_channels( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
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
      return TRUE;
   }

   return FALSE;
}

/** \brief To be used with hashmap_remove_cb()
 */
BOOL callback_free_empty_channels(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CHANNEL* l = (struct CHANNEL*)iter;

   if( 0 >= hashmap_count( l->clients ) ) {
      return TRUE;
   }

   return FALSE;
}

/** \brief To be used with hashmap_remove_cb()
 */
BOOL callback_free_mobiles(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct MOBILE* o = (struct MOBILE*)iter;
   SCAFFOLD_SIZE* serial = (SCAFFOLD_SIZE*)arg;
   if( NULL == 0 ) {
      return TRUE;
   }
   if( NULL == arg || *serial == o->serial ) {
      mobile_free( o );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_tilesets( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   if( NULL == arg ) {
      tilemap_tileset_free( set );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_sprites( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct ITEM_SPRITE* sprite = (struct ITEM_SPRITE*)iter;
   if( NULL == arg ) {
      //item_sprite_free( sprite );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_catalogs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct ITEM_SPRITESHEET* cat = (struct ITEM_SPRITESHEET*)iter;
   if( NULL == arg ) {
      //item_spritesheet_free( cat );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_item_cache_items(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct ITEM* e = (struct ITEM*)iter;
   SCAFFOLD_SIZE serial = *((SCAFFOLD_SIZE*)arg);
   if( NULL == arg || serial == e->serial ) {
      item_free( e );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_item_caches(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct TILEMAP_ITEM_CACHE* cache = (struct TILEMAP_ITEM_CACHE*)iter;
   if( NULL == arg ) {
      tilemap_item_cache_free( cache );
      return TRUE;
   }
   return FALSE;
}

#ifdef USE_CHUNKS

BOOL callback_free_chunkers(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   bstring filename = (bstring)arg;
   scaffold_assert( CONTAINER_IDX_STRING == idx->type );
   if( NULL == filename || 0 == bstrcmp( idx->value.key, filename ) ) {
      struct CHUNKER* h = (struct CHUNKER*)iter;
      chunker_free( h );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_finished_chunkers(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct CHUNKER* h = (struct CHUNKER*)iter;

   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   if( chunker_chunk_finished( h ) ) {
      scaffold_print_debug(
         &module, "(Un)chunker for %s has finished. Removing...\n",
         bdata( idx->value.key )
      );
      chunker_free( h );
      return TRUE;
   }
   return FALSE;
}

#endif /* USE_CHUNKS */

/*
BOOL callback_free_commands( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   IRC_COMMAND* cmd = (IRC_COMMAND*)iter;
   irc_command_free( cmd );
   return TRUE;
}
*/

BOOL callback_free_generic( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   mem_free( iter );
   return TRUE;
}

#ifdef ENABLE_LOCAL_CLIENT

BOOL callback_free_controls( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   bstring key_search = (bstring)arg;
   SCAFFOLD_SIZE* idx_search = (SCAFFOLD_SIZE*)arg;
   struct UI_CONTROL* control = (struct UI_CONTROL*)iter;

   /* scaffold_assert( CONTAINER_IDX_STRING == idx->type ); */

   if(
      NULL == arg ||
      (CONTAINER_IDX_STRING == idx->type &&
         0 == bstrcmp( key_search, idx->value.key )) ||
      (CONTAINER_IDX_NUMBER == idx->type &&
         idx->value.index == *idx_search)
   ) {
      ui_control_free( control );
      return TRUE;
   }
   return FALSE;
}

#endif /* ENABLE_LOCAL_CLIENT */

BOOL callback_free_strings( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( iter, (bstring)arg ) ) {
      bdestroy( (bstring)iter );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_backlog( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct BACKLOG_LINE* line = (struct BACKLOG_LINE*)iter;
   /* TODO: Implement retroactively deleting lines by ID or something. */
   if( NULL == arg ) {
      bdestroy( line->line );
      bdestroy( line->nick );
      mem_free( line );
      return TRUE;
   }
   return FALSE;
}

#ifdef ENABLE_LOCAL_CLIENT

BOOL callback_free_graphics( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( (bstring)iter, (bstring)arg ) ) {
      graphics_surface_free( (GRAPHICS*)iter );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_windows( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   bstring wid = (bstring)arg;
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   scaffold_assert( NULL != iter );
   scaffold_assert( ui_get_local() == win->ui );
   if( NULL == arg || 0 == bstrcmp( wid, win->id ) ) {
      ui_window_free( win );
      return TRUE;
   }
   return FALSE;
}

#ifdef DEBUG
void* callback_assert_windows( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   scaffold_assert( NULL != iter );
   scaffold_assert( ui_get_local() == win->ui );
   return FALSE;
}
#endif /* DEBUG */

#endif /* ENABLE_LOCAL_CLIENT */

BOOL callback_free_ani_defs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct MOBILE_ANI_DEF* animation = (struct MOBILE_ANI_DEF*)iter;
   if( NULL == arg || 0 == bstrcmp( (bstring)arg, animation->name ) ) {
      mobile_animation_free( animation );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_spawners( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg ) {
   struct TILEMAP_SPAWNER* spawner = (struct TILEMAP_SPAWNER*)iter;
   if( NULL == arg || 0 == bstrcmp( (bstring)arg, spawner->id ) ) {
      bdestroy( spawner->id );
      return TRUE;
   }
   return FALSE;
}

VECTOR_SORT_ORDER callback_sort_chunker_tracks( void* a, void* b ) {
   CHUNKER_TRACK* cta = (CHUNKER_TRACK*)a;
   CHUNKER_TRACK* ctb = (CHUNKER_TRACK*)b;
   if( cta->start == ctb->start ) {
      return VECTOR_SORT_A_B_EQUAL;
   }
   return (cta->start > ctb->start) ? VECTOR_SORT_A_HEAVIER : VECTOR_SORT_A_LIGHTER;
 }
