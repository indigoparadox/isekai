
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

#ifdef ENABLE_LOCAL_CLIENT
   /*
   if( TRUE == ipc_is_local_client( c->link ) ) {
      scaffold_assert_client();
      bdestroy( client_input_from_ui );
      client_input_from_ui = NULL;
   }
   */
#endif /* ENABLE_LOCAL_CLIENT */

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
   //IRC_COMMAND* cmd = NULL;
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
   //cmd = callback_ingest_commands( NULL, c, NULL );

   keep_going = proto_dispatch( c, NULL );

   /* TODO: Is this ever called? */
   if( FALSE == keep_going ) {
      scaffold_print_info( &module, "Remote server disconnected.\n" );
      client_stop( c );
      //bdestroy( cmd->line );
      //mem_free( cmd );
      //cmd = NULL;
   }

   /*
   if( NULL != cmd ) {
      retval = TRUE;
      if( NULL != cmd->callback ) {
         cmd->callback( cmd->client, cmd->server, cmd->args, cmd->line );
      } else {
         scaffold_print_error(
            &module, "Client: Invalid command: %s\n", bdata( &(cmd->command) )
         );
      }
      irc_command_free( cmd );
      retval = TRUE;
   }
   */

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

   /*
   if( TRUE != c->running ) {
      goto cleanup;
   }
   */

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
   while( 0 < ipc_read(
      c->link,
      buffer
   ) );

   client_clear_puppet( c );
#ifdef ENABLE_LOCAL_CLIENT
   if(
      TRUE == ipc_is_local_client( c->link ) &&
      HASHMAP_SENTINAL == c->sprites.sentinal
   ) {
      hashmap_remove_cb( &(c->sprites), callback_free_graphics, NULL );
   }
#endif /* ENABLE_LOCAL_CLIENT */

   scaffold_assert( FALSE == ipc_connected( c->link ) );
   c->running = FALSE;

cleanup:
   bdestroy( buffer );
   return;
}

void client_add_channel( struct CLIENT* c, struct CHANNEL* l ) {
   hashmap_put( &(c->channels), l->name, l );
}

void client_join_channel( struct CLIENT* c, const bstring name ) {
   bstring buffer = NULL;
   int bstr_retval;
   /* We won't record the channel in our list until the server confirms it. */

   scaffold_set_client();

   buffer = bfromcstr( "JOIN " );
   scaffold_check_null( buffer );
   bstr_retval = bconcat( buffer, name );
   scaffold_check_nonzero( bstr_retval );

   client_send( c, buffer );

   c->flags |= CLIENT_FLAGS_SENT_CHANNEL_JOIN;

cleanup:
   bdestroy( buffer );
}

void client_leave_channel( struct CLIENT* c, const bstring lname ) {
   int bstr_retval;
   bstring buffer = NULL;

   /* We won't record the channel in our list until the server confirms it. */

   scaffold_assert_client();

   buffer = bfromcstr( "PART " );
   scaffold_check_null( buffer );
   bstr_retval = bconcat( buffer, lname );
   scaffold_check_nonzero( bstr_retval );
   client_send( c, buffer );

   /* TODO: Add callback from parser and only delete channel on confirm. */
   hashmap_remove( &(c->channels), lname );

cleanup:
   bdestroy( buffer );
   return;
}

void client_send( struct CLIENT* c, const bstring buffer ) {
   int bstr_retval;
   bstring buffer_copy = NULL;

   /* TODO: Make sure we're still connected. */

   buffer_copy = bstrcpy( buffer );
   scaffold_check_null( buffer_copy );
   bstr_retval = bconchar( buffer_copy, '\r' );
   scaffold_check_nonzero( bstr_retval );
   bstr_retval = bconchar( buffer_copy, '\n' );
   scaffold_check_nonzero( bstr_retval );
   ipc_write( c->link, buffer_copy );

#ifdef DEBUG_NETWORK
   if( TRUE == c->client_side ) {
      scaffold_print_debug(
         &module, "Client sent to server: %s\n", bdata( buffer )
      );
   } else {
      scaffold_print_debug(
         &module, "Server sent to client %d: %s\n",
         c->link.socket, bdata( buffer )
      );
   }
#endif /* DEBUG_NETWORK */

cleanup:
   bdestroy( buffer_copy );
#ifdef ENABLE_LOCAL_CLIENT
   if( ipc_is_local_client( c->link ) ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }
#endif /* ENABLE_LOCAL_CLIENT */
   return;
}

void client_printf( struct CLIENT* c, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

#ifdef ENABLE_LOCAL_CLIENT
   if( ipc_is_local_client( c->link ) ) {
      scaffold_assert_client();
   } else {
      struct CONNECTION* n = c->link;
      scaffold_assert_server();
   }
#endif /* ENABLE_LOCAL_CLIENT */

   buffer = bfromcstralloc( strlen( message ), "" );
   scaffold_check_null( buffer );

   va_start( varg, message );
   scaffold_vsnprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == scaffold_error ) {
      client_send( c, buffer );
   }

