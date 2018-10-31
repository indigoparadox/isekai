
#define CLIENT_C
#include "client.h"

#include "server.h"
#include "callback.h"
#include "proto.h"
#include "chunker.h"
#include "input.h"
#include "tilemap.h"
#include "datafile.h"
#include "ui.h"
#include "tinypy/tinypy.h"
#include "windefs.h"
#include "channel.h"
#include "ipc.h"
#include "plugin.h"
#include "files.h"

extern struct VECTOR mode_list_short;

bstring client_input_from_ui = NULL;

#ifdef USE_CHUNKS
static BOOL callback_remove_chunkers( size_t idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct CHUNKER* h = (struct CHUNKER*)iter;

   client_handle_finished_chunker( c, h );

   return TRUE;
}
#endif /* USE_CHUNKS */

static BOOL callback_remove_items( size_t idx, void* iter, void* arg ) {
   struct ITEM* e = (struct ITEM*)iter;

   if( NULL != e ) {
      item_free( e );
   }

   return TRUE;
}

static void client_cleanup( const struct REF *ref ) {
   struct CLIENT* c =
      (struct CLIENT*)scaffold_container_of( ref, struct CLIENT, refcount );

   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );

   client_stop( c );

   hashmap_cleanup( &(c->sprites) );

#ifdef USE_CHUNKS
   hashmap_cleanup( &(c->chunkers) );
#endif /* USE_CHUNKS */
   vector_cleanup( &(c->delayed_files) );
   hashmap_cleanup( &(c->channels) );

   hashmap_remove_all( &(c->tilesets) );
   hashmap_cleanup( &(c->tilesets) );

   hashmap_remove_all( &(c->item_catalogs) );
   hashmap_cleanup( &(c->item_catalogs) );

   vector_remove_cb( &(c->unique_items), callback_remove_items, NULL );
   vector_cleanup( &(c->unique_items ) );

   ipc_free( &(c->link) );

   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   /* mem_free( c ); */
}

void client_init( struct CLIENT* c ) {
   ref_init( &(c->refcount), client_cleanup );

   scaffold_assert( FALSE == c->running );

   hashmap_init( &(c->channels) );
   hashmap_init( &(c->sprites) );
#ifdef USE_CHUNKS
   hashmap_init( &(c->chunkers) );
#endif /* USE_CHUNKS */
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

   lg_info(
      __FILE__, "Client connecting to: %b:%d\n", server, port
   );

   connected = ipc_connect( c->link, server , port );
   if( FALSE == connected ) {
      goto cleanup;
   }

   lg_info( __FILE__, "Client connected and running.\n" );
   c->running = TRUE;

   lg_debug( __FILE__, "Client sending registration...\n" );
   proto_register( c );

cleanup:

   return connected;
}

/** \brief This runs on the local client.
 * \return TRUE if a command was executed, or FALSE otherwise.
 */
BOOL client_update( struct CLIENT* c, GRAPHICS* g ) {
   BOOL retval = FALSE;
#ifdef DEBUG_TILES
   int bstr_ret;
   SCAFFOLD_SIZE steps_remaining_x,
      steps_remaining_y;
   static bstring pos = NULL;
   struct MOBILE* o = NULL;
#endif /* DEBUG_TILES */
   BOOL keep_going = FALSE;
   struct VECTOR* chunker_removal_queue = NULL;

   scaffold_set_client();

   /* Check for commands from the server. */
   keep_going = proto_dispatch( c, NULL );

   /* TODO: Is this ever called? */
   if( FALSE == keep_going ) {
      lg_info( __FILE__, "Remote server disconnected.\n" );
      client_stop( c );
   }

#ifdef DEBUG_TILES
   o = client_get_puppet( c );

   if( NULL == pos ) {
      pos = bfromcstr( "" );
   }

   if( NULL != o ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, FALSE );
      steps_remaining_y = mobile_get_steps_remaining_y( o, FALSE );

      bstr_ret = bassignformat( pos,
         "Player: %d (%d)[%d], %d (%d)[%d]",
         o->x, o->prev_x, steps_remaining_x,
         o->y, o->prev_y, steps_remaining_y
      );
      lgc_nonzero( bstr_ret );
      ui_debug_window( c->ui, &str_wid_debug_tiles_pos, pos );
   }

