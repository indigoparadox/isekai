
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

struct CLIENT_DELAYED_REQUEST {
   bstring filename;
   DATAFILE_TYPE type;
};

#include "clistruct.h"

extern struct VECTOR mode_list_short;

bstring client_input_from_ui = NULL;

VBOOL callback_proc_client_delayed_files(
   size_t idx, void* iter, void* arg
) {
   struct CLIENT_DELAYED_REQUEST* req = (struct CLIENT_DELAYED_REQUEST*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   lg_debug(
      __FILE__, "Processing delayed request: %b\n", req->filename
   );
   client_request_file( c, req->type, req->filename );

   bdestroy( req->filename );
   mem_free( req );
   return VTRUE;
}

#ifdef USE_CHUNKS
static VBOOL callback_remove_chunkers( size_t idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct CHUNKER* h = (struct CHUNKER*)iter;

   client_handle_finished_chunker( c, h );

   return VTRUE;
}

void* callback_send_chunkers_l( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)arg;
   struct CHUNKER* h = (struct CHUNKER*)iter;
   bstring chunk_out = NULL;
   size_t start_pos = 0;

   if( chunker_chunk_finished( h ) ) {
      goto cleanup;
   }

   chunk_out = bfromcstralloc( CHUNKER_DEFAULT_CHUNK_SIZE, "" );
   start_pos = chunker_chunk_pass( h, chunk_out );
   proto_send_chunk( c, h, start_pos, idx, chunk_out );

cleanup:
   bdestroy( chunk_out );
   return NULL;
}

void* callback_proc_chunkers( bstring idx, void* iter, void* arg ) {
   struct CLIENT* c = (struct CLIENT*)iter;

   /* Process some compression chunks. */
   hashmap_iterate_v( c->chunkers, callback_send_chunkers_l, c );

   /* Removed any finished chunkers. */
   hashmap_remove_cb( c->chunkers, callback_free_finished_chunkers, NULL );

   return NULL;
}

#endif /* USE_CHUNKS */

#ifdef USE_ITEMS

static VBOOL callback_remove_items( size_t idx, void* iter, void* arg ) {
   struct ITEM* e = (struct ITEM*)iter;

   if( NULL != e ) {
      item_free( e );
   }

   return VTRUE;
}

#endif /* USE_ITEMS */

static void client_cleanup( const struct REF *ref ) {
   struct CLIENT* c =
      (struct CLIENT*)scaffold_container_of( ref, struct CLIENT, refcount );

   bdestroy( client_get_nick( c ) );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );

   client_stop( c );

   hashmap_free( &(c->sprites) );

#ifdef USE_CHUNKS
   hashmap_free( &(c->chunkers) );
#endif /* USE_CHUNKS */
   vector_free( &(c->delayed_files) );
   hashmap_free( &(c->channels) );

   hashmap_remove_all( c->tilesets );
   hashmap_free( &(c->tilesets) );

   /* XXX: Free all sub-hashmaps. */
   hashmap_free( &(c->mode_data) );

#ifdef USE_ITEMS

   hashmap_remove_all( &(c->item_catalogs) );
   hashmap_free( &(c->item_catalogs) );

   vector_remove_cb( &(c->unique_items), callback_remove_items, NULL );
   vector_cleanup( &(c->unique_items ) );

#endif /* USE_ITEMS */

   ipc_free( &(c->link) );

   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   /* mem_free( c ); */
}

struct CLIENT* client_new() {
   struct CLIENT* c = NULL;
   c = mem_alloc( 1, struct CLIENT );
   lgc_null( c );
   client_init( c );
cleanup:
   return c;
}

void client_init( struct CLIENT* c ) {
   ref_init( &(c->refcount), client_cleanup );

   scaffold_assert( VFALSE == c->running );

   c->channels = hashmap_new();
   c->sprites = hashmap_new();
#ifdef USE_CHUNKS
   c->chunkers = hashmap_new();
#endif /* USE_CHUNKS */
   c->delayed_files = vector_new();
   c->tilesets = hashmap_new();
#ifdef USE_ITEMS
   hashmap_init( &(c->item_catalogs) );
   vector_init( &(c->unique_items) );
#endif // USE_ITEMS

   c->nick = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->realname = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->remote = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->username = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   c->link = ipc_alloc();
   c->protocol_data = NULL;
   c->mode_data = hashmap_new();

   c->sentinal = CLIENT_SENTINAL;
   c->running = VTRUE;
}

