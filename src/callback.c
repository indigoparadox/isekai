
#define CALLBACKS_C
#include "callback.h"

#include "client.h"
#include "server.h"
#include "proto.h"
#include "chunker.h"
#include "tilemap.h"
#include "ui.h"
#ifdef USE_EZXML
#include "datafile.h"
#include "ezxml.h"
#endif /* USE_EZXML */

void* callback_ingest_commands( const bstring key, void* iter, void* arg ) {
   SCAFFOLD_SIZE last_read_count = 0;
   struct SERVER* s = NULL;
   static bstring buffer = NULL;
   struct CLIENT* c = (struct CLIENT*)iter;
   IRC_COMMAND* cmd = NULL;
   const IRC_COMMAND* table = NULL;
   /* TODO: Factor this out. */
   int bstr_result;

#ifdef ENABLE_LOCAL_CLIENT
   /* Figure out if we're being called from a client or server. */
   if( NULL == arg || CLIENT_SENTINAL == ((struct CLIENT*)arg)->sentinal ) {
      table = proto_table_client;
   } else
#endif /* ENABLE_LOCAL_CLIENT */
   if( SERVER_SENTINAL == ((struct CLIENT*)arg)->sentinal ) {
      s = (struct SERVER*)arg;
      table = proto_table_server;
   } else {
      scaffold_assert( NULL == arg ); /* Just die. */
   }

   /* Make sure a buffer is present. */
   if( NULL == buffer ) {
      buffer = bfromcstralloc( CONNECTION_BUFFER_LEN, "" );
   } else {
      bwriteallow( (*buffer) ); /* Unprotect the buffer. */
      bstr_result = btrunc( buffer, 0 );
      scaffold_check_nonzero( bstr_result );
   }

   /* Get the next line and clean it up. */
   last_read_count = connection_read_line(
      &(c->link),
      buffer,
#ifdef ENABLE_LOCAL_CLIENT
      c->client_side
#else
      FALSE
#endif /* ENABLE_LOCAL_CLIENT */
   );
   btrimws( buffer );
   bwriteprotect( (*buffer) ); /* Protect the buffer until next read. */

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

/* Append all clients to the bstrlist arg. */
void* callback_concat_clients( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct bstrList* list = (struct bstrList*)arg;
   scaffold_list_append_string_cpy( list, c->nick );
   return NULL;
}

/* Return only the client arg if present. */
void* callback_search_clients( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( NULL == arg || 0 == bstrcmp( nick, c->nick ) ) {
      return c;
   }
   return NULL;
}

/* Return all clients EXCEPT the client arg. */
void* callback_search_clients_r( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 != bstrcmp( nick, c->nick ) ) {
      return c;
   }
   return NULL;
}

/* Return any client that is in the bstrlist arg. */
void* callback_search_clients_l( const bstring key, void* iter, void* arg ) {
   const struct bstrList* list = (const struct bstrList*)arg;
   struct CLIENT* c = (struct CLIENT*)iter;
   int i;

   for( i = 0 ; list->qty > i ; i++ ) {
      if( 0 == bstrcmp( c->nick, list->entry[i] ) ) {
         return c;
      }
   }

   return NULL;
}

void* callback_send_clients( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring buffer = (bstring)arg;
   client_send( c, buffer );
   return NULL;
}

void* callback_search_channels( const bstring key, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring name = (bstring)arg;
   if( 0 == bstrcmp( l->name, name ) ) {
      return l;
   }
   return NULL;
}

void * callback_search_servefiles( const bstring res, void* iter, void* arg ) {
   bstring file_iter = (bstring)iter,
      file_iter_short = NULL,
      file_search = (bstring)arg;

   file_iter_short = bmidstr(
      file_iter,
      str_server_data_path.slen + 1,
      blength( file_iter ) - str_server_data_path.slen - 1
   );
   scaffold_check_null( file_iter_short );

   if( 0 != bstrcmp( file_iter_short, file_search ) ) {
      /* FIXME: If the files request and one of the files present start    *
       * with similar names (tilelist.png.tmx requested, tilelist.png      *
       * exists, eg), then weird stuff happens.                            */
      /* FIXME: Don't try to send directories. */
      bdestroy( file_iter_short );
      file_iter_short = NULL;
   } else {
      scaffold_print_debug( &module, "Server: File Found: %s\n", bdata( file_iter ) );
   }

cleanup:
   return file_iter_short;
}

