
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

bstring client_input_from_ui = NULL;

static void client_cleanup( const struct REF *ref ) {
   struct CLIENT* c =
      (struct CLIENT*)scaffold_container_of( ref, struct CLIENT, refcount );

   ipc_free( &(c->link) );

   bdestroy( c->nick );
   bdestroy( c->realname );
   bdestroy( c->remote );
   bdestroy( c->username );
   hashmap_cleanup( &(c->sprites) );

   /* client_stop( c ); */

   hashmap_cleanup( &(c->chunkers) );
   vector_cleanup( &(c->delayed_files) );
   hashmap_cleanup( &(c->channels) );

   hashmap_remove_cb( &(c->tilesets), callback_free_tilesets, NULL );
   hashmap_cleanup( &(c->tilesets) );

   hashmap_remove_cb( &(c->item_catalogs), callback_free_catalogs, NULL );
   hashmap_cleanup( &(c->item_catalogs) );

   c->sentinal = 0;
   /* TODO: Ensure entire struct is freed. */
   /* mem_free( c ); */
}

void client_init( struct CLIENT* c ) {
   ref_init( &(c->refcount), client_cleanup );

   scaffold_assert( FALSE == c->running );

   hashmap_init( &(c->channels) );
   hashmap_init( &(c->sprites) );
   hashmap_init( &(c->chunkers) );
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
   c->running = TRUE;
   client_cleanup( &(c->refcount) );
   return TRUE;
}

BOOL client_free( struct CLIENT* c ) {
   return refcount_dec( c, "client" );
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
#ifdef DEBUG_TILES
   int bstr_ret;
   SCAFFOLD_SIZE steps_remaining_x,
      steps_remaining_y;
   static bstring pos = NULL;
#endif /* DEBUG_TILES */
   BOOL keep_going = FALSE;

   scaffold_set_client();

   /* Check for commands from the server. */
   keep_going = proto_dispatch( c, NULL );

   /* TODO: Is this ever called? */
   if( FALSE == keep_going ) {
      scaffold_print_info( &module, "Remote server disconnected.\n" );
      client_stop( c );
   }

#ifdef DEBUG_TILES
   if( NULL == pos ) {
      pos = bfromcstr( "" );
   }

   if( NULL != c->puppet ) {
      steps_remaining_x = mobile_get_steps_remaining_x( c->puppet, FALSE );
      steps_remaining_y = mobile_get_steps_remaining_y( c->puppet, FALSE );

      bstr_ret = bassignformat( pos,
         "Player: %d (%d)[%d], %d (%d)[%d]",
         c->puppet->x, c->puppet->prev_x, steps_remaining_x,
         c->puppet->y, c->puppet->prev_y, steps_remaining_y
      );
      scaffold_check_nonzero( bstr_ret );
      ui_debug_window( c->ui, &str_wid_debug_tiles_pos, pos );
   }

#ifdef USE_CHUNKS
   /* Deal with chunkers that will never receive blocks that are finished via
    * their cache.
    */
   hashmap_iterate( &(c->chunkers), callback_proc_client_chunkers, c );
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
      hashmap_remove_cb( &(c->channels), callback_free_channels, NULL );
#ifdef DEBUG
   scaffold_print_debug(
      &module,
      "Removed %d channels. %d remaining.\n",
      deleted, hashmap_count( &(c->channels) )
   );
#endif /* DEBUG */
   scaffold_assert( 0 == hashmap_count( &(c->channels) ) );

cleanup:
   return;
}

#ifdef USE_CHUNKS
void client_free_chunkers( struct CLIENT* c ) {
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;

   deleted =
#endif /* DEBUG */
      hashmap_remove_cb( &(c->chunkers), callback_free_chunkers, NULL );
   scaffold_print_debug(
      &module,
      "Removed %d chunkers. %d remaining.\n",
      deleted, hashmap_count( &(c->chunkers) )
   );
   scaffold_assert( 0 == hashmap_count( &(c->chunkers) ) );
cleanup:
   return;
}
#endif /* USE_CHUNKS */

