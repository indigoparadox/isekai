
#include "config.h"
#include "ipc.h"
#include "scaffold.h"
#include "input.h"
#include "server.h"
#include "ui.h"
#include "callback.h"
#include "backlog.h"
#include "animate.h"
#include "rng.h"
#include "channel.h"
#include "proto.h"

#ifdef __GNUC__
#include <unistd.h>
#endif /* __GNUC__ */

#define SERVER_LOOPS_PER_CYCLE 5

SCAFFOLD_MODULE( "main.c" );

static struct tagbstring str_readme_id = bsStatic( "readme" );
static struct tagbstring str_readme_title = bsStatic( "Readme" );
static struct tagbstring str_cdialog_id = bsStatic( "connect" );
static struct tagbstring str_cdialog_title = bsStatic( "Connect to Server" );
static struct tagbstring str_cdialog_prompt =
   bsStatic( "Connect to [address:port]:" );

#if defined( USE_CONNECT_DIALOG ) && !defined( USE_NETWORK )
#error Connect dialog requires network to be enabled!
#endif /* USE_CONNECT_DIALOG && !USE_NETWORK */

#ifndef USE_CONNECT_DIALOG
SCAFFOLD_SIZE loop_count = 0;
#endif /* USE_CONNECT_DIALOG */

struct SERVER* main_server = NULL;

#ifdef ENABLE_LOCAL_CLIENT
struct CLIENT* main_client = NULL;
GRAPHICS* g_screen = NULL;
struct INPUT* input = NULL;
struct UI* ui = NULL;
struct CHANNEL* l = NULL;
bstring buffer_host = NULL;
bstring buffer_channel = NULL;
BOOL showed_readme = FALSE;
#endif /* ENABLE_LOCAL_CLIENT */

static struct tagbstring str_top_down = bsStatic( "Top Down" );
#ifndef DISABLE_MODE_POV
static struct tagbstring str_pov = bsStatic( "POV" );
#endif /* !DISABLE_MODE_POV */
static bstring mode_list[] = {
   &str_top_down,
#ifndef DISABLE_MODE_POV
   &str_pov,
#endif /* !DISABLE_MODE_POV */
   NULL
};

#ifdef USE_RANDOM_PORT
bstring str_service = NULL;
#endif /* USE_RANDOM_PORT */

#ifdef DEBUG_FPS
#define FPS_SAMPLES 50
int ticksum = 0;
int tickindex = 0;
int ticklist[FPS_SAMPLES] = { 0 };
static bstring graphics_fps = NULL;
static struct tagbstring str_wid_debug_fps = bsStatic( "debug_fps" );
#endif /* DEBUG_FPS */

static struct tagbstring str_wid_debug_ip = bsStatic( "debug_ip" );
static struct tagbstring str_cid_connect_host = bsStatic( "connect_host" );
static struct tagbstring str_cid_connect_nick = bsStatic( "connect_nick" );
static struct tagbstring str_cid_connect_channel = bsStatic( "connect_channel" );
static struct tagbstring str_cid_connect_gfxmode = bsStatic( "connect_gfxmode" );
static struct tagbstring str_title = bsStatic( "ProCIRCd" );
static struct tagbstring str_loading = bsStatic( "Loading" );
static struct tagbstring str_localhost = bsStatic( "127.0.0.1" );
//#ifndef DISABLE_MODE_ISO
//static struct tagbstring str_default_channel = bsStatic( "#isotest" );
//#else
static struct tagbstring str_default_channel = bsStatic( "#testchan" );
//#endif /* !DISABLE_MODE_ISO */
static uint32_t server_port = 33080;

#ifdef USE_ALLEGRO
/* TODO: Is this really defined nowhere? */
void allegro_exit();
#endif /* USE_ALLEGRO */

