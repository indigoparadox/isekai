
#include "channel.h"

#include "callbacks.h"

void channel_init( CHANNEL* l, const bstring name ) {
   vector_init( &(l->clients) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
   scaffold_check_null( l->topic );
cleanup:
   return;
}

static void channel_cleanup( CHANNEL* l ) {
   vector_free( &(l->clients) );
   bdestroy( l->name );
   bdestroy( l->topic );
   gamedata_cleanup( &(l->gamedata) );
}

void channel_free( CHANNEL* l ) {
   ref_dec( &(l->refcount) );
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

void channel_remove_client( CHANNEL* l, CLIENT* c ) {
   size_t deleted = 0;
   deleted = vector_delete_cb( &(l->clients), cb_channel_del_clients, c->nick );
   scaffold_print_debug(
      "Removed %d clients from channel %s. %d remaining.\n",
      deleted, bdata( l->name ), vector_count( &(l->clients) )
   );
}

CLIENT* channel_get_client_by_name( CHANNEL* l, bstring nick ) {
   return vector_iterate( &(l->clients), callback_search_clients, nick );
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