void client_stop( struct CLIENT* c ) {
   bstring buffer = NULL;
#ifdef DEBUG
   SCAFFOLD_SIZE deleted;

   scaffold_assert( CLIENT_SENTINAL == c->sentinal );

   if( FALSE != ipc_connected( c->link ) ) {
      scaffold_print_info( &module, "Client connection stopping...\n" );
      ipc_stop( c->link );
   }

#ifdef ENABLE_LOCAL_CLIENT

   if( TRUE == ipc_is_local_client( c->link ) ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }

   c->flags = 0;

#endif /* ENABLE_LOCAL_CLIENT */

#endif /* DEBUG */

   client_clear_puppet( c );

   buffer = bfromcstralloc( CLIENT_BUFFER_ALLOC, "" );
   scaffold_check_null( buffer );

   client_free_channels( c );
#ifdef USE_CHUNKS
   client_free_chunkers( c );
#endif /* USE_CHUNKS */

   /* Empty receiving buffer. */
   proto_empty_buffer( c );

   client_clear_puppet( c );
#ifdef ENABLE_LOCAL_CLIENT
   client_local_free( c );
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

#ifdef USE_CHUNKS

void client_send_file(
   struct CLIENT* c, DATAFILE_TYPE type,
   const bstring serverpath, const bstring filepath
) {
   struct CHUNKER* h = NULL;
   BOOL valid_file;

   scaffold_print_debug(
      &module, "Sending file to client %p: %s\n", c, bdata( filepath )
   );

   /* Begin transmitting tilemap. */
   h = mem_alloc( 1, struct CHUNKER );
   scaffold_check_null( h );

   valid_file = chunker_chunk_start_file(
      h,
      type,
      serverpath,
      filepath,
      64
   );

   if( FALSE == valid_file ) {
      scaffold_print_error(
         &module, "Server: File not found, canceling: %b\n", filepath
      );
      proto_send_chunk( c, NULL, 0, filepath, NULL );
   }

   scaffold_print_debug(
      &module, "Server: Adding chunker to send: %b\n", filepath
   );
   hashmap_put( &(c->chunkers), filepath, h, TRUE );

cleanup:
   if( NULL != h ) {
      chunker_free( h );
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

static void* client_dr_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   bstring filename = (bstring)arg;
   struct CLIENT_DELAYED_REQUEST* req = (struct CLIENT_DELAYED_REQUEST*)iter;

   if( NULL == req || 0 == bstrcmp( filename, req->filename ) ) {
      return req;
   }

   return NULL;
}

void client_request_file_later(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
   struct CLIENT_DELAYED_REQUEST* request = NULL;
   VECTOR_ERR verr;

   /* Make sure request wasn't made already. */
   request = vector_iterate( &(c->delayed_files), client_dr_cb, filename );
   if( NULL != request ) {
      goto cleanup; /* Silently. */
   }

   /* Create a new delayed request. */
   request = mem_alloc( 1, struct CLIENT_DELAYED_REQUEST );
   scaffold_check_null( request );

   request->filename = bstrcpy( filename );
   request->type = type;

   verr = vector_add( &(c->delayed_files), request );
   scaffold_check_equal( VECTOR_ERR_NONE, verr );

cleanup:
   return;
}

void client_request_file(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
) {
#ifdef USE_CHUNKS
   struct CHUNKER* h = NULL;
   BOOL force_finished = FALSE;

   hashmap_lock( &(c->chunkers), TRUE );

   if( FALSE != hashmap_contains_key_nolock( &(c->chunkers), filename ) ) {
      /* File already requested, so just be patient. */
      goto cleanup;
   }

   h = hashmap_get_nolock( &(c->chunkers), filename );
   if( NULL == h ) {
      /* Create a chunker and get it started, since one is not in progress. */
      /* TODO: Verify cached file hash from server. */
      h = mem_alloc( 1, struct CHUNKER );
      chunker_unchunk_start(
         h, type, filename, &str_client_cache_path
      );
      scaffold_print_debug(
         &module, "Adding unchunker to receive: %s\n", bdata( filename )
      );
      hashmap_put_nolock( &(c->chunkers), filename, h, TRUE );
      scaffold_check_nonzero( scaffold_error );

      if( !chunker_unchunk_finished( h ) ) {
         /* File not in cache. */
         proto_request_file( c, filename, type );
      }
   }

cleanup:
   hashmap_lock( &(c->chunkers), FALSE );
#else
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
      scaffold_print_error(
         &module, "Invalid progress for %s.\n", bdata( cp->filename )
      );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   h = hashmap_get( &(c->chunkers), cp->filename );
   if( NULL == h ) {
      scaffold_print_error(
         &module,
         "Client: Invalid data block received (I didn't ask for this?): %s\n",
         bdata( cp->filename )
      );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   if( DATAFILE_TYPE_INVALID == cp->type ) {
      scaffold_assert( NULL != h );
      scaffold_print_debug(
         &module, "Removing invalid chunker: %b\n", h->filename );
      chunker_free( h );
      remove_ok = hashmap_remove( &(c->chunkers), cp->filename );
      scaffold_assert( TRUE == remove_ok );
      goto cleanup;
   }

   scaffold_assert( 0 < blength( cp->data ) );

   chunker_unchunk_pass( h, cp->data, cp->current, cp->total, cp->chunk_size );

   chunker_percent = chunker_unchunk_percent_progress( h, FALSE );
   if( 0 < chunker_percent ) {
      scaffold_print_debug(
         &module, "Chunker: %s: %d%%\n", bdata( h->filename ), chunker_percent );
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

cleanup:
   scaffold_print_debug(
      &module,
      "Client: Removing finished chunker for: %s\n", bdata( h->filename )
   );
   chunker_free( h );
   hashmap_remove( &(c->chunkers), h->filename );
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
      t = &(c->puppet->channel->tilemap);
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

         //proto_send_msg_channel( c, l, client_input_from_ui );
         //mobile_speak( c->puppet, client_input_from_ui );

         //goto reset_buffer;
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
