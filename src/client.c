
#define CLIENT_C
#include "client.h"

#include "server.h"
#include "callback.h"
#include "proto.h"
#include "input.h"
#include "tilemap.h"
#include "datafile.h"
#include "ui.h"
#include "windefs.h"
#include "channel.h"
#include "ipc.h"

bstring client_input_from_ui = NULL;

static BOOL callback_remove_items(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct ITEM* e = (struct ITEM*)iter;

   if( NULL != e ) {
      item_free( e );
   }

   return TRUE;
}

static void client_cleanup( const struct REF *ref ) {
   struct CLIENT* c =
      (struct CLIENT*)scaffold_container_of( ref, struct CLIENT, refcount );

   ipc_free( &(c->link) );

   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );

   client_stop( c );

   hashmap_cleanup( &(c->sprites) );

   vector_cleanup( &(c->delayed_files) );
   hashmap_cleanup( &(c->channels) );

   //hashmap_remove_cb( &(c->tilesets), callback_free_tilesets, NULL );
   hashmap_remove_all( &(c->tilesets) );
   hashmap_cleanup( &(c->tilesets) );

   //hashmap_remove_cb( &(c->item_catalogs), callback_free_catalogs, NULL );
   hashmap_remove_all( &(c->item_catalogs) );
   hashmap_cleanup( &(c->item_catalogs) );

   vector_remove_cb( &(c->unique_items), callback_remove_items, NULL );
   vector_cleanup( &(c->unique_items ) );

   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   /* mem_free( c ); */
}

void client_init( struct CLIENT* c ) {
   ref_init( &(c->refcount), client_cleanup );

   scaffold_assert( FALSE == c->running );

   hashmap_init( &(c->channels) );
   hashmap_init( &(c->sprites) );
   vector_init( &(c->delayed_files) );
   hashmap_init( &(c->tilesets) );
   hashmap_init( &(c->item_catalogs) );
   vector_init( &(c->unique_items) );

   c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->link = ipc_alloc();

   c->sentinal = CLIENT_SENTINAL;
   c->running = TRUE;
}

/** \brief Mark this struct as the MAIN local client.
 */
void client_set_local( struct CLIENT* c, BOOL val ) {
   c->local_client = val;
}

BOOL client_is_local( struct CLIENT* c ) {
   return c->local_client;
}

 /** \brief This should ONLY be called from server_free in order to avoid
  *         an infinite loop.
  *
  * \param c - The CLIENT struct from inside of the struct SERVER being freed.
  * \return TRUE for now.
  *
  */
BOOL client_free_from_server( struct CLIENT* c ) {
   /* Kind of a hack, but make sure "running" is set to true to fool the      *
    * client_stop() call that comes later.                                    */
   //c->running = TRUE;
   //client_cleanup( &(c->refcount) );
   return TRUE;
}

BOOL client_free( struct CLIENT* c ) {
   return refcount_dec( c, "client" );
}

void client_set_active_t( struct CLIENT* c, struct TILEMAP* t ) {
   if( NULL != c->active_tilemap ) {
      /* Take care of existing map before anything. */
      tilemap_free( c->active_tilemap );
      c->active_tilemap = NULL;
   }
   if( NULL != t ) {
      refcount_inc( t, "tilemap" ); /* Add first, to avoid deletion. */
   }
   c->active_tilemap = t; /* Assign to client last, to activate. */
}

struct CHANNEL* client_get_channel_by_name( struct CLIENT* c, const bstring name ) {
   return hashmap_get( &(c->channels), name );
}

BOOL client_connect( struct CLIENT* c, const bstring server, int port ) {
   BOOL connected = FALSE;

   scaffold_set_client();

   scaffold_print_info(
      &module, "Client connecting to: %b:%d\n", server, port
   );

   connected = ipc_connect( c->link, server , port );
   if( FALSE == connected ) {
      goto cleanup;
   }

   scaffold_print_info( &module, "Client connected and running.\n" );
   c->running = TRUE;

   scaffold_print_debug( &module, "Client sending registration...\n" );
   proto_register( c );

cleanup:

   return connected;
}

/** \brief This runs on the local client.
 * \return TRUE if a command was executed, or FALSE otherwise.
 */