#ifdef DEBUG_FPS
static double calc_fps( int newtick ) {
   ticksum -= ticklist[tickindex];  /* subtract value falling off */
   ticksum += newtick;              /* add new value */
   ticklist[tickindex] = newtick;   /* save new value so it can be subtracted later */

   if( FPS_SAMPLES == ++tickindex ) {
      tickindex = 0;
   }

   return ((double)ticksum / FPS_SAMPLES);
}
#endif /* DEBUG_FPS */

static BOOL loop_game() {
   BOOL keep_going = TRUE;
   int i;
   struct ANIMATION* a = NULL;
   GRAPHICS* throbber = NULL;
   GRAPHICS_RECT r;
   static BOOL load_complete = FALSE;
   GFX_COORD_PIXEL backlog_height_tiles = CONFIG_BACKLOG_TILES_HEIGHT;
   GFX_COORD_PIXEL backlog_height_px = 0;

   for( i = 0 ; SERVER_LOOPS_PER_CYCLE > i ; i++ ) {
      server_service_clients( main_server );
   }

   if( !main_server->self.running ) {
      keep_going = FALSE;
      goto cleanup;
   }

#ifdef USE_VM
   /* Run the channel VMs. */
   hashmap_iterate( &(main_server->self.channels), callback_proc_channel_vms, NULL );

#ifndef USE_TURNS
   /* If we're doing real-time, then do a VM tick for every full cycle of
    * state updates. */
   vm_tick();
#endif /* USE_TURNS */
#endif /* USE_VM */

   server_poll_new_clients( main_server );

#ifdef ENABLE_LOCAL_CLIENT
   if( FALSE == animate_is_blocking() ) {
      client_local_poll_input( main_client, l, input );
      client_update( main_client, g_screen );
      client_local_update( main_client, l );
   }

   /* Do drawing. */
   l = hashmap_get_first( &(main_client->channels) );
   if( TRUE == channel_has_error( l ) ) {
      /* Abort and go back to connect dialog. */
      /* We need to stop both client AND server, 'cause the data is bad! */
      // TODO
      //scaffold_print_debug( &module, "Stopping server...\n" );
      //scaffold_set_server();
      //server_stop_clients( main_server );
      //server_stop( main_server );
      ui_message_box( ui, l->error );
      client_stop( main_client );
      scaffold_print_debug( &module, "Unloading loading animation...\n" );
      animate_cancel_animation( NULL, &str_loading );
      goto cleanup;

   } else if( FALSE == channel_is_loaded( l ) ) {
      /* Make sure the loading animation is running. */
      if( NULL == animate_get_animation( &str_loading ) ) {
         load_complete = FALSE;
         scaffold_print_debug( &module, "Creating loading animation...\n" );
         graphics_surface_new( throbber, 0, 0, 32, 32 );
         graphics_draw_rect( throbber, 0, 0, 32, 32, GRAPHICS_COLOR_WHITE, TRUE );

         graphics_measure_text( g_screen, &r, GRAPHICS_FONT_SIZE_16, &str_loading );

         throbber->virtual_x = (GRAPHICS_SCREEN_WIDTH / 2) - (r.w / 2) - 40;
         throbber->virtual_y = (GRAPHICS_SCREEN_HEIGHT / 2) - 8;

         a = mem_alloc( 1, struct ANIMATION );
         animate_create_resize( a, throbber, 16, 16, 1, 1, FALSE );
         a->indefinite = TRUE;

         graphics_surface_free( throbber );
         throbber = NULL;

         animate_add_animation( a, &str_loading );
      }

      graphics_clear_screen( g_screen, GRAPHICS_COLOR_CHARCOAL );
      graphics_draw_text(
         g_screen, GRAPHICS_SCREEN_WIDTH / 2, GRAPHICS_SCREEN_HEIGHT / 2,
         GRAPHICS_TEXT_ALIGN_CENTER, GRAPHICS_COLOR_WHITE,
         GRAPHICS_FONT_SIZE_16, &str_loading, FALSE
      );
      ui_draw( ui, g_screen );

      /* Animations are drawn on top of everything. */
      animate_cycle_animations( g_screen );
      animate_draw_animations( g_screen );

      goto cleanup;

   } else if( TRUE != main_client->running ) {
      /* We're stopping, not starting. */
      scaffold_print_debug( &module, "Stopping server...\n" );
      server_stop( main_server );
#ifndef USE_NETWORK
      keep_going = FALSE;
#endif /* USE_NETWORK */
      goto cleanup;

   } else if( NULL == main_client->active_tilemap ) {
      scaffold_print_debug( &module, "Unsetting main client...\n" );
      client_set_active_t( main_client, l->tilemap );
   }

   /* Client drawing stuff after this. */
   scaffold_set_client();

   if( !load_complete ) {
      /* If we're this far, we must be done loading! */
      scaffold_print_debug( &module, "Unloading loading animation...\n" );
      animate_cancel_animation( NULL, &str_loading );
      load_complete = TRUE;

      /* Setup the window for drawing tiles, etc. */
      twindow_update_details( &(main_client->local_window) );

      backlog_height_px =
         backlog_height_tiles * main_client->local_window.grid_h;

      /* Show the backlog at the bottom, shrinking the map to fit. */
      main_client->local_window.height -= backlog_height_tiles;
      backlog_ensure_window( ui, backlog_height_px );
   } else {
      /* We need this for the one-time stuff under !load_complete AND for        *
       * drawing the mask below.
       */
      backlog_height_px =
         backlog_height_tiles * main_client->local_window.grid_h;
   }

   /* If we're on the move then update the window frame. */
   /* Allows for smooth-scrolling view window with bonus that action is    *
    * technically paused while doing so.                                   */
   if(
      0 != main_client->puppet->steps_remaining ||
      main_client->local_window.max_x == main_client->local_window.min_x ||
      TILEMAP_REDRAW_ALL == main_client->active_tilemap->redraw_state
   ) {
      client_local_update( main_client, l );
   }

   animate_cycle_animations( g_screen );

   /* If there's no puppet then there should be a load screen. */
   scaffold_assert( NULL != main_client->puppet );

   client_local_draw( main_client, l );

   /* Draw masks to cover up garbage from mismatch between viewport and window.
    */
   //if( main_client->local_window.width < (GRAPHICS_SCREEN_WIDTH / GRAPHICS_SPRITE_WIDTH) ) {
   /* graphics_draw_rect(
      g_screen,
      twindow->width * GRAPHICS_SPRITE_WIDTH,
      0,
      ((GRAPHICS_SCREEN_WIDTH / GRAPHICS_SPRITE_WIDTH) - twindow->width)
         * GRAPHICS_SPRITE_WIDTH,
      GRAPHICS_SCREEN_HEIGHT,
      GRAPHICS_COLOR_CHARCOAL,
      TRUE
   ); */
   //}
   //if( twindow->height < (GRAPHICS_SCREEN_HEIGHT / GRAPHICS_SPRITE_HEIGHT) ) {
   graphics_draw_rect(
      g_screen,
      0,
      GRAPHICS_SCREEN_HEIGHT - backlog_height_px,
      GRAPHICS_SCREEN_WIDTH,
      backlog_height_px,
      GRAPHICS_COLOR_CHARCOAL,
      TRUE
   );
   //}

   /* XXX Tilegrid support.
   if( NULL != twindow ) {
      ui_window_draw_tilegrid( ui, twindow );
   } */
   ui_draw( ui, g_screen );

   /* Animations are drawn on top of everything. */
   animate_draw_animations( g_screen );
#endif /* ENABLE_LOCAL_CLIENT */
cleanup:
   return keep_going;
}

