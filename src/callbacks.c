
#include "callbacks.h"

#include "client.h"
#include "server.h"
#include "irc.h"

void* callback_ingest_commands( const bstring key, void* iter, void* arg ) {
   size_t last_read_count = 0;
   SERVER* s = NULL;
   static bstring buffer = NULL;
   CLIENT* c = (CLIENT*)iter;
   IRC_COMMAND* cmd = NULL;
   const IRC_COMMAND* table = NULL;

   /* Figure out if we're being called from a client or server. */
   if( SERVER_SENTINAL == ((CLIENT*)arg)->sentinal ) {
      s = (SERVER*)arg;
      table = irc_table_server;
   } else {
      table = irc_table_client;
   }

   /* Make sure a buffer is present. */
   if( NULL == buffer ) {
      buffer = bfromcstralloc( CONNECTION_BUFFER_LEN, "" );
   } else {
      bwriteallow( (*buffer) ); /* Unprotect the buffer. */
      btrunc( buffer, 0 );
   }

   /* TODO: Do we need the buffer to be part of the client anymore? */

   last_read_count = connection_read_line( &(c->link), buffer, FALSE );
   btrimws( buffer );
   bwriteprotect( (*buffer) ); /* Protect the buffer until next read. */

   if( 0 >= last_read_count ) {
      /* TODO: Handle error reading. */
      goto cleanup;
   }

   scaffold_print_debug(
      "Server: Line received from %d: %s\n",
      c->link.socket, bdata( buffer )
   );

   cmd = irc_dispatch( table, s, c, buffer );

cleanup:
   return cmd;
}

void* callback_search_clients( const bstring key, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 == bstrcmp( nick, c->nick ) ) {
      return c;
   }
   return NULL;
}

void* callback_search_channels( const bstring key, void* iter, void* arg ) {
   CHANNEL* l = (CHANNEL*)iter;
   bstring name = (bstring)arg;
   if( 0 == bstrcmp( l->name, name ) ) {
      return l;
   }
   return NULL;
}

BOOL callback_free_clients( const bstring key, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 == bstrcmp( nick, c->nick ) ) {
      client_free( c );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_channels( const bstring key, void* iter, void* arg ) {
   CHANNEL* l = (CHANNEL*)iter;
   bstring name = (bstring)arg;

   if( NULL == arg || 0 == bstrcmp( l->name, name ) ) {
      channel_free( l );
      return TRUE;
   }

   return FALSE;
}

/*
static void* server_prn_channel( VECTOR* v, size_t idx, void* iter, void* arg ) {
   SERVER_PBUFFER* pbuffer = (SERVER_PBUFFER*)arg;
   CLIENT* c_iter = (CLIENT*)iter;
   if( 0 == bstrcmp( pbuffer->c_sender->nick, c_iter->nick ) ) {
      return NULL;
   }
   server_client_send( pbuffer->s, c_iter, pbuffer->buffer );
   return NULL;
}
*/

#if 0
static void* channel_lst_client( VECTOR* v, size_t idx, void* iter, void* arg ) {
   struct bstrList* list = (struct bstrList*)arg;
   CLIENT* c = (CLIENT*)iter;
   int bstr_check;
   size_t client_count = vector_count( v );

   /* Make sure we have enough space for all clients. */
   if( list->mlen < client_count ) {
      bstr_check = bstrListAlloc( list, client_count );
      scaffold_check_nonzero( bstr_check );
   }

   list->entry[list->qty] = c->nick;
   list->qty++;

cleanup:
   return NULL;
}
#endif