BOOL client_update( struct CLIENT* c, GRAPHICS* g ) {
   BOOL retval = FALSE;
   BOOL keep_going = FALSE;
   struct VECTOR* chunker_removal_queue = NULL;

   scaffold_set_client();

   /* Check for commands from the server. */
   keep_going = proto_dispatch( c, NULL );

   /* TODO: Is this ever called? */
   if( FALSE == keep_going ) {
      scaffold_print_info( &module, "Remote server disconnected.\n" );
      client_stop( c );
   }

   vector_remove_cb( &(c->delayed_files), callback_proc_client_delayed_files, c );

   return retval;
}

void client_free_channels( struct CLIENT* c ) {
   hashmap_remove_all( &(c->channels) );
   assert( 0 == hashmap_count( &(c->channels) ) );
}

void client_stop( struct CLIENT* c ) {
   bstring buffer = NULL;

#ifdef ENABLE_LOCAL_CLIENT

   if( TRUE == client_is_local( c ) ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }

   c->flags = 0;

#endif /* ENABLE_LOCAL_CLIENT */

   client_clear_puppet( c );

   buffer = bfromcstralloc( CLIENT_BUFFER_ALLOC, "" );
   scaffold_check_null( buffer );

   client_free_channels( c );

   /* Empty receiving buffer. */
   proto_empty_buffer( c );

   client_clear_puppet( c );
   client_set_active_t( c, NULL );
#ifdef ENABLE_LOCAL_CLIENT
   if( TRUE == client_is_local( c ) ) {
      client_local_free( c );
      hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
      hashmap_remove_all( &(c->tilesets) );
      hashmap_remove_all( &(c->item_catalogs) );
      vector_remove_all( &(c->unique_items) );
   }
#endif /* ENABLE_LOCAL_CLIENT */

   scaffold_assert( FALSE == ipc_connected( c->link ) );
   c->running = FALSE;

cleanup:
   bdestroy( buffer );
   return;
}

short client_add_channel( struct CLIENT* c, struct CHANNEL* l ) {
   if( hashmap_put( &(c->channels), l->name, l, FALSE ) ) {
      scaffold_print_error( &module, "Attempted to double-add channel...\n" );
      return 1;
   }
   return 0;
}

void client_set_puppet( struct CLIENT* c, struct MOBILE* o ) {
   if( NULL != c->puppet ) { /* Take care of existing mob before anything. */
      mobile_free( c->puppet );
      c->puppet = NULL;
   }
   if( NULL != o ) {
      refcount_inc( o, "mobile" ); /* Add first, to avoid deletion. */
      if( NULL != o->owner ) {
         client_clear_puppet( o->owner );
      }
      o->owner = c;
   }
   c->puppet = o; /* Assign to client last, to activate. */
}

void client_clear_puppet( struct CLIENT* c ) {
   client_set_puppet( c, NULL );
}

static void* client_dr_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   bstring filename = (bstring)arg;
   struct CLIENT_DELAYED_REQUEST* req = (struct CLIENT_DELAYED_REQUEST*)iter;

   if( NULL == req || 0 == bstrcmp( filename, req->filename ) ) {
      return req;
   }

   return NULL;
}

#ifdef ENABLE_LOCAL_CLIENT

/** \brief
 * \param
 * \param
 * \return TRUE if input is consumed, or FALSE otherwise.
 */
BOOL client_poll_ui(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
) {
   struct TILEMAP* t = NULL;
   int bstr_ret;
   BOOL retval = FALSE;

   if( NULL != c && NULL != c->puppet && NULL != c->puppet->channel ) {
      t = c->puppet->channel->tilemap;
   }

   /* Poll window: Chat */
   if( NULL != ui_window_by_id( c->ui, &str_client_window_id_chat ) ) {
      retval = TRUE; /* Whatever the window does, it consumes input. */
      if( 0 != ui_poll_input(
         c->ui, p, &str_client_window_id_chat
      ) ) {
         tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );

         /* Process collected input. */

         proto_send_msg_channel( c, l, client_input_from_ui );
         mobile_speak( c->puppet, client_input_from_ui );

         goto reset_buffer;
      }
      goto cleanup;
   }

   /* Poll window: Chat */
   if( NULL != ui_window_by_id( c->ui, &str_client_window_id_inv ) ) {
      retval = TRUE; /* Whatever the window does, it consumes input. */
      if( 0 != ui_poll_input(
         c->ui, p, &str_client_window_id_inv
      ) ) {
         tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );

         /* Process collected input. */
         /* TODO: ?

         proto_send_msg_channel( c, l, client_input_from_ui );
         mobile_speak( c->puppet, client_input_from_ui );

         goto reset_buffer;
         */
      }
      goto cleanup;
   }