static BOOL loop_connect() {
   BOOL keep_going = TRUE;
   bstring server_address = NULL;
   int bstr_result = 0,
      input_res = 0;
   struct VECTOR* server_tuple = NULL;

#ifdef ENABLE_LOCAL_CLIENT

   backlog_close_window( ui );

#ifdef USE_CONNECT_DIALOG
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;

   graphics_clear_screen( g_screen, GRAPHICS_COLOR_CHARCOAL );

   if( NULL == buffer_host ) {
      buffer_host = bfromcstr( "" );
   }
   if( NULL == buffer_channel ) {
      buffer_channel = bfromcstr( "" );
   }

   if(
      !showed_readme &&
      NULL == ui_window_by_id( ui, &str_readme_id )
   ) {
      /* Show the readme first thing. */
      ui_window_new(
         ui, win, &str_readme_id,
         &str_readme_title, NULL,
         -1, -1, 300, 400
      );

      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_HTML, TRUE, TRUE, NULL,
         -1, -1, -1, -1
      );
      ui_control_add( win, &str_readme_id, control );

      ui_window_push( ui, win );
   } else if(
      showed_readme &&
      NULL == ui_window_by_id( ui, &str_cdialog_id )
   ) {
#endif /* USE_CONNECT_DIALOG */

#ifdef USE_RANDOM_PORT
      if( FALSE == ipc_is_listening( main_server->self.link ) ) {
         server_port = 30000 + rng_max( 30000 );
      } else {
         /* Join the running server in progress by default. */
         server_get_port( main_server );
      }
#endif /* USE_RANDOM_PORT */

      /* TODO: Add fields for these to connect dialog. */
      bstr_result = bassigncstr( main_client->nick, "TestNick" );
      scaffold_check_nonzero( bstr_result );
      bstr_result = bassigncstr( main_client->realname, "Tester Tester" );
      scaffold_check_nonzero( bstr_result );
      bstr_result = bassigncstr( main_client->username, "TestUser" );
      scaffold_check_nonzero( bstr_result );

#ifdef USE_CONNECT_DIALOG

      scaffold_print_debug( &module, "Creating connect dialog...\n" );

      /* Prompt for an address and port. */
      ui_window_new(
         ui, win, &str_cdialog_id,
         &str_cdialog_title, &str_cdialog_prompt,
         -1, -1, -1, -1
      );

      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, TRUE, TRUE, buffer_host,
         -1, -1, -1, -1
      );
      ui_control_add( win, &str_cid_connect_host, control );

      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, TRUE, TRUE, buffer_channel,
         -1, -1, -1, -1
      );
      ui_control_add( win, &str_cid_connect_channel, control );

      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_TEXT, TRUE, TRUE, main_client->nick,
         -1, -1, -1, -1
      );
      ui_control_add( win, &str_cid_connect_nick, control );

      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_DROPDOWN, TRUE, TRUE, NULL,
         -1, -1, -1, -1
      );
      main_client->gfx_mode = MODE_TOPDOWN;
      control->list = mode_list;
      control->self.attachment = &(main_client->gfx_mode);
      main_client->gfx_mode = MODE_TOPDOWN;
      ui_control_add( win, &str_cid_connect_gfxmode, control );

      ui_window_push( ui, win );
      bstr_result =
         bassignformat( buffer_host, "%s:%d", bdata( &str_localhost ), server_port );
      bstr_result = bassign( buffer_channel, &str_default_channel );
      scaffold_check_nonzero( bstr_result );
   }

   ui_draw( ui, g_screen );
   //graphics_draw_line( g_screen, 200, 0, 200, 200, GRAPHICS_COLOR_RED );
   input_get_event( input );

   if(
      INPUT_TYPE_CLOSE == input->type || (
         INPUT_TYPE_KEY == input->type &&
         INPUT_SCANCODE_ESC == input->scancode
      )
   ) {
      /* FIXME */
      return FALSE;
   }


   /* Notice that we are polling against two different cases below. If we poll
    * the connect dialog and get something, that means it's open and in focus.
    * Same for the readme dialog. Note the dialog_id param to ui_poll_input().
    */
   input_res = ui_poll_input( ui, input, &str_readme_id );
   if( UI_INPUT_RETURN_KEY_ENTER == input_res  ) {
      showed_readme = TRUE;
      ui_window_destroy( ui, &str_readme_id );
      goto cleanup;
   }

   input_res = ui_poll_input( ui, input, &str_cdialog_id );
   if(
      UI_INPUT_RETURN_KEY_ENTER == input_res ||
      0 < input_res
   ) {
      scaffold_print_info(
         &module, "Connecting to: %b\n",
         buffer_host
      );
      /* Dismiss the connect dialog. */
      ui_window_destroy( ui, &str_cdialog_id );

      /* Split up the address and port. */
      server_tuple = bgsplit( buffer_host, ':' );
      if( 2 < vector_count( server_tuple ) ) {
         scaffold_print_error( &module, "Invalid host string.\n" );
         goto cleanup;
      }
      server_address = vector_get( server_tuple, 0 );
      server_port = bgtoi( vector_get( server_tuple, 1 ) );
      if( 0 >= server_port ) {
         scaffold_print_error( &module, "Invalid port string.\n" );
         goto cleanup;
      }
#else
      /* TODO: Specify these via command line or config. */
      server_address = &str_localhost;
      buffer_channel = &str_default_channel;
#endif /* USE_CONNECT_DIALOG */

      if( FALSE == ipc_is_listening( main_server->self.link ) ) {
         if( FALSE == server_listen( main_server, server_port ) ) {
            goto cleanup;
         }
      }
      scaffold_print_debug( &module, "Listening on port: %d\n", server_port );

#ifdef USE_RANDOM_PORT
      if( NULL == str_service ) {
         str_service = bformat( "Port: %d", server_port );
         scaffold_check_null( str_service );
      }
      ui_debug_window( ui, &str_wid_debug_ip, str_service );
#endif /* USE_RANDOM_PORT */

      client_connect( main_client, server_address, server_port );

#ifdef USE_CONNECT_DIALOG
      goto cleanup;
   }
