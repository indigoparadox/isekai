
#define CHANNEL_C
#include "channel.h"

#include "callback.h"
#include "tilemap.h"
#include "datafile.h"
#include "server.h"
#include "proto.h"
#ifdef USE_TINYPY
#include "tinypy/tinypy.h"
#endif /* USE_TINYPY */
#ifdef USE_DUKTAPE
#include "duktape/duktape.h"
#include "duktape/dukhelp.h"
#endif /* USE_DUKTAPE */

static void channel_free_final( const struct REF *ref ) {
   struct CHANNEL* l = scaffold_container_of( ref, struct CHANNEL, refcount );

   scaffold_print_debug(
      &module,
      "Destroying channel: %b\n",
      l->name
   );

   if( channel_vm_can_step( l ) ) {
      channel_vm_end( l );
   }

   /* Actually free stuff. */
   hashmap_remove_cb( &(l->clients), callback_free_clients, NULL );
   hashmap_cleanup( &(l->clients) );
   vector_remove_cb( &(l->mobiles), callback_free_mobiles, NULL );
   vector_cleanup( &(l->mobiles) );

   bdestroy( l->name );
   bdestroy( l->topic );

   tilemap_free( &(l->tilemap) );

   /* Free channel. */
   scaffold_free( l );
}

void channel_free( struct CHANNEL* l ) {
   refcount_dec( l, "channel" );
}

void channel_init( struct CHANNEL* l, const bstring name, BOOL local_images ) {
   ref_init( &(l->refcount), channel_free_final );
   hashmap_init( &(l->clients) );
   vector_init( &(l->mobiles ) );
   vector_init( &(l->speech_backlog ) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
   scaffold_check_null( l->topic );
   tilemap_init( &(l->tilemap), local_images );
cleanup:
   return;
}

void channel_add_client( struct CHANNEL* l, struct CLIENT* c, BOOL spawn ) {
   struct MOBILE* o = NULL;
   struct TILEMAP* t = NULL;
   struct TILEMAP_SPAWNER* spawner = NULL;
   SCAFFOLD_SIZE bytes_read = 0;
   BYTE* mobdata_buffer = NULL;
   SCAFFOLD_SIZE mobdata_size = 0;
   bstring mobdata_path = NULL;
   struct VECTOR* player_spawns = NULL;
   SCAFFOLD_SIZE v_count = 0;

   scaffold_check_null( c );

   if( NULL != channel_get_client_by_name( l, c->nick ) ) {
      goto cleanup;
   }

   if( TRUE == spawn ) {
      scaffold_assert_server();
      t = &(l->tilemap);
      player_spawns = vector_iterate_v(
         &(t->spawners), callback_search_spawners, &str_player
      );
      scaffold_check_null_msg( player_spawns, "No player spawns available." );
      spawner =
         vector_get( player_spawns, rand() % vector_count( player_spawns ) );
   }

   if( NULL != spawner ) {
      /* Create a basic mobile for the new client. */
      /* TODO: Get the desired mobile data ID from client. */
      mobile_new(
         o, &str_mobile_def_id_default, spawner->pos.x, spawner->pos.y
      );

      do {
         o->serial = rand() % SERIAL_MAX;
      } while( NULL != vector_get( &(l->mobiles), o->serial ) );

      client_set_puppet( c, o );
      mobile_set_channel( o, l );
      channel_add_mobile( l, o );

      scaffold_print_info(
         &module,
         "Spawning %s (%d) at: %d, %d\n",
         bdata( c->nick ), o->serial, o->x, o->y
      );
   } else if( TRUE == spawn ) {
      scaffold_print_error(
         &module,
         "Unable to find mobile spawner for this map.\n"
      );
   }

   client_add_channel( c, l  );

   hashmap_put( &(l->clients), c->nick, c );

cleanup:
   if( NULL != player_spawns) {
      /* These don't have garbage refs, so just free the vector forcibly. */
      player_spawns->count = 0;
      vector_cleanup( player_spawns );
      free( player_spawns );
   }
   if( NULL != mobdata_buffer ) {
      scaffold_free( mobdata_buffer );
   }
   bdestroy( mobdata_path );
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
         &module, "Removed 1 clients from channel %s. %d remaining.\n",
         bdata( l->name ), hashmap_count( &(l->clients) )
      );
   }
}