cleanup:
   return retval;

reset_buffer:
   bstr_ret = btrunc( client_input_from_ui, 0 );
   scaffold_check_nonzero( bstr_ret );
   return retval;
}

static void client_request_file_local(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
   /* FIXME: No file receiving method implemented! */
   BYTE* data = NULL;
   SCAFFOLD_SIZE length;
   bstring filepath = NULL;
   SCAFFOLD_SIZE bytes_read;

   filepath = files_root( filename );

   scaffold_print_debug( &module, "Loading local resource: %b\n", filepath );

   bytes_read = files_read_contents( filepath, &data, &length );
   scaffold_assert( 0 < bytes_read );
   scaffold_check_null_msg( data, "Unable to load resource data." );
   scaffold_check_zero( bytes_read, "Resource is empty." );
   datafile_handle_stream( type, filename, data, length, c );

cleanup:
   return;
}

void client_request_file_later(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
   struct CLIENT_DELAYED_REQUEST* request = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   /* Make sure request wasn't made already. */
   request = vector_iterate( &(c->delayed_files), client_dr_cb, filename );
   if( NULL != request ) {
      goto cleanup; /* Silently. */
   }

   /* Create a new delayed request. */
   request = mem_alloc( 1, struct CLIENT_DELAYED_REQUEST );
   scaffold_check_null( request );
   scaffold_check_null( request );

   request->filename = bstrcpy( filename );
   request->type = type;

   verr = vector_add( &(c->delayed_files), request );
   scaffold_check_negative( verr );

cleanup:
   return;
}

void client_request_file(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
   /* FIXME: No file receiving method implemented! */
   client_request_file_local( c, type, filename );

cleanup:
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

void client_set_names(
   struct CLIENT* c, bstring nick, bstring uname, bstring rname
) {
   int bstr_result;
   /* TODO: Handle this if we're connected. */
   scaffold_assert( !client_connected( c ) );
   bstr_result = bassign( c->nick, nick );
   scaffold_assert( BSTR_ERR != bstr_result );
   bstr_result = bassign( c->realname, uname );
   scaffold_assert( BSTR_ERR != bstr_result );
   bstr_result = bassign( c->username, rname );
   scaffold_assert( BSTR_ERR != bstr_result );
   return;
}

struct MOBILE* client_get_puppet( struct CLIENT* c ) {
   if( NULL == c ) {
      return NULL;
   }
   return c->puppet;
}

struct ITEM* client_get_item( struct CLIENT* c, SCAFFOLD_SIZE serial ) {
   return vector_get( &(c->unique_items), serial );
}

struct ITEM_SPRITESHEET* client_get_catalog(
   struct CLIENT* c, const bstring name
) {
   return hashmap_get( &(c->item_catalogs), name );
}

void client_set_item( struct CLIENT* c, SCAFFOLD_SIZE serial, struct ITEM* e ) {
   struct ITEM* c_e = NULL;
   int retval = 0;

   c_e = vector_get( &(c->unique_items), serial );

   if( c_e == e ) {
      goto cleanup;
   } else if( NULL != c_e ) {
      c_e->count = e->count;
      retval = bassign( c_e->catalog_name, e->catalog_name );
      scaffold_check_nonzero( retval );
      scaffold_assign_or_cpy_c( c_e->display_name, bdata( e->display_name ), retval );
      c_e->sprite_id = e->sprite_id;

      /* Don't overwrite contents. */

      item_free( e );
   } else {
      vector_set( &(c->unique_items), serial, e, TRUE );
   }

cleanup:
   return;
}

GRAPHICS* client_get_screen( struct CLIENT* c ) {
   scaffold_assert_client();

   return c->ui->screen_g;
}