/** \brief Mark this struct as the MAIN local client.
 */
void client_set_local( struct CLIENT* c, VBOOL val ) {
   c->local_client = val;
}

VBOOL client_is_local( struct CLIENT* c ) {
   return c->local_client;
}

 /** \brief This should ONLY be called from server_free in order to avoid
  *         an infinite loop.
  *
  * \param c - The CLIENT struct from inside of the struct SERVER being freed.
  * \return VTRUE for now.
  *
  */
VBOOL client_free_from_server( struct CLIENT* c ) {
   /* Kind of a hack, but make sure "running" is set to true to fool the      *
    * client_stop() call that comes later.                                    */
   //c->running = VTRUE;
   //client_cleanup( &(c->refcount) );
   return VTRUE;
}

VBOOL client_free( struct CLIENT* c ) {
   return refcount_dec( c, "client" );
}

#if 0
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
#endif // 0

struct CHANNEL* client_get_channel_by_name( struct CLIENT* c, const bstring name ) {
   return hashmap_get( c->channels, name );
}

VBOOL client_connect( struct CLIENT* c, const bstring server, int port ) {
   VBOOL connected = VFALSE;

   scaffold_set_client();

   lg_info(
      __FILE__, "Client connecting to: %b:%d\n", server, port
   );

   connected = ipc_connect( c->link, server , port );
   if( VFALSE == connected ) {
      goto cleanup;
   }

   lg_info( __FILE__, "Client connected and running.\n" );
   c->running = VTRUE;

   lg_debug( __FILE__, "Client sending registration...\n" );
   proto_register( c );

cleanup:

   return connected;
}

/** \brief This runs on the local client.
 * \return VTRUE if a command was executed, or VFALSE otherwise.
 */
VBOOL client_update( struct CLIENT* c, GRAPHICS* g ) {
   VBOOL retval = VFALSE;
#ifdef DEBUG_TILES
   int bstr_ret;
   SCAFFOLD_SIZE steps_remaining_x,
      steps_remaining_y;
   static bstring pos = NULL;
   struct MOBILE* o = NULL;
#endif /* DEBUG_TILES */
   VBOOL keep_going = VFALSE;
   struct VECTOR* chunker_removal_queue = NULL;
   struct TWINDOW* twindow = NULL;

   scaffold_set_client();

   /* Check for commands from the server. */
   keep_going = proto_dispatch( c, NULL );

   /* TODO: Is this ever called? */
   if( VFALSE == keep_going ) {
      lg_info( __FILE__, "Remote server disconnected.\n" );
      client_stop( c );
   }

#ifdef DEBUG_TILES
   o = client_get_puppet( c );

   if( NULL == pos ) {
      pos = bfromcstr( "" );
   }

   if( NULL != o ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, VFALSE );
      steps_remaining_y = mobile_get_steps_remaining_y( o, VFALSE );

      bstr_ret = bassignformat( pos,
         "Player: %d (%d)[%d], %d (%d)[%d]",
         mobile_get_x( o ), mobile_get_prev_x( o ), steps_remaining_x,
         mobile_get_y( o ), mobile_get_prev_y( o ), steps_remaining_y
      );
      lgc_nonzero( bstr_ret );
      twindow = client_get_local_window( c );
      ui_debug_window( twindow_get_ui( twindow ), &str_wid_debug_tiles_pos, pos );
   }

#ifdef USE_CHUNKS
   /* Deal with chunkers that will never receive blocks that are finished via
    * their cache.
    */
   chunker_removal_queue =
      hashmap_iterate_v( c->chunkers, callback_proc_client_chunkers, c );
   if( NULL != chunker_removal_queue ) {
      vector_remove_cb( chunker_removal_queue, callback_remove_chunkers, c );
      vector_free( &chunker_removal_queue );
   }
