#include "channel.h"

void* channel_cmp_name( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CHANNEL* l = (CHANNEL*)iter;
   bstring name = (bstring)arg;

   if( 0 == bstrcmp( l->name, name ) ) {
      return l;
   }

   return NULL;
}

void channel_init( CHANNEL* l, const bstring name ) {
   vector_init( &(l->clients) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
   scaffold_check_null( l->topic );
cleanup:
   return;
}

void channel_cleanup( CHANNEL* l ) {
   vector_free( &(l->clients) );
   bdestroy( l->name );
   bdestroy( l->topic );
   gamedata_cleanup( &(l->gamedata) );
}

void channel_add_client( CHANNEL* l, CLIENT* c ) {
   scaffold_check_null( c );

   if( NULL != channel_get_client_by_name( l, c->nick ) ) {
      goto cleanup;
   }

   vector_add( &(l->clients), c );

cleanup:
   return;
}

void* channel_cmp_clients( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 == bstrcmp( nick, c->nick ) ) {
      return c;
   }
   return NULL;
}

#ifdef DEBUG
void* channel_cdb_clients( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring nick = (bstring)arg;
   if( 0 == bstrcmp( nick, c->nick ) ) {
      assert( 0 == c->sentinal );
      return c;
   }
   return NULL;
}
#endif /* DEBUG */

void channel_remove_client( CHANNEL* l, CLIENT* c ) {
   size_t deleted = 0;
#ifdef DEBUG
   /* Make sure clients have been cleaned up before deleting. */
   //vector_iterate( &(l->clients), channel_cdb_clients, c->nick );
#endif /* DEBUG */
   deleted = vector_delete_cb( &(l->clients), channel_cmp_clients, c->nick, FALSE );
   scaffold_print_debug(
      "Removed %d clients from channel %s. %d remaining.\n",
      deleted, bdata( l->name ), vector_count( &(l->clients) )
   );
}

CLIENT* channel_get_client_by_name( CHANNEL* l, bstring nick ) {
   return vector_iterate( &(l->clients), channel_cmp_clients, nick );
}

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

struct bstrList* channel_list_clients( CHANNEL* l ) {
   struct bstrList* list = NULL;

   list = bstrListCreate();
   scaffold_check_null( list );

   vector_iterate( &(l->clients), channel_lst_client, list );

cleanup:
   return list;
}
