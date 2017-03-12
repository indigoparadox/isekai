
#include "callbacks.h"

#include "client.h"
#include "server.h"
#include "proto.h"
#include "chunker.h"
#include "tilemap.h"

void* callback_ingest_commands( const bstring key, void* iter, void* arg ) {
   SCAFFOLD_SIZE last_read_count = 0;
   SERVER* s = NULL;
   static bstring buffer = NULL;
   struct CLIENT* c = (struct CLIENT*)iter;
   IRC_COMMAND* cmd = NULL;
   const IRC_COMMAND* table = NULL;
   /* TODO: Factor this out. */
   int bstr_result;

   /* Figure out if we're being called from a client or server. */
   if( NULL == arg || CLIENT_SENTINAL == ((struct CLIENT*)arg)->sentinal ) {
      table = proto_table_client;
   } else if( SERVER_SENTINAL == ((struct CLIENT*)arg)->sentinal ) {
      s = (SERVER*)arg;
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
   last_read_count = connection_read_line( &(c->link), buffer, c->client_side );
   btrimws( buffer );
   bwriteprotect( (*buffer) ); /* Protect the buffer until next read. */

   if( 0 >= last_read_count ) {
      goto cleanup;
   }

#ifdef DEBUG_NETWORK
   scaffold_print_debug(
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

void* callback_search_tilesets_name( const bstring key, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   bstring name = (bstring)arg;

   if( 0 == bstrcmp( key, name ) ) {
      return set;
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

void* callback_search_tilesets_gid( const bstring res, void* iter, void* arg ) {
   SCAFFOLD_SIZE gid = (*(SCAFFOLD_SIZE*)arg);
   struct TILEMAP_TILESET* tileset = (struct TILEMAP_TILESET*)iter;

   if( tileset->firstgid <= gid ) {
      return tileset;
   }

   return NULL;
}

void* callback_draw_mobiles( const bstring res, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;

   if( NULL == o ) { return NULL; }

   mobile_animate( o );
   mobile_draw_ortho( o, twindow );

   return NULL;
}

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

#if 0
BOOL callback_send_list_to_client( const bstring res, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   bstring xmit_buffer = (bstring)iter;

   client_send( c, xmit_buffer );

   bdestroy( xmit_buffer );

   return TRUE;
}
#endif // 0

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
         "Chunker for %s has finished. Removing...\n", bdata( key )
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
         "Unchunker for %s has finished. Removing...\n", bdata( key )
      );
      chunker_free( h );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_commands( const bstring res, void* iter, void* arg ) {
   IRC_COMMAND* cmd = (IRC_COMMAND*)iter;
   irc_command_free( cmd );
   return TRUE;
}

BOOL callback_free_generic( const bstring res, void* iter, void* arg ) {
   free( iter );
   return TRUE;
}

BOOL callback_free_strings( const bstring res, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( iter, (bstring)arg ) ) {
      bdestroy( (bstring)iter );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_graphics( const bstring res, void* iter, void* arg ) {
   if( NULL == arg || 0 == bstrcmp( iter, (bstring)arg ) ) {
      graphics_surface_free( (GRAPHICS*)iter );
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