#endif /* USE_CHUNKS */

cleanup:
#endif /* DEBUG_TILES */

   vector_remove_cb( c->delayed_files, callback_proc_client_delayed_files, c );

   return retval;
}

void client_free_channels( struct CLIENT* c ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;

   deleted =
#endif /* DEBUG */
      hashmap_remove_all( c->channels );
      //hashmap_remove_cb( &(c->channels), callback_free_channels, NULL );
#ifdef DEBUG
   lg_debug(
      __FILE__,
      "Removed %d channels. %d remaining.\n",
      deleted, hashmap_count( c->channels )
   );
#endif /* DEBUG */
   scaffold_assert( 0 == hashmap_count( c->channels ) );
}

#ifdef USE_CHUNKS
void client_free_chunkers( struct CLIENT* c ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;

   deleted =
#endif /* DEBUG */
      hashmap_remove_all( c->chunkers );
      //hashmap_remove_cb( &(c->chunkers), callback_free_chunkers, NULL );
#ifdef DEBUG
   lg_debug(
      __FILE__,
      "Removed %d chunkers. %d remaining.\n",
      deleted, hashmap_count( c->chunkers )
   );
   scaffold_assert( 0 == hashmap_count( c->chunkers ) );
#endif /* DEBUG */
}
#endif /* USE_CHUNKS */