struct CLIENT* channel_get_client_by_name( struct CHANNEL* l, bstring nick ) {
   return hashmap_get( &(l->clients), nick );
}

void channel_add_mobile( struct CHANNEL* l, struct MOBILE* o ) {
   mobile_set_channel( o, l );
   vector_set( &(l->mobiles), o->serial, o, TRUE );
}

void channel_set_mobile(
   struct CHANNEL* l, uint8_t serial, const bstring mob_id,
   const bstring def_filename, const bstring mob_nick,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, struct CLIENT* local_c
) {
   struct MOBILE* o = NULL;
   int bstr_res = 0;
   struct CLIENT* mobile_c = NULL;

//#if 0
//#ifdef STRICT_DEBUG
   const char* nick_c = NULL,
      * lname_c = NULL;
//#endif /* STRICT_DEBUG */

//   scaffold_assert( 0 < hashmap_count( &(l->clients) ) );
//   c = channel_get_client_by_name( l, mob_nick );
//#ifdef STRICT_DEBUG
   nick_c = bdata( mob_nick );
   lname_c = bdata( l->name );
   scaffold_assert( NULL != nick_c );
   scaffold_assert( NULL != lname_c );
//   scaffold_assert( NULL != c );
//#else
//   scaffold_check_null( c );
//#endif /* STRICT_DEBUG */
//#endif // 0

   o = vector_get( &(l->mobiles), serial );
   if( NULL == o ) {
      mobile_new( o, NULL, 0, 0 );
      o->serial = serial;
      if( NULL == o->def_filename ) {
         o->def_filename = bstrcpy( def_filename );
      } else {
         bstr_res = bassign( o->def_filename, def_filename );
         scaffold_check_nonzero( bstr_res );
      }
      scaffold_assert( NULL != o->def_filename );
      o->initialized = FALSE;
      if( NULL == o->mob_id ) {
         o->mob_id = bstrcpy( mob_id );
      } else {
         bstr_res = bassign( o->mob_id, mob_id );
         scaffold_check_nonzero( bstr_res );
      }
      mobile_set_channel( o, l );
      vector_set( &(l->mobiles), o->serial, o, TRUE );
      if( NULL != local_c && TRUE == local_c->client_side ) {
         client_request_file(
            local_c, CHUNKER_DATA_TYPE_MOBDEF, o->def_filename
         );
      }
   }

   scaffold_assert( 0 < hashmap_count( &(l->clients) ) );
   mobile_c = channel_get_client_by_name( l, mob_nick );
   if( NULL != mobile_c && 0 == bstrcmp( mobile_c->nick, mob_nick ) ) {
      client_set_puppet( mobile_c, o );
   }

   bstr_res = bassign( o->display_name, mob_nick );
   scaffold_assert( NULL != o->display_name );
   scaffold_check_nonzero( bstr_res );

   o->x = x;
   o->y = y;
   o->prev_x = x;
   o->prev_y = y;

   tilemap_set_redraw_state( &(l->tilemap), TILEMAP_REDRAW_ALL );

cleanup:
   return;
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

   scaffold_print_info(
      &module, "Loading tilemap for channel: %s\n", bdata( l->name )
   );
   mapdata_filename = bstrcpy( l->name );
   scaffold_check_null( mapdata_filename );
   bdelete( mapdata_filename, 0, 1 ); /* Get rid of the # */

   mapdata_path = bstrcpy( &str_server_data_path );
   scaffold_check_null( mapdata_path );

   scaffold_join_path( mapdata_path, mapdata_filename );
   scaffold_check_nonzero( scaffold_error );

   /* TODO: Different map filename extensions. */

#ifdef USE_EZXML

   bstr_retval = bcatcstr( mapdata_path, ".tmx" );
   scaffold_check_nonzero( bstr_retval );

   scaffold_print_info(
      &module, "Loading for XML data in: %s\n", bdata( mapdata_path ) );
   bytes_read = scaffold_read_file_contents(
      mapdata_path, &mapdata_buffer, &mapdata_size );
   scaffold_check_null_msg(
      mapdata_buffer, "Unable to load tilemap data." );
   scaffold_check_zero_msg( bytes_read, "Unable to load tilemap data." );

   datafile_parse_tilemap_ezxml_string(
      &(l->tilemap), mapdata_buffer, mapdata_size, FALSE
   );
#endif /* USE_EZXML */

cleanup:
   bdestroy( mapdata_filename );
   bdestroy( mapdata_path );
   if( NULL != mapdata_buffer ) {
      scaffold_free( mapdata_buffer );
   }
   return;
}

