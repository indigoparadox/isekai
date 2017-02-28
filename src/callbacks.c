
#include "callbacks.h"

#include "client.h"
#include "server.h"

typedef struct {
   SERVER* s;
   bstring buffer;
   CLIENT* c_sender;
} SERVER_PBUFFER;

typedef struct {
   SERVER* s;
   bstring nick;
} SERVER_DUO;

void* callback_search_clients( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 == bstrcmp( nick, c->nick ) ) {
      return c;
   }
   return NULL;
}

void* callback_search_channels( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CHANNEL* l = (CHANNEL*)iter;
   bstring name = (bstring)arg;
   if( 0 == bstrcmp( l->name, name ) ) {
      return l;
   }
   return NULL;
}

BOOL callback_free_clients( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 == bstrcmp( nick, c->nick ) ) {
      client_free( c );
      return TRUE;
   }
   return FALSE;
}

BOOL callback_free_channels( VECTOR* v, size_t idx, void* iter, void* arg ) {
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