void client_stop( struct CLIENT* c ) {
   bstring buffer = NULL;
#ifdef DEBUG
   SCAFFOLD_SIZE test_count = 0;

   //scaffold_assert( CLIENT_SENTINAL == c->sentinal );

   if( VFALSE != ipc_connected( c->link ) ) {
      lg_info( __FILE__, "Client connection stopping...\n" );
      ipc_stop( c->link );
   }

#ifdef ENABLE_LOCAL_CLIENT

   if( VFALSE != client_is_local( c ) ) {
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
   //client_set_active_t( c, NULL );
#ifdef ENABLE_LOCAL_CLIENT
   // XXX: Free these after removing all?
   if( VFALSE != client_is_local( c ) ) {
      hashmap_remove_cb( c->sprites, callback_free_graphics, NULL );
      hashmap_remove_all( c->tilesets );
#ifdef USE_ITEMS
      hashmap_remove_all( c->item_catalogs );
      vector_remove_all( c->unique_items );
#endif // USE_ITEMS
   }
#endif /* ENABLE_LOCAL_CLIENT */

   scaffold_assert( VFALSE == ipc_connected( c->link ) );
   c->running = VFALSE;

#ifdef DEBUG

   if( VFALSE != client_is_local( c ) ) {
      scaffold_assert( VFALSE == c->running );

      test_count = hashmap_count( c->channels );
      scaffold_assert( 0 == test_count );

      test_count = hashmap_count( c->sprites );
      scaffold_assert( 0 == test_count );

#ifdef USE_CHUNKS
      test_count = hashmap_count( c->chunkers );
      scaffold_assert( 0 == test_count );
#endif /* USE_CHUNKS */

      test_count = vector_count( c->delayed_files );
      scaffold_assert( 0 == test_count );

      test_count = hashmap_count( c->tilesets );
      scaffold_assert( 0 == test_count );

#ifdef USE_ITEMS
      test_count = hashmap_count( c->item_catalogs );
      scaffold_assert( 0 == test_count );

      test_count = vector_count( c->unique_items );
      scaffold_assert( 0 == test_count );
#endif // USE_ITEMS
   }
#endif // DEBUG

cleanup:
   bdestroy( buffer );
   return;
}

short client_add_channel( struct CLIENT* c, struct CHANNEL* l ) {
   if( hashmap_put( c->channels, l->name, l, VFALSE ) ) {
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
   VBOOL valid_file;

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

   if( VFALSE == valid_file ) {
      lg_error(
         __FILE__, "Server: File not found, canceling: %b\n", filepath
      );
      proto_send_chunk( c, NULL, 0, filepath, NULL );
   }

   lg_debug(
      __FILE__, "Server: Adding chunker to send: %b\n", filepath
   );
   hashmap_put( c->chunkers, filepath, h, VTRUE );

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
      mobile_set_owner( o, c );
   }
   c->puppet = o; /* Assign to client last, to activate. */
}

void client_clear_puppet( struct CLIENT* c ) {
   client_set_puppet( c, NULL );
}

static void* client_dr_cb( size_t idx, void* iter, void* arg ) {
   bstring filename = (bstring)arg;
   struct CLIENT_DELAYED_REQUEST* req = (struct CLIENT_DELAYED_REQUEST*)iter;

   if( NULL == req ) {
      lg_error( __FILE__, "NULL file request found. This shouldn't happen.\n" );
      return req;
   }
   if( 0 == bstrcmp( filename, req->filename ) ) {
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
   VBOOL deffered_lock = VFALSE;

   /* Make sure request wasn't made already. */
   if( vector_is_locked( c->delayed_files ) ) {
      deffered_lock = VTRUE;
      vector_lock( c->delayed_files, VFALSE ); /* Hack, recursive locking. */
   }
   request = vector_iterate( c->delayed_files, client_dr_cb, filename );
   if( deffered_lock ) {
      vector_lock( c->delayed_files, VTRUE ); /* Hack, recursive locking. */
   }
   if( NULL != request ) {
      goto cleanup; /* Silently. */
   }

   /* Create a new delayed request. */
   lg_info( __FILE__, "Creating delayed request for file: %b\n", filename );
   request = mem_alloc( 1, struct CLIENT_DELAYED_REQUEST );
   lgc_null( request );

   request->filename = bstrcpy( filename );
   request->type = type;

   if( vector_is_locked( c->delayed_files ) ) {
      deffered_lock = VTRUE;
      vector_lock( c->delayed_files, VFALSE ); /* Hack, recursive locking. */
   }
   verr = vector_add( c->delayed_files, request );
   if( deffered_lock ) {
      vector_lock( c->delayed_files, VTRUE ); /* Hack, recursive locking. */
   }
   lgc_negative( verr );

cleanup:
   return;
}

void client_request_file(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
#ifdef USE_CHUNKS
   struct CHUNKER* h = NULL;

   if( VFALSE != hashmap_contains_key( c->chunkers, filename ) ) {
      /* File already requested, so just be patient. */
      goto cleanup;
   }

   h = hashmap_get( c->chunkers, filename );
   if( NULL == h ) {
      /* Create a chunker and get it started, since one is not in progress. */
      /* TODO: Verify cached file hash from server. */
      h = mem_alloc( 1, struct CHUNKER );
      lg_debug( __FILE__, "Adding unchunker to receive: %b\n", filename );
      lg_debug( __FILE__, "File cache path is: %b\n", &str_client_cache_path );
      chunker_unchunk_start(
         h, type, filename, &str_client_cache_path
      );
      hashmap_put( c->chunkers, filename, h, VTRUE );
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
   VBOOL remove_ok;

   scaffold_assert( 0 < blength( cp->filename ) );

   if( cp->current > cp->total ) {
      lg_error(
         __FILE__, "Invalid progress for %s.\n", bdata( cp->filename )
      );
      //lgc_error = LGC_ERROR_MISC;
      goto cleanup;
   }

   h = hashmap_get( c->chunkers, cp->filename );
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
      remove_ok = hashmap_remove( c->chunkers, cp->filename );
      chunker_free( h );
      scaffold_assert( VFALSE != remove_ok );
      goto cleanup;
   }

   scaffold_assert( 0 < blength( cp->data ) );

   chunker_unchunk_pass( h, cp->data, cp->current, cp->total, cp->chunk_size );

   chunker_percent = chunker_unchunk_percent_progress( h, VFALSE );
   if( 0 < chunker_percent ) {
      lg_debug(
         __FILE__, "Chunker: %b: %d%%\n", h->filename, chunker_percent );
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

   assert( VTRUE == chunker_unchunk_finished( h ) );

   datafile_handle_stream( h->type, h->filename, h->raw_ptr, h->raw_length, c );

/* cleanup: */
   lg_debug( __FILE__, "Removing finished chunker for: %b\n", h->filename );
   /* Hashmap no longer adds a ref. */
   hashmap_remove( c->chunkers, h->filename );
   chunker_free( h );
   return;
}

#endif /* USE_CHUNKS */

/** \brief
 * \param
 * \param
 * \return VTRUE if input is consumed, or VFALSE otherwise.
 */
VBOOL client_poll_ui(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
) {
   struct TILEMAP* t = NULL;
   int bstr_ret;
   VBOOL retval = VFALSE;
   struct TWINDOW* w = NULL;

   if( NULL != c && NULL != mobile_get_channel( c->puppet ) ) {
      t = mobile_get_tilemap( c->puppet );
   }

#ifdef DEBUG_VM
   /* Poll window: REPL */
   if( NULL != ui_window_by_id( c->ui, &str_client_window_id_repl ) ) {
      retval = VTRUE; /* Whatever the window does, it consumes input. */
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
   w = client_get_local_window( c );
   lgc_null( w );
   if( NULL != ui_window_by_id( client_get_ui( c ), &str_client_window_id_chat ) ) {
      retval = VTRUE; /* Whatever the window does, it consumes input. */
      if( 0 != ui_poll_input(
         client_get_ui( c ), p, &str_client_window_id_chat
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
   if( NULL != ui_window_by_id( client_get_ui( c ), &str_client_window_id_inv ) ) {
      retval = VTRUE; /* Whatever the window does, it consumes input. */
      if( 0 != ui_poll_input(
         client_get_ui( c ), p, &str_client_window_id_inv
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

int client_set_names(
   struct CLIENT* c, bstring nick, bstring uname, bstring rname
) {
   int bstr_result = 0;

   /* TODO: Handle this if we're connected. */
   if( client_is_local( c ) ) {
      scaffold_assert( !client_is_connected( c ) );
   }

   if( NULL != nick ) {
      bwriteallow( *(c->nick) );
      bstr_result = bassign( c->nick, nick );
      lgc_nonzero( bstr_result );
      bwriteprotect( *(c->nick) );
   }

   if( NULL != rname ) {
      bwriteallow( *(c->realname) );
      bstr_result = bassign( c->realname, rname );
      lgc_nonzero( bstr_result );
      bwriteprotect( *(c->realname) );
   }

   if( NULL != uname ) {
      bwriteallow( *(c->username) );
      bstr_result = bassign( c->username, uname );
      lgc_nonzero( bstr_result );
      bwriteprotect( *(c->username) );
   }

cleanup:
   return bstr_result;
}

int client_set_remote( struct CLIENT* c, const bstring remote ) {
   int res = 0;
   bwriteallow( *(c->remote) );
   res = bassign( c->remote, remote );
   bwriteprotect( *(c->remote) );
   return res;
}

struct MOBILE* client_get_puppet( struct CLIENT* c ) {
   if( NULL == c ) {
      return NULL;
   }
   return c->puppet;
}

#ifdef USE_ITEMS

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
      vector_set( &(c->unique_items), serial, e, VTRUE );
   }

cleanup:
   return;
}

#endif // USE_ITEMS

GRAPHICS* client_get_screen( struct CLIENT* c ) {
   struct TWINDOW* w = NULL;
   struct GRAPHICS* g = NULL;

   scaffold_assert_client();

   lgc_null( c );

   w = client_get_local_window( c );
   lgc_null( w );

   g = twindow_get_screen( w );

cleanup:

   return g;
}

struct TILEMAP* client_get_tilemap_active( struct CLIENT* c ) {
   struct CHANNEL* l = NULL;
   struct TILEMAP* t = NULL;
   VBOOL previously_silent = lgc_error_silent;

   lgc_silence();
   lgc_null( c );
   l = client_get_channel_active( c );
   lgc_null( l );
   t = channel_get_tilemap( l );

cleanup:
   if( !previously_silent ) {
      lgc_unsilence();
   }

   return t;
}

/** \brief Returns a reference to the client's current nick. This reference
 *         should not be modified!
 * \param
 * \return
 *
 */
bstring client_get_nick( struct CLIENT* c ) {
   return c->nick;
}

#ifdef USE_CHUNKS
struct CHUNKER* client_get_chunker( struct CLIENT* c, bstring key ) {
   return hashmap_get( c->chunkers, key );
}
#endif /* USE_CHUNKS */

GRAPHICS* client_get_sprite( struct CLIENT* c, bstring filename ) {
   return hashmap_get( c->sprites, filename );
}

struct TILEMAP_TILESET* client_get_tileset( struct CLIENT* c, bstring filename ) {
   return hashmap_get( c->tilesets, filename );
}

void client_add_ref( struct CLIENT* c ) {
   refcount_inc( c, "client" );
}

void client_load_tileset_data( struct CLIENT* c, const bstring filename, BYTE* data, size_t length ) {
   struct TILEMAP_TILESET* set = NULL;

   /* TODO: Fetch this tileset from other tilemaps, too. */
   set = client_get_tileset( c, filename );
   scaffold_assert( NULL != set );
   datafile_parse_ezxml_string(
      set, data, length, VTRUE, DATAFILE_TYPE_TILESET, filename
   );

   /* External tileset loaded. */
   c->tilesets_loaded++;
   lg_debug( __FILE__, "Tileset load count: %d\n", c->tilesets_loaded );
}

void client_load_tilemap_data( struct CLIENT* c, const bstring filename, BYTE* data, size_t length ) {
   bstring lname = NULL;
   struct CHANNEL* l = NULL;
#ifdef USE_EZXML
   ezxml_t xml_data = NULL;
#endif /* USE_EZXML */

   lname = bfromcstr( "" );
#ifdef USE_EZXML
   xml_data = datafile_tilemap_ezxml_peek_lname(
      data, length, lname
   );
#endif /* USE_EZXML */

   l = client_get_channel_by_name( c, lname );
   /* TODO: Make this dump us back at the menu. */
   lgc_null_msg(
      l, "Unable to find channel to attach loaded tileset."
   );
   if( NULL == l ) {
      channel_set_error( l, "Unable to load channel; missing data." );
   }

#ifdef USE_EZXML
   scaffold_assert( TILEMAP_SENTINAL != l->tilemap->sentinal );
   c->tilesets_loaded += datafile_parse_tilemap_ezxml_t(
      l->tilemap, xml_data, filename, VTRUE
   );
   scaffold_assert( TILEMAP_SENTINAL == l->tilemap->sentinal );

   /* Download missing tilesets. */
   hashmap_iterate( c->tilesets, callback_download_tileset, c );

#endif /* USE_EZXML */

   /* Go through the parsed tilemap and load graphics. */
   proto_client_request_mobs( c, l );

   lg_debug(
      __FILE__,
      "Client: Tilemap for %s successfully attached to channel.\n",
      bdata( l->name )
   );

cleanup:
   ezxml_free( xml_data );
   bdestroy( lname );
   return;
}

#if 0
/* TODO: Condense tilesets and sprites into "images" */
size_t client_get_sprite_count( struct CLIENT* c ) {
   return hashmap_count( &(c->sprites) );
}

size_t client_get_tileset_count( struct CLIENT* c ) {
   return hashmap_count( &(c->tilesets) );
}

size_t client_get_chunker_count(  )
#endif

#define client_is_loaded_compare( count, src, last, identifier ) \
   count = src; \
   if( last != count ) { \
      lg_debug( __FILE__, identifier " count changed: %d to %d\n", last, count ); \
      last = count; \
   } \
   if( 0 >= count  ) { \
      goto cleanup; \
   }

VBOOL client_is_loaded( struct CLIENT* c ) {
   VBOOL loaded = VFALSE;
   static size_t
      last_tilesets_loaded = 0,
      last_delayed = 0,
#ifdef USE_CHUNKS
      last_chunkers = 0,
#endif /* USE_CHUNKS */
      last_tilesets = 0;

   if( last_tilesets != hashmap_count( c->tilesets ) ) {
      lg_debug( __FILE__,
         "Tileset count changed: %d to %d\n", last_tilesets,
         hashmap_count( c->tilesets ) );
      last_tilesets = hashmap_count( c->tilesets );
   }
   if( 0 >= hashmap_count( c->tilesets ) ) {
      goto cleanup;
   }

#ifdef USE_CHUNKS
   if( last_chunkers != hashmap_count( c->chunkers ) ) {
      lg_debug( __FILE__,
         "Chunker count changed: %d to %d\n", last_chunkers,
         hashmap_count( c->chunkers ) );
      last_chunkers = hashmap_count( c->chunkers );
   }
   if( 0 < hashmap_count( c->chunkers ) ) {
      goto cleanup;
   }
#endif /* USE_CHUNKS */

   if( last_delayed != vector_count( c->delayed_files ) ) {
      lg_debug( __FILE__,
         "Delayed file count changed: %d to %d\n", last_delayed,
         vector_count( c->delayed_files ) );
      last_delayed = vector_count( c->delayed_files );
   }
   if( 0 < vector_count( c->delayed_files ) ) {
      goto cleanup;
   }

   /* Loaded Tilesets */
   if( last_tilesets_loaded != c->tilesets_loaded ) {
      lg_debug(
         __FILE__, "Loaded tileset count changed: %d to %d\n",
         last_tilesets_loaded, c->tilesets_loaded
      );
      last_tilesets_loaded = c->tilesets_loaded;
   }
   if( 0 >= c->tilesets_loaded  ) {
      goto cleanup;
   }

   /* We made it this far... */
   loaded = VTRUE;

cleanup:
   return loaded;
}

struct CHANNEL* client_iterate_channels(
   struct CLIENT* c, hashmap_iter_cb cb, void* data
) {
   struct CHANNEL* l = NULL;
   lgc_null( c );
   l = hashmap_iterate( c->channels, cb, data );
cleanup:
   return l;
}

VBOOL client_set_sprite( struct CLIENT* c, bstring filename, GRAPHICS* g ) {
   VBOOL already_present = VFALSE;

   lg_debug(
      __FILE__, "Setting sprites for client: %b\n", client_get_nick( c ) );

   already_present = hashmap_put( c->sprites, filename, g, VFALSE );
   if( already_present ) {
      lg_error(
         __FILE__, "Attempted to double-add spritesheet: %b\n", filename );
      return VFALSE;
   } else {
      client_iterate_channels( c, callback_attach_channel_mob_sprites, c );
      lg_debug(
         __FILE__,
         "Client: Spritesheet %b successfully loaded into cache.\n",
         filename
      );
   }
   return !already_present;
}

VBOOL client_set_tileset( struct CLIENT* c, bstring filename, struct TILEMAP_TILESET* set ) {
   VBOOL already_present = VFALSE;

   already_present = hashmap_put( c->tilesets, filename, set, VTRUE );
   if( already_present ) {
      lg_error(
         __FILE__, "Attempted to double-add spritesheet: %b\n", filename );
      return VFALSE;
   } else {
      client_iterate_channels( c, callback_attach_channel_mob_sprites, c );
      lg_debug(
         __FILE__,
         "Client: Spritesheet %b successfully loaded into cache.\n",
         filename
      );
   }
   return !already_present;
}

VBOOL client_is_running( struct CLIENT* c ) {
   return c->running;
}

/* int client_get_gfx_mode( struct CLIENT* c ) {
   return c->gfx_mode;
} */

struct CHANNEL* client_get_channel_active( struct CLIENT* c ) {
   return hashmap_get_first( c->channels );
}

struct TWINDOW* client_get_local_window( struct CLIENT* c ) {
   return c->local_window;
}

VBOOL client_is_listening( struct CLIENT* c ) {
   return NULL != c && ipc_is_listening( c->link );
}

VBOOL client_is_connected( struct CLIENT* c ) {
   return (VFALSE != ipc_connected( c->link ) && VTRUE == (c)->running);
}

CLIENT_FLAGS client_test_flags( struct CLIENT* c, CLIENT_FLAGS flags ) {
   return c->flags & flags;
}

void client_set_flag( struct CLIENT* c, CLIENT_FLAGS flags ) {
   c->flags |= flags;
}

SCAFFOLD_SIZE_SIGNED client_write( struct CLIENT* c, const bstring buffer ) {
   return NULL != c && ipc_write( c->link, buffer );
}

SCAFFOLD_SIZE_SIGNED client_read( struct CLIENT* c, const bstring buffer ) {
   return NULL != c && ipc_read( c->link, buffer );
}

bstring client_get_realname( struct CLIENT* c ) {
   return c->realname;
}

bstring client_get_username( struct CLIENT* c ) {
   return c->username;
}

struct UI* client_get_ui( struct CLIENT* c ) {
   struct UI* ui = NULL;
   struct TWINDOW* w = NULL;

   w = client_get_local_window( c );
   lgc_null( w );

   ui = twindow_get_ui( w );
cleanup:
   return ui;
}

/** \brief
 *
 * \param
 * \param
 * \return Number of channels removed.
 *
 */
int client_remove_channel( struct CLIENT* c, const bstring lname ) {
   // TODO: Remove channel if empty.
   return hashmap_remove( c->channels, lname );
}

bstring client_get_remote( struct CLIENT* c ) {
   return c->remote;
}

size_t client_get_channels_count( struct CLIENT* c ) {
   return hashmap_count( c->channels );
}

#ifdef USE_CHUNKS
size_t client_remove_chunkers( struct CLIENT* c, bstring filter ) {
   return hashmap_remove_cb( c->chunkers, callback_free_chunkers, filter );
}
#endif /* USE_CHUNKS */

/* struct CLIENT* client_from_local_window( struct TWINDOW* twindow ) {
   return scaffold_container_of( twindow, struct CLIENT, local_window );
} */

void client_set_local_window( struct CLIENT* c, struct TWINDOW* w ) {
   scaffold_assert( NULL != w );
   scaffold_assert( NULL != c );
   c->local_window = w;
}

void* client_get_mode_data( struct CLIENT* c, bstring mode, struct CHANNEL* l ) {
   struct HASHMAP* channels = NULL;
   void* mode_data = NULL;

   assert( NULL != l && NULL != l->name );
   assert( NULL != mode );

   lgc_null( c );

   channels = hashmap_get( c->mode_data, mode );
   if( NULL == channels ) {
      lg_debug( __FILE__, "Creating channel list for %b for client %b...\n", mode, c->nick );
      channels = hashmap_new();
      hashmap_put( c->mode_data, mode, channels, VFALSE );
   }

   mode_data = hashmap_get( channels, l->name );
   if( NULL == mode_data ) {
      plugin_call( PLUGIN_MODE, mode, PLUGIN_CLIENT_INIT, c, l );
   }
   mode_data = hashmap_get( channels, l->name );

cleanup:
   return mode_data;
}

void client_set_mode_data(
   struct CLIENT* c, const bstring mode, struct CHANNEL* l, void* mode_data
) {
   struct HASHMAP* channels = NULL;

   assert( NULL != l && NULL != l->name );
   assert( NULL != mode );

   lgc_null( c );

   channels = hashmap_get( c->mode_data, mode );
   if( NULL == channels ) {
      lg_debug( __FILE__, "Creating channel list for %b for client %b...\n", mode, c->nick );
      channels = hashmap_new();
      hashmap_put( c->mode_data, mode, channels, VFALSE );
   }

   assert( NULL == hashmap_get( channels, l->name ) );
   hashmap_put( channels, l->name, mode_data, FALSE );

cleanup:
   return;
}

/* void client_set_mode_data(
   struct CLIENT* c, bstring mode, struct CHANNEL* l, void* mode_data
) {
   struct HASHMAP* channels = NULL;

   lgc_null( c );
   assert( NULL != l && NULL != l->name );
   assert( NULL != mode );

   channels = hashmap_get( c->mode_data, mode );
   if( NULL == channels ) {
      lg_debug( __FILE__, "Creating channel list for %b for mobile %b...\n", mode, c->nick );
      channels = hashmap_new();
      hashmap_put( c->mode_data, mode, channels, VFALSE );
   }

   hashmap_put( channels, l->name, mode_data, VFALSE );
cleanup:
   return;
} */