cleanup:
   bdestroy( buffer );
   return;
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
   //scaffold_check_nonzero( scaffold_error );

   if( FALSE == valid_file ) {
      scaffold_print_error(
         &module, "Server: File not found, canceling: %b\n", filepath
      );
      proto_send_chunk( c, NULL, 0, filepath, NULL );
   }

   scaffold_print_debug(
      &module, "Server: Adding chunker to send: %b\n", filepath
   );
   hashmap_put( &(c->chunkers), filepath, h );

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
      hashmap_put_nolock( &(c->chunkers), filename, h );
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

   /*
   if( 0 < d->incoming_buffer_len && d->incoming_buffer_len != progress->total ) {
      scaffold_print_error( "Invalid total for %s.\n", bdata( progress->filename ) );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }
   */

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
static BOOL client_poll_ui(
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

static BOOL client_poll_keyboard( struct CLIENT* c, struct INPUT* input ) {
   struct MOBILE* puppet = NULL;
   struct MOBILE_UPDATE_PACKET update;
   struct UI* ui = NULL;
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;
   struct CHANNEL* l = NULL;
   struct TILEMAP* t = NULL;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   /* Make sure the buffer that all windows share is available. */
   if(
      NULL == c->puppet ||
      (c->puppet->steps_remaining < -8 || c->puppet->steps_remaining > 8)
   ) {
      /* TODO: Handle limited input while loading. */
      input_clear_buffer( input );
      return FALSE; /* Silently ignore input until animations are done. */
   } else {
      puppet = c->puppet;
      ui = c->ui;
      update.o = puppet;
      update.l = puppet->channel;
      scaffold_check_null( update.l );
      l = puppet->channel;
      scaffold_check_null_msg( l, "No channel loaded." );
      t = &(l->tilemap);
      scaffold_check_null_msg( t, "No tilemap loaded." );
   }

   /* If no windows need input, then move on to game input. */
   switch( input->character ) {
   case INPUT_ASSIGNMENT_QUIT: proto_client_stop( c ); return TRUE;
   case INPUT_ASSIGNMENT_UP:
      update.update = MOBILE_UPDATE_MOVEUP;
      update.x = c->puppet->x;
      update.y = c->puppet->y - 1;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_LEFT:
      update.update = MOBILE_UPDATE_MOVELEFT;
      update.x = c->puppet->x - 1;
      update.y = c->puppet->y;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_DOWN:
      update.update = MOBILE_UPDATE_MOVEDOWN;
      update.x = c->puppet->x;
      update.y = c->puppet->y + 1;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_RIGHT:
      update.update = MOBILE_UPDATE_MOVERIGHT;
      update.x = c->puppet->x + 1;
      update.y = c->puppet->y;
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_ATTACK:
      update.update = MOBILE_UPDATE_ATTACK;
      /* TODO: Get attack target. */
      proto_client_send_update( c, &update );
      return TRUE;

   case INPUT_ASSIGNMENT_INV:
      if( NULL == client_input_from_ui ) {
         client_input_from_ui = bfromcstralloc( 80, "" );
         scaffold_check_null( client_input_from_ui );
      }
      ui_window_new(
         ui, win, &str_client_window_id_inv,
         &str_client_window_title_inv, NULL, -1, -1, 600, 380
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, TRUE, client_input_from_ui,
         0, UI_CONST_HEIGHT_FULL, 300, UI_CONST_HEIGHT_FULL
      );
      ui_control_add( win, &str_client_control_id_inv_self, control );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_INVENTORY, TRUE, client_input_from_ui,
         300, UI_CONST_HEIGHT_FULL, 300, UI_CONST_HEIGHT_FULL
      );
      cache = tilemap_get_item_cache( t, puppet->x, puppet->y, TRUE );
      ui_set_inventory_pane_list( control, &(cache->items) );
      ui_control_add( win, &str_client_control_id_inv_ground, control );
      ui_window_push( ui, win );
      return TRUE;

   case '\\':
      if( NULL == client_input_from_ui ) {
         client_input_from_ui = bfromcstralloc( 80, "" );
         scaffold_check_null( client_input_from_ui );
      }
      ui_window_new(
         ui, win, &str_client_window_id_chat,
         &str_client_window_title_chat, NULL, -1, -1, -1, -1
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, TRUE, client_input_from_ui,
         -1, -1, -1, -1
      );
      ui_control_add( win, &str_client_control_id_chat, control );
      ui_window_push( ui, win );
      return TRUE;
#ifdef DEBUG_VM
   case 'p': windef_show_repl( ui ); return TRUE;
#endif /* DEBUG_VM */
#ifdef DEBUG_TILES
   case 't':
      if( 0 == input->repeat ) {
         tilemap_toggle_debug_state();
         return TRUE;
      }
      break;
   case 'l':
      if( 0 == input->repeat ) {
         tilemap_dt_layer++;
         return TRUE;
      }
      break;
#endif /* DEBUG_TILES */
   }

   cleanup:
   return FALSE;
}

void client_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p ) {
   scaffold_set_client();
   input_get_event( p );
   if( INPUT_TYPE_CLOSE == p->type ) {
      proto_client_stop( c );
   } else if( INPUT_TYPE_KEY == p->type ) {
      if( !client_poll_ui( c, l, p ) ) {
         client_poll_keyboard( c, p );
      }
   }
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

/*
BOOL client_connected( struct CLIENT* c ) {
   if( 0 < c->link.socket ) {
      scaffold_assert( TRUE == c->running );
      return TRUE;
   } else {
      scaffold_assert( FALSE == c->running );
      return FALSE;
   }
}
*/

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

GRAPHICS* client_get_local_screen( struct CLIENT* c ) {
   scaffold_assert_client();

   return c->ui->screen_g;
}
