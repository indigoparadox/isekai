
#include "channel.h"

#include "callbacks.h"

static void channel_cleanup( const struct _REF *ref ) {
   CHANNEL* l = scaffold_container_of( ref, struct _CHANNEL, refcount );
   hashmap_cleanup( &(l->clients) );
   bdestroy( l->name );
   bdestroy( l->topic );
   gamedata_cleanup( &(l->gamedata) );
}

void channel_free( CHANNEL* l ) {
   ref_dec( &(l->refcount) );
}

void channel_init( CHANNEL* l, const bstring name ) {
   ref_init( &(l->refcount), channel_cleanup );
   hashmap_init( &(l->clients) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
   scaffold_check_null( l->topic );
cleanup:
   return;
}

void channel_add_client( CHANNEL* l, CLIENT* c ) {
   scaffold_check_null( c );

   if( NULL != channel_get_client_by_name( l, c->nick ) ) {
      goto cleanup;
   }

   hashmap_put( &(l->clients), c->nick, c );

cleanup:
   return;
}

void channel_remove_client( CHANNEL* l, CLIENT* c ) {
   size_t deleted = 0;
   CLIENT* c_test = NULL;
   //deleted = vector_delete_cb( &(l->clients), callback_free_clients, c->nick );

   c_test = hashmap_get( &(l->clients), c->nick );

   if( NULL != c_test && TRUE == hashmap_remove( &(l->clients), c->nick ) ) {
      scaffold_print_debug(
         "Removed 1 clients from channel %s. %d remaining.\n",
         bdata( l->name ), hashmap_count( &(l->clients) )
      );
   }
}

CLIENT* channel_get_client_by_name( CHANNEL* l, bstring nick ) {
   //return vector_iterate( &(l->clients), callback_search_clients, nick );
   return hashmap_get( &(l->clients), nick );
}

/*
struct bstrList* channel_list_clients( CHANNEL* l ) {
   struct bstrList* list = NULL;

   list = bstrListCreate();
   scaffold_check_null( list );

   vector_iterate( &(l->clients), channel_lst_client, list );

cleanup:
   return list;
}
*/