#ifdef USE_CHUNKS
   /* Deal with chunkers that will never receive blocks that are finished via
    * their cache.
    */
   chunker_removal_queue = hashmap_iterate_v( &(c->chunkers), callback_proc_client_chunkers, c );
   if( NULL != chunker_removal_queue ) {
      vector_remove_cb( chunker_removal_queue, callback_remove_chunkers, c );
      vector_free( &chunker_removal_queue );
   }
#endif /* USE_CHUNKS */

cleanup:
#endif /* DEBUG_TILES */

   vector_remove_cb( &(c->delayed_files), callback_proc_client_delayed_files, c );

   return retval;
}

void client_free_channels( struct CLIENT* c ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;

   deleted =
#endif /* DEBUG */
      hashmap_remove_all( &(c->channels) );
      //hashmap_remove_cb( &(c->channels), callback_free_channels, NULL );
#ifdef DEBUG
   lg_debug(
      __FILE__,
      "Removed %d channels. %d remaining.\n",
      deleted, hashmap_count( &(c->channels) )
   );
#endif /* DEBUG */
   scaffold_assert( 0 == hashmap_count( &(c->channels) ) );
}

#ifdef USE_CHUNKS
void client_free_chunkers( struct CLIENT* c ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;

   deleted =
#endif /* DEBUG */
      hashmap_remove_all( &(c->chunkers) );
      //hashmap_remove_cb( &(c->chunkers), callback_free_chunkers, NULL );
#ifdef DEBUG
   lg_debug(
      __FILE__,
      "Removed %d chunkers. %d remaining.\n",
      deleted, hashmap_count( &(c->chunkers) )
   );
   scaffold_assert( 0 == hashmap_count( &(c->chunkers) ) );
#endif /* DEBUG */
}
#endif /* USE_CHUNKS */

void client_stop( struct CLIENT* c ) {
   bstring buffer = NULL;
#ifdef DEBUG
   SCAFFOLD_SIZE test_count = 0;

   scaffold_assert( CLIENT_SENTINAL == c->sentinal );

   if( FALSE != ipc_connected( c->link ) ) {
      lg_info( __FILE__, "Client connection stopping...\n" );
      ipc_stop( c->link );
   }

#ifdef ENABLE_LOCAL_CLIENT

   if( FALSE != client_is_local( c ) ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }

   c->flags = 0;

#endif /* ENABLE_LOCAL_CLIENT */

#endif /* DEBUG */

   client_clear_puppet( c );

   buffer = bfromcstralloc( CLIENT_BUFFER_ALLOC, "" );
   lgc_null( buffer );

   client_free_channels( c );
#ifdef USE_CHUNKS
   client_free_chunkers( c );
#endif /* USE_CHUNKS */

   /* Empty receiving buffer. */
   proto_empty_buffer( c );

   client_clear_puppet( c );
   client_set_active_t( c, NULL );
#ifdef ENABLE_LOCAL_CLIENT
   if( FALSE != client_is_local( c ) ) {
      plugin_call( PLUGIN_MODE, vector_get( &mode_list_short, c->gfx_mode ), PLUGIN_FREE, c );
      hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
      hashmap_remove_all( &(c->tilesets) );
      hashmap_remove_all( &(c->item_catalogs) );
      vector_remove_all( &(c->unique_items) );
   }
#endif /* ENABLE_LOCAL_CLIENT */

   scaffold_assert( FALSE == ipc_connected( c->link ) );
   c->running = FALSE;

#ifdef DEBUG

   if( FALSE != client_is_local( c ) ) {
      scaffold_assert( FALSE == c->running );

      test_count = hashmap_count( &(c->channels) );
      scaffold_assert( 0 == test_count );

      test_count = hashmap_count( &(c->sprites) );
      scaffold_assert( 0 == test_count );

#ifdef USE_CHUNKS
      test_count = hashmap_count( &(c->chunkers) );
      scaffold_assert( 0 == test_count );
#endif /* USE_CHUNKS */

      test_count = vector_count( &(c->delayed_files) );
      scaffold_assert( 0 == test_count );

      test_count = hashmap_count( &(c->tilesets) );
      scaffold_assert( 0 == test_count );

      test_count = hashmap_count( &(c->item_catalogs) );
      scaffold_assert( 0 == test_count );

      test_count = vector_count( &(c->unique_items) );
      scaffold_assert( 0 == test_count );
   }
#endif // DEBUG

cleanup:
   bdestroy( buffer );
   return;
}

