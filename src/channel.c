
#define CHANNEL_C
#include "channel.h"

#include "ui.h"
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
#endif /* USE_DUKTAPE */

extern struct tagbstring str_backlog_id;
extern struct CLIENT* main_client;

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
   /* FIXME: Things still holding open channel refs. */
   refcount_dec( l, "channel" );
}

/**
 * \param[in] server The server (or local client, if this is a local client
 *                   mirror of a server-side channel.)
 */
void channel_init(
   struct CHANNEL* l, const bstring name, BOOL local_images,
   struct CLIENT* server
) {
   ref_init( &(l->refcount), channel_free_final );
   hashmap_init( &(l->clients) );
   vector_init( &(l->mobiles ) );
   vector_init( &(l->speech_backlog ) );
   l->name = bstrcpy( name );
   l->topic = bfromcstr( "No topic" );
   scaffold_check_null( l->name );
   scaffold_check_null( l->topic );
   tilemap_init( &(l->tilemap), local_images, server );
cleanup:
   return;
}

void channel_add_client( struct CHANNEL* l, struct CLIENT* c, BOOL spawn ) {
   struct MOBILE* o = NULL;
   struct TILEMAP* t = NULL;
   struct TILEMAP_SPAWNER* spawner = NULL;
   SCAFFOLD_SIZE bytes_read = 0;
   struct VECTOR* player_spawns = NULL;
   SCAFFOLD_SIZE v_count = 0;

   scaffold_check_null( c );

   if( NULL != channel_get_client_by_name( l, c->nick ) ) {
      goto cleanup;
   }

   if( TRUE == spawn ) {
      scaffold_assert_server();
      t = &(l->tilemap);

      /* Make a list of player-capable spawns and pick one at pseudo-random. */
      player_spawns = vector_iterate_v(
         &(t->spawners), callback_search_spawners, &str_player
      );
      scaffold_check_null_msg( player_spawns, "No player spawns available." );
      spawner =
         vector_get( player_spawns, rand() % vector_count( player_spawns ) );
   }

   if( NULL != spawner ) {
      scaffold_print_debug(
         &module, "Creating player mobile for client: %d\n", c->link.socket
      );

      /* Create a basic mobile for the new client. */
      /* TODO: Get the desired mobile data ID from client. */
      mobile_new(
         o, (const bstring)&str_mobile_def_id_default,
         spawner->pos.x, spawner->pos.y
      );
      mobile_load_local( o );

      scaffold_gen_serial( o, &(l->mobiles) );

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
   assert( 0 != o->serial );
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
   const char* nick_c = NULL,
      * lname_c = NULL;

   nick_c = bdata( mob_nick );
   lname_c = bdata( l->name );
   scaffold_assert( NULL != nick_c );
   scaffold_assert( NULL != lname_c );

   scaffold_print_debug(
      &module, "Adding player mobile to channel: %b (%d)\n", mob_nick, serial
   );

   scaffold_assert( 0 != serial );

   o = vector_get( &(l->mobiles), serial );
   if( NULL == o ) {
      mobile_new( o, mob_id, x, y );
      o->serial = serial;
      scaffold_print_debug(
         &module, "Player mobile does not exist. Creating with serial: %d\n",
         o->serial
      );
      scaffold_assert( NULL != o->def_filename );
      mobile_set_channel( o, l );
      vector_set( &(l->mobiles), o->serial, o, TRUE );
      if( NULL != local_c && TRUE == local_c->client_side ) {
         client_request_file(
            local_c, CHUNKER_DATA_TYPE_MOBDEF, o->def_filename
         );
      }
   } else {
      scaffold_print_debug(
         &module, "Resetting position for existing player mobile: %d, %d\n",
         x, y
      );
      o->x = x;
      o->y = y;
      o->prev_x = x;
      o->prev_y = y;
   }

   scaffold_assert( 0 < hashmap_count( &(l->clients) ) );
   mobile_c = channel_get_client_by_name( l, mob_nick );
   if( NULL != mobile_c && 0 == bstrcmp( mobile_c->nick, mob_nick ) ) {
      client_set_puppet( mobile_c, o );
   }

   bstr_res = bassign( o->display_name, mob_nick );
   scaffold_assert( NULL != o->display_name );
   scaffold_check_nonzero( bstr_res );

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
      &module, "Loading tilemap XML data from: %s\n", bdata( mapdata_path ) );
   bytes_read = scaffold_read_file_contents(
      mapdata_path, &mapdata_buffer, &mapdata_size );
   scaffold_check_null_msg(
      mapdata_buffer, "Unable to load tilemap data." );
   scaffold_check_zero_msg( bytes_read, "Unable to load tilemap data." );

   datafile_parse_ezxml_string(
      &(l->tilemap), mapdata_buffer, mapdata_size, FALSE,
      DATAFILE_TYPE_TILEMAP, mapdata_path
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

void channel_speak( struct CHANNEL* l, const bstring nick, const bstring msg ) {
   struct CHANNEL_BUFFER_LINE* line = NULL;
   time_t time_now;
   struct tm* time_temp = NULL;
   struct UI_WINDOW* bl = NULL;

   line = (struct CHANNEL_BUFFER_LINE*)calloc(
      1, sizeof( struct CHANNEL_BUFFER_LINE)
   );
   scaffold_check_null( line );

   line->nick = bstrcpy( nick );
   line->line = bstrcpy( msg );

   /* Grab the time in a platform-agnostic way. */
   time( &time_now );
   time_temp = localtime( &time_now );
   memcpy( &(line->time), time_temp, sizeof( struct tm ) );

   vector_insert( &(l->speech_backlog), 0, line );

   /* TODO: A more elegant method of marking the backlog window dirty. */
   if( NULL != main_client ) {
      bl = ui_window_by_id( main_client->ui, &str_backlog_id );
      if( NULL != bl ) {
         bl->dirty = TRUE;
      }
   }

cleanup:
   return;
}

void* channel_backlog_iter( struct CHANNEL* l, vector_search_cb cb, void* arg ) {
   return vector_iterate( &(l->speech_backlog), cb, arg );
}

void channel_vm_start( struct CHANNEL* l, bstring code ) {
#ifdef USE_DUKTAPEx
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
#ifdef USE_DUKTAPEx
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