void channel_vm_start( struct CHANNEL* l, bstring code ) {
#ifdef USE_DUKTAPE
   if( NULL == l->vm ) {
      l->vm =
         duk_create_heap( NULL, NULL, NULL, l, duktape_helper_channel_crash );
      /* duk_push_c_function( l->vm, dukt ) */
   }
   duk_peval_string( l->vm, bdata( code ) );
#endif /* USE_DUKTAPE */
#ifdef USE_TINYPY
   tp_obj tp_code_str;
   tp_obj compiled;
   tp_obj r = None;

   /* For macro compatibility. */
   tp_vm* tp = l->vm;

   /* Init the VM and load the code into it. */
   l->vm = tp_init( 0, NULL );
   tp_code_str = tp_string_n( bdata( code ), blength( code ) );
   compiled = tp_compile( l->vm, tp_code_str, tp_string( "<eval>" ) );
   tp_frame( l->vm, l->vm->builtins, (void*)compiled.string.val, &r );
   l->vm_cur = l->vm->cur;

   /* Lock the VM and prepare it to run. (tp_run) */
   if( l->vm->jmp ) {
      tp_raise( , "tp_run(%d) called recusively", l->vm->cur );
   }
   l->vm->jmp = 1;
#ifdef TINYPY_SJLJ
   if( setjmp( l->vm->buf ) ) {
      tp_handle( l->vm );
      scaffold_print_error( &module, "Error executing script for channel.\n" );
   }
#endif /* TINYPY_SJLJ */
#endif /* USE_TINYPY */
}

void channel_vm_step( struct CHANNEL* l ) {
#ifdef USE_TINYPY
   /* void tp_run(tp_vm *tp,int cur) { */
   if( l->vm->cur >= l->vm_cur && l->vm_step_ret != -1 ) {
      l->vm_step_ret = tp_step( l->vm );
   } else {
      scaffold_print_error( &module, "Channel VM stopped: %b\n", l->name );
   }
#endif /* USE_TINYPY */
}

void channel_vm_end( struct CHANNEL* l ) {
#ifdef USE_DUKTAPE
   if( NULL != l->vm ) {
      duk_destroy_heap( l->vm );
      l->vm = NULL;
   }
#endif /* USE_DUKTAPE */
#ifdef USE_TINYPY
   l->vm->cur = l->vm_cur - 1;
   l->vm->jmp = 0;
   tp_deinit( l->vm );
   l->vm = NULL;
#endif /* USE_TINYPY */
}

BOOL channel_vm_can_step( struct CHANNEL* l ) {
   BOOL retval = FALSE;

#if defined( USE_TINYPY ) || defined( USE_DUKTAPE )
   if( NULL != l->vm ) {
      retval = TRUE;
   }
#endif /* USE_TINYPY || USE_DUKTAPE */

   return retval;
}