short client_add_channel( struct CLIENT* c, struct CHANNEL* l ) {
   if( hashmap_put( &(c->channels), l->name, l, FALSE ) ) {
      lg_error( __FILE__, "Attempted to double-add channel...\n" );
      return 1;
   }
   return 0;
}

#ifdef USE_CHUNKS

void client_send_file(
   struct CLIENT* c, DATAFILE_TYPE type,
   const bstring serverpath, const bstring filepath
) {
   struct CHUNKER* h = NULL;
   BOOL valid_file;

   lg_debug(
      __FILE__, "Sending file to client %p: %s\n", c, bdata( filepath )
   );

   /* Begin transmitting tilemap. */
   h = mem_alloc( 1, struct CHUNKER );
   lgc_null( h );

   valid_file = chunker_chunk_start_file(
      h,
      type,
      serverpath,
      filepath,
      64
   );

   if( FALSE == valid_file ) {
      lg_error(
         __FILE__, "Server: File not found, canceling: %b\n", filepath
      );
      proto_send_chunk( c, NULL, 0, filepath, NULL );
   }

   lg_debug(
      __FILE__, "Server: Adding chunker to send: %b\n", filepath
   );
   hashmap_put( &(c->chunkers), filepath, h, TRUE );

cleanup:
   if( NULL != h ) {
      //chunker_free( h );
   }
   return;
}

#endif /* USE_CHUNKS */

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

static void* client_dr_cb( size_t idx, void* iter, void* arg ) {
   bstring filename = (bstring)arg;
   struct CLIENT_DELAYED_REQUEST* req = (struct CLIENT_DELAYED_REQUEST*)iter;

   if( NULL == req || 0 == bstrcmp( filename, req->filename ) ) {
      return req;
   }

   return NULL;
}

#ifndef USE_CHUNKS

static void client_request_file_local(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
   /* FIXME: No file receiving method implemented! */
   BYTE* data = NULL;
   size_t length;
   bstring filepath = NULL;
   size_t bytes_read;

   filepath = files_root( filename );

   lg_debug( __FILE__, "Loading local resource: %b\n", filepath );

   bytes_read = files_read_contents( filepath, &data, &length );
   scaffold_assert( 0 < bytes_read );
   lgc_null_msg( data, "Unable to load resource data." );
   lgc_zero( bytes_read, "Resource is empty." );
   datafile_handle_stream( type, filename, data, length, c );

cleanup:
   return;
}

#endif /* USE_CHUNKS */

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
   lgc_null( request );

   request->filename = bstrcpy( filename );
   request->type = type;

   verr = vector_add( &(c->delayed_files), request );
   lgc_negative( verr );

cleanup:
   return;
}

void client_request_file(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
#ifdef USE_CHUNKS
   struct CHUNKER* h = NULL;

   if( FALSE != hashmap_contains_key( &(c->chunkers), filename ) ) {
      /* File already requested, so just be patient. */
      goto cleanup;
   }

   h = hashmap_get( &(c->chunkers), filename );
   if( NULL == h ) {
      /* Create a chunker and get it started, since one is not in progress. */
      /* TODO: Verify cached file hash from server. */
      h = mem_alloc( 1, struct CHUNKER );
      lg_debug( __FILE__, "Adding unchunker to receive: %b\n", filename );
      lg_debug( __FILE__, "File cache path is: %b\n", &str_client_cache_path );
      chunker_unchunk_start(
         h, type, filename, &str_client_cache_path
      );
      hashmap_put( &(c->chunkers), filename, h, TRUE );
      lg_debug( __FILE__, "scaffold_error: %d\n", lgc_error );
      lgc_nonzero( lgc_error );

      if( !chunker_unchunk_finished( h ) ) {
         /* File not in cache. */
         proto_request_file( c, filename, type );
      }
   }

cleanup:
#else
   /* FIXME: No file receiving method implemented! */
   client_request_file_local( c, type, filename );

cleanup:
#endif /* USE_CHUNKS */
   return;
}

