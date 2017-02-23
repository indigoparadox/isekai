#include "channel.h"

void channel_init( CHANNEL* l, const bstring name ) {
   vector_init( &(l->clients) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
cleanup:
   return;
}

void channel_cleanup( CHANNEL* l ) {
   vector_free( &(l->clients) );
   bdestroy( l->name );
   bdestroy( l->topic );
   gamedata_cleanup( &(l->gamedata) );
}

BOOL channel_client_present( CHANNEL* l, CLIENT* c ) {
   int i;
   CLIENT* c_test = NULL;
   BOOL found = FALSE;

   channel_lock_clients( l, TRUE );
   for( i = 0 ; vector_count( &(l->clients) ) > i ; i++ ) {
      c_test = (CLIENT*)vector_get( &(l->clients), i );
      if( NULL != c_test && 0 == bstrcmp( c_test->nick, c->nick ) ) {
         found = TRUE;
         break;
      }
   }
   channel_lock_clients( l, FALSE );

   return found;
}

void channel_add_client( CHANNEL* l, CLIENT* c ) {
   if( channel_client_present( l, c ) ) {
      goto cleanup;
   }

   channel_lock_clients( l, TRUE );
   vector_add( &(l->clients), c );
   channel_lock_clients( l, FALSE );

   client_lock_channels( c, TRUE );
   client_lock_channels( c, FALSE );

cleanup:
   return;
}

void channel_remove_client( CHANNEL* l, CLIENT* c ) {
   CLIENT* c_test = NULL;
   int i;

   channel_lock_clients( l, TRUE );
   for( i = 0 ; vector_count( &(l->clients) ) > i ; i++ ) {
      c_test = (CLIENT*)vector_get( &(l->clients), i );
      if( NULL != c_test && 0 == bstrcmp( c_test->nick, c->nick ) ) {
         vector_delete( &(l->clients), i );
         goto cleanup;
      }
   }

cleanup:
   channel_lock_clients( l, FALSE );
   return;
}

CLIENT* channel_get_client_by_name( CHANNEL* l, bstring nick ) {
   CLIENT* c_test = NULL;
   int i;

   channel_lock_clients( l, TRUE );
   for( i = 0 ; vector_count( &(l->clients) ) > i ; i++ ) {
      c_test = (CLIENT*)vector_get( &(l->clients), i );
      if( NULL != c_test && 0 == bstrcmp( c_test->nick, nick ) ) {
         goto cleanup;
      }
   }
   c_test = NULL;

cleanup:
   channel_lock_clients( l, FALSE );
   return c_test;
}

struct bstrList* channel_list_clients( CHANNEL* l ) {
   struct bstrList* list = NULL;
   int bstr_check;
   int client_count;
   int i;
   CLIENT* c;

   channel_lock_clients( l, TRUE );

   client_count = vector_count( &(l->clients) );
   list = bstrListCreate();
   scaffold_check_null( list );
   bstr_check = bstrListAlloc( list, client_count );
   scaffold_check_nonzero( bstr_check );

   for( i = 0 ; client_count > i ; i++ ) {
      c = (CLIENT*)vector_get( &(l->clients), i );
      if( NULL != c ) {
         list->entry[list->qty] = c->nick;
         list->qty++;
      }
   }

cleanup:
   channel_lock_clients( l, FALSE );
   return list;
}

void channel_lock_clients( CHANNEL* l, BOOL lock ) {
#ifdef USE_THREADS
#error Locking mechanism undefined!
#endif /* USE_THREADS */
}