void* callback_get_tile_stack_l( bstring key, void* iter, void* arg ) {
   struct TILEMAP_LAYER* layer = (struct TILEMAP_LAYER*)iter;
   struct TILEMAP_POSITION* pos = (struct TILEMAP_POSITION*)arg;
   struct TILEMAP* t = layer->tilemap;
   struct TILEMAP_TILESET* set = NULL;
   uint32_t gid = 0;
   struct TILEMAP_TILE_DATA* tdata = NULL;

   scaffold_assert( TILEMAP_SENTINAL == t->sentinal );

   gid = tilemap_get_tile( layer, pos->x, pos->y );
   set = tilemap_get_tileset( t, gid );
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

#ifdef ENABLE_LOCAL_CLIENT

void* callback_search_windows( const bstring key, void* iter, void* arg ) {
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

void* callback_search_tilesets_img_name( const bstring key, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   if( NULL != hashmap_iterate( &(set->images), callback_search_graphics, arg ) ) {
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
/*  */
void* callback_search_channels_tilemap_img_name( const bstring key, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   struct TILEMAP* t = &(l->tilemap);
   if( NULL != hashmap_iterate( &(t->tilesets), callback_search_tilesets_img_name, arg ) ) {
      return l;
   }
   return NULL;
}

void* callback_search_graphics( const bstring key, void* iter, void* arg ) {
   GRAPHICS* g = (GRAPHICS*)iter;
   bstring s_key = (bstring)arg;

   if( 0 == bstrcmp( key, s_key ) ) {
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

void* callback_search_tilesets_name( const bstring key, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   bstring name = (bstring)arg;

   if( 0 == bstrcmp( key, name ) ) {
      return set;
   }
   return NULL;
}

#ifdef USE_CHUNKS

void* callback_proc_client_chunkers( const bstring key, void* iter, void* arg ) {
   struct CHUNKER* h = (struct CHUNKER*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   if( chunker_unchunk_finished( h ) ) {
      /* Cached file found, so abort transfer. */
      if( chunker_unchunk_cached( h ) ) {
         proto_abort_chunker( c, h );
      }

      client_handle_finished_chunker( c, h );
   }
}

#endif /* USE_CHUNKS */

#endif /* ENABLE_LOCAL_CLIENT */

#ifdef USE_CHUNKS

void* callback_send_chunkers_l( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct CHUNKER* h = (struct CHUNKER*)iter;
   bstring chunk_out = NULL;
   SCAFFOLD_SIZE start_pos = 0;

   if( chunker_chunk_finished( h ) ) {
      goto cleanup;
   }

   chunk_out = bfromcstralloc( CHUNKER_DEFAULT_CHUNK_SIZE, "" );
   start_pos = chunker_chunk_pass( h, chunk_out );
   proto_send_chunk( c, h, start_pos, key, chunk_out );

cleanup:
   bdestroy( chunk_out );
   return NULL;
}

void* callback_proc_chunkers( const bstring key, void* iter, void* arg ) {
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

void* callback_proc_channel_vms( const bstring res, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;

   scaffold_assert_server();

   if( channel_vm_can_step( l ) ) {
      channel_vm_step( l );
   }

   return NULL;
}

void* callback_search_tilesets_gid( const bstring res, void* iter, void* arg ) {
   SCAFFOLD_SIZE gid = (*(SCAFFOLD_SIZE*)arg);
   struct TILEMAP_TILESET* tileset = (struct TILEMAP_TILESET*)iter;

   if( tileset->firstgid <= gid ) {
      return tileset;
   }

   return NULL;
}

#ifdef ENABLE_LOCAL_CLIENT

void* callback_proc_tileset_img_gs( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;

   scaffold_assert( NULL == iter );

   client_request_file( c, CHUNKER_DATA_TYPE_TILESET_IMG, key );

   return NULL;
}

void* callback_proc_tileset_imgs( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;

   return hashmap_iterate( &(set->images), callback_proc_tileset_img_gs, c );
}

void* callback_draw_mobiles( const bstring res, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;

   if( NULL == o ) { return NULL; }

   mobile_animate( o );
   mobile_draw_ortho( o, twindow );

   return NULL;
}

#endif /* ENABLE_LOCAL_CLIENT */

void* callback_send_mobs_to_client( const bstring res, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct MOBILE* o = (struct MOBILE*)iter;

   if( NULL == c || NULL == o ) {
      return NULL;
   }

   proto_send_mob( c, o );

   return NULL;
}

void* callback_send_mobs_to_channel( const bstring res, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)arg;
   struct CLIENT* c = (struct CLIENT*)iter;

   vector_iterate( &(l->mobiles), callback_send_mobs_to_client, c );

   return NULL;
}

void* callback_send_updates_to_client( const bstring res, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct MOBILE_UPDATE_PACKET* update = (struct MOBILE_UPDATE_PACKET*)arg;

   if( NULL == c || NULL == update->o ) {
      return NULL;
   }

   proto_server_send_update( c, update );

   return NULL;
}

void* callback_parse_mobs( const bstring res, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
#ifdef USE_EZXML
   ezxml_t xml_data = (ezxml_t)arg;
   const char* mob_id_test = NULL;
   const char* mob_id_c = NULL;

   /* Since the vector index is set by serial, there will be a number of      *
    * NULLs before we find one that isn't.                                    */
   if( NULL == iter ) {
      goto cleanup;
   }

   mob_id_test = ezxml_attr( xml_data, "id" );
   scaffold_check_null( mob_id_test );

   mob_id_c = bdata( o->mob_id );
   if( 0 == strncmp( mob_id_c, mob_id_test, blength( o->mob_id ) ) ) {
      scaffold_print_debug(
         &module, "Client: Found mobile with ID: %b\n", o->mob_id
      );
      datafile_parse_mobile_ezxml_t( o, xml_data, TRUE );
      return o;
   }

#endif /* USE_EZXML */
cleanup:
   return NULL;
}

void* callback_parse_mob_channels( const bstring key, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
#ifdef USE_EZXML
   ezxml_t xml_data = (ezxml_t)arg;

   /* TODO: Return a condensed vector, somehow? */

   return vector_iterate( &(l->mobiles), callback_parse_mobs, xml_data );
#endif /* USE_EZXML */
   return NULL;
}

BOOL callback_free_clients( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( NULL == arg || 0 == bstrcmp( nick, c->nick ) ) {
      client_free( c );
      return TRUE;
   }
   return FALSE;
}

void* callback_remove_clients( const bstring res, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring nick = (bstring)arg;

   hashmap_remove_cb( &(l->clients), callback_free_clients, nick );

   return NULL;
}

BOOL callback_free_channels( const bstring key, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;
   bstring name = (bstring)arg;

   if( NULL == arg || 0 == bstrcmp( l->name, name ) ) {
      channel_free( l );
      return TRUE;
   }

   return FALSE;
}

BOOL callback_free_empty_channels( const bstring key, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)iter;

   if( 0 >= hashmap_count( &(l->clients) ) ) {
      channel_free( l );
      return TRUE;
   }

   return FALSE;
}

BOOL callback_free_mobiles( const bstring res, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   SCAFFOLD_SIZE* serial = (SCAFFOLD_SIZE*)arg;
   if( NULL == arg || *serial == o->serial ) {
      mobile_free( o );
      return TRUE;
   }
   return FALSE;
}

#ifdef USE_CHUNKS

BOOL callback_free_chunkers( const bstring key, void* iter, void* arg ) {
   bstring filename = (bstring)arg;
   if( NULL == filename || 0 == bstrcmp( key, filename ) ) {
      struct CHUNKER* h = (struct CHUNKER*)iter;
      chunker_free( h );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_finished_chunkers( const bstring key, void* iter, void* arg ) {
   struct CHUNKER* h = (struct CHUNKER*)iter;
   if( chunker_chunk_finished( h ) ) {
      scaffold_print_debug(
         &module, "Chunker for %s has finished. Removing...\n", bdata( key )
      );
      chunker_free( h );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_finished_unchunkers( const bstring key, void* iter, void* arg ) {
   struct CHUNKER* h = (struct CHUNKER*)iter;
   if( chunker_unchunk_finished( h ) ) {
      scaffold_print_debug(
         &module, "Unchunker for %s has finished. Removing...\n", bdata( key )
      );
      chunker_free( h );
      return TRUE;
   }
   return FALSE;
}

#endif /* USE_CHUNKS */

BOOL callback_free_commands( const bstring res, void* iter, void* arg ) {
   IRC_COMMAND* cmd = (IRC_COMMAND*)iter;
   irc_command_free( cmd );
   return TRUE;
}

BOOL callback_free_generic( const bstring res, void* iter, void* arg ) {
   scaffold_free( iter );
   return TRUE;
}

#ifdef ENABLE_LOCAL_CLIENT

BOOL callback_free_controls( const bstring key, void* iter, void* arg ) {
   bstring id = (bstring)key;
   bstring id_search = (bstring)arg;
   struct UI_CONTROL* control = (struct UI_CONTROL*)iter;

   if( NULL == arg || 0 == bstrcmp( id_search, id ) ) {
      ui_control_free( control );
      return TRUE;
   }
   return FALSE;
}

#endif /* ENABLE_LOCAL_CLIENT */

BOOL callback_free_strings( const bstring res, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( iter, (bstring)arg ) ) {
      bdestroy( (bstring)iter );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_backlog( const bstring res, void* iter, void* arg ) {
   struct CHANNEL_BUFFER_LINE* line = (struct CHANNEL_BUFFER_LINE*)iter;
   /* TODO: Implement retroactively deleting lines by ID or something. */
   if( NULL == arg ) {
      bdestroy( line->line );
      bdestroy( line->nick );
      bdestroy( line->display_name );
      scaffold_free( line );
      return TRUE;
   }
   return FALSE;
}

#ifdef ENABLE_LOCAL_CLIENT

BOOL callback_free_graphics( const bstring res, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( iter, (bstring)arg ) ) {
      graphics_surface_free( (GRAPHICS*)iter );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_windows( const bstring res, void* iter, void* arg ) {
   bstring wid = (bstring)arg;
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   if( NULL == arg || 0 == bstrcmp( wid, win->id ) ) {
      ui_window_free( win );
      return TRUE;
   }
   return FALSE;
}

#endif /* ENABLE_LOCAL_CLIENT */

BOOL callback_free_ani_defs( const bstring key, void* iter, void* arg ) {
   struct MOBILE_ANI_DEF* animation = (struct MOBILE_ANI_DEF*)iter;
   if( NULL == arg || 0 == bstrcmp( (bstring)arg, animation->name ) ) {
      animation->frames.count = 0;
      vector_cleanup( &(animation->frames) );
      bdestroy( animation->name );
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