#ifdef ENABLE_LOCAL_CLIENT

#ifdef USE_CHUNKS

void client_process_chunk( struct CLIENT* c, struct CHUNKER_PROGRESS* cp ) {
   struct CHUNKER* h = NULL;
   int8_t chunker_percent;
   BOOL remove_ok;

   scaffold_assert( 0 < blength( cp->filename ) );

   if( cp->current > cp->total ) {
      lg_error(
         __FILE__, "Invalid progress for %s.\n", bdata( cp->filename )
      );
      //lgc_error = LGC_ERROR_MISC;
      goto cleanup;
   }

   h = hashmap_get( &(c->chunkers), cp->filename );
   if( NULL == h ) {
      lg_error(
         __FILE__,
         "Client: Invalid data block received (I didn't ask for this?): %s\n",
         bdata( cp->filename )
      );
      //scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   if( DATAFILE_TYPE_INVALID == cp->type ) {
      scaffold_assert( NULL != h );
      lg_debug(
         __FILE__, "Removing invalid chunker: %b\n", h->filename );
      remove_ok = hashmap_remove( &(c->chunkers), cp->filename );
      chunker_free( h );
      scaffold_assert( FALSE != remove_ok );
      goto cleanup;
   }

   scaffold_assert( 0 < blength( cp->data ) );

   chunker_unchunk_pass( h, cp->data, cp->current, cp->total, cp->chunk_size );

   chunker_percent = chunker_unchunk_percent_progress( h, FALSE );
   if( 0 < chunker_percent ) {
      lg_debug(
         __FILE__, "Chunker: %s: %d%%\n", bdata( h->filename ), chunker_percent );
   }

   if( chunker_unchunk_finished( h ) ) {
      /* Cached file found, so abort transfer. */
      if( chunker_unchunk_cached( h ) ) {
         proto_abort_chunker( c, h );
      }

      client_handle_finished_chunker( c, h );
   }

cleanup:
   return;
}

void client_handle_finished_chunker( struct CLIENT* c, struct CHUNKER* h ) {

   assert( TRUE == chunker_unchunk_finished( h ) );

   datafile_handle_stream( h->type, h->filename, h->raw_ptr, h->raw_length, c );

/* cleanup: */
   lg_debug(
      __FILE__,
      "Removing finished chunker for: %s\n", bdata( h->filename )
   );
   /* Hashmap no longer adds a ref. */
   hashmap_remove( &(c->chunkers), h->filename );
   chunker_free( h );
   return;
}

#endif /* USE_CHUNKS */

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

#ifdef DEBUG_VM
   /* Poll window: REPL */
   if( NULL != ui_window_by_id( c->ui, &str_client_window_id_repl ) ) {
      retval = TRUE; /* Whatever the window does, it consumes input. */
      if( 0 != ui_poll_input(
         c->ui, p, &str_client_window_id_repl
      ) ) {
         ui_window_pop( c->ui );
         tilemap_set_redraw_state( t, TILEMAP_REDRAW_ALL );

         /* Process collected input. */

         proto_client_debug_vm( c, l, client_input_from_ui );

         goto reset_buffer;
      }
      goto cleanup;
   } else
#endif /* DEBUG_VM */

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
   lgc_nonzero( bstr_ret );
   return retval;
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
      lgc_nonzero( retval );
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

struct TILEMAP* client_get_tilemap( struct CLIENT* c ) {
   return c->active_tilemap;
}
