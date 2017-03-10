
#include "channel.h"

#include "callbacks.h"
#include "tilemap.h"

static void channel_cleanup( const struct REF *ref ) {
   struct CHANNEL* l = scaffold_container_of( ref, struct CHANNEL, refcount );

   /* FIXME: Actually free stuff. */
   hashmap_remove_cb( &(l->clients), callback_free_clients, NULL );
   hashmap_cleanup( &(l->clients) );
   vector_remove_cb( &(l->mobiles), callback_free_mobiles, NULL );
   vector_free( &(l->mobiles) );

   bdestroy( l->name );
   bdestroy( l->topic );

   /* FIXME: Free channel. */
}

void channel_free( struct CHANNEL* l ) {
   ref_dec( &(l->refcount) );
}

void channel_init( struct CHANNEL* l, const bstring name, BOOL local_images ) {
   ref_init( &(l->refcount), channel_cleanup );
   hashmap_init( &(l->clients) );
   vector_init( &(l->mobiles ) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
   scaffold_check_null( l->topic );
   l->tilemap = NULL;
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
   } while( NULL != vector_get( &(l->mobiles), o->serial ) );
   client_set_puppet( c, o );
   channel_add_mobile( l, o );
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
         channel_remove_mobile( l, c->puppet->serial );
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


void channel_add_mobile( struct CHANNEL* l, struct MOBILE* o ) {
   vector_set( &(l->mobiles), o->serial, o, TRUE );
}

void channel_remove_mobile( struct CHANNEL* l, SCAFFOLD_SIZE serial ) {
   vector_remove( &(l->mobiles), serial );
}

void channel_load_tilemap( struct CHANNEL* l ) {
   bstring mapdata_filename = NULL,
      mapdata_path = NULL;
   BYTE* mapdata_buffer = NULL;
   int bstr_retval;
   SCAFFOLD_SIZE_SIGNED bytes_read = 0;
   SCAFFOLD_SIZE mapdata_size = 0;

   mapdata_filename = bfromcstralloc( CHUNKER_FILENAME_ALLOC, "" );
   scaffold_check_null( mapdata_filename );

   scaffold_print_info( "Loading tilemap for channel: %s\n", bdata( l->name ) );
   mapdata_filename = bstrcpy( l->name );
   scaffold_check_null( mapdata_filename );
   bdelete( mapdata_filename, 0, 1 ); /* Get rid of the # */

   tilemap_new( l->tilemap, FALSE );

   mapdata_path = bstrcpy( &str_chunker_server_path );
   scaffold_check_null( mapdata_path );

   scaffold_join_path( mapdata_path, mapdata_filename );
   scaffold_check_nonzero( scaffold_error );

   /* TODO: Different map filename extensions. */

#ifdef USE_EZXML

   bstr_retval = bcatcstr( mapdata_path, ".tmx" );
   scaffold_check_nonzero( bstr_retval );

   scaffold_print_info( "Loading for XML data in: %s\n", bdata( mapdata_path ) );
   bytes_read = scaffold_read_file_contents( mapdata_path, &mapdata_buffer, &mapdata_size );
   scaffold_check_null_msg( mapdata_buffer, "Unable to load tilemap data." );
   scaffold_check_zero_msg( bytes_read, "Unable to load tilemap data." );

   datafile_parse_tilemap_ezxml( l->tilemap, mapdata_buffer, mapdata_size, FALSE );
#endif // USE_EZXML

cleanup:
   bdestroy( mapdata_filename );
   bdestroy( mapdata_path );
   if( NULL != mapdata_buffer ) {
      free( mapdata_buffer );
   }
   return;
}
