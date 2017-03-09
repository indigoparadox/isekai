
#include "callbacks.h"

#include "client.h"
#include "server.h"
#include "proto.h"
#include "chunker.h"

void* callback_ingest_commands( const bstring key, void* iter, void* arg ) {
   SCAFFOLD_SIZE last_read_count = 0;
   SERVER* s = NULL;
   static bstring buffer = NULL;
   struct CLIENT* c = (struct CLIENT*)iter;
   IRC_COMMAND* cmd = NULL;
   const IRC_COMMAND* table = NULL;
   /* TODO: Factor this out. */
   BOOL is_client = FALSE;

   /* Figure out if we're being called from a client or server. */
   if( NULL == arg || CLIENT_SENTINAL == ((struct CLIENT*)arg)->sentinal ) {
      table = proto_table_client;
      is_client = TRUE;
   } else if( SERVER_SENTINAL == ((struct CLIENT*)arg)->sentinal ) {
      s = (SERVER*)arg;
      table = proto_table_server;
      is_client = FALSE;
   } else {
      scaffold_assert( NULL == arg ); /* Just die. */
   }

   /* Make sure a buffer is present. */
   if( NULL == buffer ) {
      buffer = bfromcstralloc( CONNECTION_BUFFER_LEN, "" );
   } else {
      bwriteallow( (*buffer) ); /* Unprotect the buffer. */
      btrunc( buffer, 0 );
   }

   /* Get the next line and clean it up. */
   last_read_count = connection_read_line( &(c->link), buffer, is_client );
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
   server_client_send( c, buffer );
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
void* callback_search_tilesets_img_name( const bstring key, void* iter, void* arg ) {
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;
   if( NULL != hashmap_iterate( &(set->images), callback_search_graphics, arg ) ) {
      /* This is the tileset that contains this image. */
      return set;
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
   bstring xmit_buffer_template = (bstring)arg;
   struct CHUNKER* h = (struct CHUNKER*)iter;
   bstring xmit_buffer_out = NULL;
#ifdef DEBUG
   int bstr_result;
#endif /* DEBUG */

   if( chunker_chunk_finished( h ) ) {
      goto cleanup;
   }

   scaffold_assert( 0 < blength( xmit_buffer_template ) );
   xmit_buffer_out = bstrcpy( xmit_buffer_template );
   scaffold_check_null( xmit_buffer_out );
   scaffold_assert( 0 < blength( xmit_buffer_out ) );

   /* Note the starting point and progress for the client. */
#ifdef DEBUG
   bstr_result =
#endif /* DEBUG */
   bformata(
      xmit_buffer_out, "%s TILEMAP %s %d %d %d %d : ",
      bdata( h->channel ), bdata( key ), h->type, h->raw_position,
      h->tx_chunk_length, h->raw_length
   );
   scaffold_assert( BSTR_OK == bstr_result );

   chunker_chunk_pass( h, xmit_buffer_out );

cleanup:
   return xmit_buffer_out;
}

void* callback_proc_chunkers( const bstring key, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;
   SERVER* s = (SERVER*)arg;
   bstring xmit_buffer_template = NULL;
   struct VECTOR* chunks = NULL;
   bstring chunk_iter = NULL;

   xmit_buffer_template = bformat(
      ":%s GDB %s ", bdata( s->self.remote ), bdata( c->nick )
   );
   scaffold_check_null( xmit_buffer_template );

   /* Process some compression chunks. */
   chunks = hashmap_iterate_v(
      &(c->chunkers), callback_send_chunkers_l, xmit_buffer_template
   );

   /* Removed any finished chunkers. */
   hashmap_remove_cb(
      &(c->chunkers), callback_free_finished_chunkers, NULL
   );

   if( NULL == chunks ) {
      goto cleanup; /* Silently. */
   }

   /* Send the processed chunks to the client. */
   vector_remove_cb( chunks, callback_send_list_to_client, c );

cleanup:
   chunk_iter = vector_get( chunks, 0 );
   while( NULL != chunk_iter ) {
      bdestroy( chunk_iter );
      vector_remove( chunks, 0 );
      vector_get( chunks, 0 );
   }
   if( NULL != chunks ) {
      vector_free( chunks );
   }
   bdestroy( xmit_buffer_template );
   return NULL;
}

void* callback_proc_tileset_img_gs( const bstring key, void* iter, void* arg ) {
   struct CHANNEL_CLIENT* lc = (struct CHANNEL_CLIENT*)arg;

   client_request_file(
      lc->c, lc->l,
      CHUNKER_DATA_TYPE_TILESET_IMG,
      key
   );

   return NULL;
}

void* callback_proc_tileset_imgs( const bstring key, void* iter, void* arg ) {
   struct CHANNEL_CLIENT* lc = (struct CHANNEL_CLIENT*)arg;
   struct TILEMAP_TILESET* set = (struct TILEMAP_TILESET*)iter;

   return hashmap_iterate( &(set->images), callback_proc_tileset_img_gs, lc );
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

   mobile_draw_ortho( o, twindow );

   return NULL;
}

void* callback_send_mobs_to_client( const bstring res, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct MOBILE* o = (struct MOBILE*)iter;

   if( NULL == c || NULL == o ) {
      return NULL;
   }

   server_client_printf(
      c, "MOB %b %d %b %b",
      o->channel->name, o->serial, o->sprites_filename, o->owner->nick
   );

   return NULL;
}

void* callback_send_mobs_to_channel( const bstring res, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)arg;
   struct CLIENT* c = (struct CLIENT*)iter;

   vector_iterate( &(l->gamedata.mobiles), callback_send_mobs_to_client, c );

   return NULL;
}

BOOL callback_send_list_to_client( const bstring res, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   bstring xmit_buffer = (bstring)iter;

   server_client_send( c, xmit_buffer );

   bdestroy( xmit_buffer );

   return TRUE;
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