#else
      /* One successful connection. */
      loop_count++;
#endif /* USE_CONNECT_DIALOG */

#endif /* ENABLE_LOCAL_CLIENT */

cleanup:
   if( NULL != server_tuple ) {
      vector_remove_cb( server_tuple, callback_free_strings, NULL );
      vector_free( &server_tuple );
   }
   return keep_going; /* TODO: ESC to quit. */
}

static BOOL loop_master() {
   BOOL retval = FALSE;
   BOOL connected = FALSE;
   uint16_t main_client_joined = 0;
   int bstr_ret;

#ifdef ENABLE_LOCAL_CLIENT
   graphics_start_fps_timer();

   connected = client_connected( main_client );
   main_client_joined = main_client->flags & CLIENT_FLAGS_SENT_CHANNEL_JOIN;

   if(
      FALSE == connected
#ifndef USE_CONNECT_DIALOG
      && 0 >= loop_count
#endif /* !USE_CONNECT_DIALOG */
   ) {
      retval = loop_connect();
#ifndef USE_CONNECT_DIALOG
   } else if( FALSE == connected && 0 < loop_count ) {
      retval = FALSE;
#endif /* !USE_CONNECT_DIALOG */
   } else if( connected && (!main_client_joined) ) {
      scaffold_print_debug(
         &module, "Server connected; joining client to channel...\n"
      );
      main_client->ui = ui;
      main_client->local_window.g = g_screen;
      client_set_active_t( main_client, NULL );
      proto_client_join( main_client, buffer_channel );
      retval = TRUE;

#ifdef DEBUG_FPS
      ui_debug_window( ui, &str_wid_debug_fps, graphics_fps );
#endif /* DEBUG_FPS */
   } else {
#endif /* ENABLE_LOCAL_CLIENT */
      retval = loop_game();
#ifdef ENABLE_LOCAL_CLIENT
   }

   graphics_flip_screen( g_screen );

   graphics_wait_for_fps_timer();

#ifdef DEBUG_FPS
   if( NULL == graphics_fps ) {
      graphics_fps = bfromcstr( "" );
   }
   bstr_ret = bassignformat(
      graphics_fps, "FPS: %f\n", calc_fps( graphics_sample_fps_timer() )
   );
   scaffold_check_nonzero( bstr_ret );
#endif /* DEBUG_FPS */

#endif /* ENABLE_LOCAL_CLIENT */

cleanup:
   return retval;
}

