
#include "channel.h"

#include "callbacks.h"

static void channel_cleanup( const struct REF *ref ) {
   struct CHANNEL* l = scaffold_container_of( ref, struct CHANNEL, refcount );
   hashmap_cleanup( &(l->clients) );

   bdestroy( l->name );
   bdestroy( l->topic );

   gamedata_cleanup( &(l->gamedata) );
   /* FIXME: Free channel. */
}

void channel_free( struct CHANNEL* l ) {
   ref_dec( &(l->refcount) );
}

void channel_init( struct CHANNEL* l, const bstring name ) {
   ref_init( &(l->refcount), channel_cleanup );
   hashmap_init( &(l->clients) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
   scaffold_check_null( l->topic );
cleanup:
   return;
}

void channel_add_client( struct CHANNEL* l, struct CLIENT* c ) {
   struct MOBILE* o = NULL;

   scaffold_check_null( c );

   if( NULL != channel_get_client_by_name( l, c->nick ) ) {
      goto cleanup;
   }

   /* Create a basic mobile for the new client. */
   mobile_new( o );
   do {
      o->serial = rand() % UCHAR_MAX;
   } while( NULL != vector_get( &(l->gamedata.mobiles), o->serial ) );
   client_set_puppet( c, o );
   gamedata_add_mobile( &(l->gamedata), o );
   mobile_set_channel( o, l );

   hashmap_put( &(l->clients), c->nick, c );

cleanup:
   return;
}

void channel_remove_client( struct CHANNEL* l, struct CLIENT* c ) {
   struct CLIENT* c_test = NULL;
   c_test = hashmap_get( &(l->clients), c->nick );
   if( NULL != c_test && TRUE == hashmap_remove( &(l->clients), c->nick ) ) {
      if( NULL != c->puppet ) {
         gamedata_remove_mobile( &(l->gamedata), c->puppet->serial );
      }

      scaffold_print_debug(
         "Removed 1 clients from channel %s. %d remaining.\n",
         bdata( l->name ), hashmap_count( &(l->clients) )
      );
   }
}

struct CLIENT* channel_get_client_by_name( struct CHANNEL* l, bstring nick ) {
   return hashmap_get( &(l->clients), nick );
}