#ifdef _WIN32
int CALLBACK WinMain(
   _In_ HINSTANCE hInstance,
   _In_ HINSTANCE hPrevInstance,
   _In_ LPSTR     lpCmdLine,
   _In_ int       nShowCmd
) {
#else
int main( int argc, char** argv ) {
#endif /* _WIN32 */
#ifdef SCAFFOLD_LOG_FILE
   scaffold_log_handle = fopen( "stdout.log", "w" );
   scaffold_log_handle_err = fopen( "stderr.log", "w" );
#endif /* SCAFFOLD_LOG_FILE */

#ifdef ENABLE_LOCAL_CLIENT
   g_screen = mem_alloc( 1, GRAPHICS );
   scaffold_check_null( g_screen );

#ifdef _WIN32
   graphics_screen_new(
      &g_screen, GRAPHICS_SCREEN_WIDTH, GRAPHICS_SCREEN_HEIGHT,
      GRAPHICS_VIRTUAL_SCREEN_WIDTH, GRAPHICS_VIRTUAL_SCREEN_HEIGHT,
      nShowCmd, hInstance
   );
#else
   graphics_screen_new(
      &g_screen, GRAPHICS_SCREEN_WIDTH, GRAPHICS_SCREEN_HEIGHT,
      0, 0, 0, NULL
   );
#endif /* _WIN32 */

   scaffold_check_nonzero( scaffold_error );

   graphics_set_window_title( g_screen, &str_title, NULL );

   input = mem_alloc( 1, struct INPUT );
   scaffold_check_null( input );
   input_init( input );
   ui_init( g_screen );
   ui = ui_get_local();
   backlog_init();
   animate_init();
   rng_init();

   ipc_setup();
   scaffold_check_nonzero( scaffold_error );

   proto_setup();
   scaffold_check_nonzero( scaffold_error );

#endif /* ENABLE_LOCAL_CLIENT */

   server_new( main_server, &str_localhost );

#ifdef ENABLE_LOCAL_CLIENT
   scaffold_set_client();
   client_new( main_client );
   client_set_local( main_client, TRUE );
#endif /* ENABLE_LOCAL_CLIENT */

   while( loop_master() );

cleanup:
   proto_shutdown();
   ipc_shutdown();
   animate_shutdown();
#ifdef USE_CONNECT_DIALOG
   bdestroy( buffer_host );
   bdestroy( buffer_channel );
#endif /* USE_CONNECT_DIALOG */
#ifdef ENABLE_LOCAL_CLIENT
#ifndef DISABLE_MODE_POV
   if( NULL != main_client && NULL != main_client->z_buffer ) {
      mem_free( main_client->z_buffer );
   }
#endif /* !DISABLE_MODE_POV */
   input_shutdown( input );
   mem_free( input );
   backlog_shutdown();
   ui_cleanup( ui );
   scaffold_set_client();
   client_free( main_client );
#endif /* ENABLE_LOCAL_CLIENT */
   scaffold_set_server();
   server_free( main_server );
#ifdef ENABLE_LOCAL_CLIENT
#ifdef USE_RANDOM_PORT
   bdestroy( str_service );
#endif /* USE_RANDOM_PORT */
   graphics_shutdown( g_screen );
   /* mem_free( g_screen ); */
#endif /* ENABLE_LOCAL_CLIENT */
#ifdef SCAFFOLD_LOG_FILE
   fclose( scaffold_log_handle );
   fclose( scaffold_log_handle_err );
#endif /* SCAFFOLD_LOG_FILE */

#if defined( _WIN32 ) && defined( DEBUG )
   _CrtDumpMemoryLeaks();
#endif /* _WIN32 && DEBUG */

   return 0;
}
#if defined( USE_ALLEGRO ) && defined( END_OF_MAIN )
END_OF_MAIN();
#endif /* USE_ALLEGRO */
