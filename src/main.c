
#include "scaffold.h"
#include "input.h"
#include "server.h"
#include "ui.h"
#include "callback.h"

#include <stdlib.h>
#include <time.h>

#ifdef __GNUC__
#include <unistd.h>
#endif /* __GNUC__ */

SCAFFOLD_MODULE( "main.c" );

static struct tagbstring str_cdialog_id = bsStatic( "connect" );
static struct tagbstring str_cdialog_title = bsStatic( "Connect to Server" );
static struct tagbstring str_cdialog_prompt =
   bsStatic( "Connect to [address:port]:" );

#if defined( USE_CONNECT_DIALOG ) && !defined( USE_NETWORK )
#error Connect dialog requires network to be enabled!
#endif /* USE_CONNECT_DIALOG && !USE_NETWORK */

struct SERVER* main_server = NULL;

#ifdef ENABLE_LOCAL_CLIENT
struct CLIENT* main_client = NULL;
GRAPHICS* g_screen = NULL;
struct INPUT* input = NULL;
struct UI* ui = NULL;
struct GRAPHICS_TILE_WINDOW* twindow = NULL;
struct CHANNEL* l = NULL;
bstring buffer = NULL;
#endif /* ENABLE_LOCAL_CLIENT */

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
static struct tagbstring str_title = bsStatic( "ProCIRCd" );
static struct tagbstring str_loading = bsStatic( "Loading..." );
static struct tagbstring str_localhost = bsStatic( "127.0.0.1" );
static struct tagbstring str_default_channel = bsStatic( "#testchan" );
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

   if( !main_server->self.running ) {
      keep_going = FALSE;
      goto cleanup;
   }

#ifdef ENABLE_LOCAL_CLIENT
   if(
      FALSE == server_service_clients( main_server ) &&
      !main_client->running &&
      0 >= vector_count( &(main_server->self.command_queue) )
   ) {
      server_stop( main_server );
   }
#endif /* ENABLE_LOCAL_CLIENT */

   server_poll_new_clients( main_server );

#ifdef ENABLE_LOCAL_CLIENT
   client_update( main_client, g_screen );

   /* Do drawing. */
   l = hashmap_get_first( &(main_client->channels) );
   if(
      NULL == l ||
      NULL == main_client->puppet
   ) {
      client_poll_input( main_client, l, input );
      graphics_draw_text(
         g_screen, GRAPHICS_SCREEN_WIDTH / 2, GRAPHICS_SCREEN_HEIGHT / 2,
         GRAPHICS_TEXT_ALIGN_CENTER, GRAPHICS_COLOR_WHITE,
         GRAPHICS_FONT_SIZE_16, &str_loading
      );
      graphics_flip_screen( g_screen );
      graphics_wait_for_fps_timer();
      goto cleanup;
   } else if( NULL == twindow->t ) {
      twindow->t = &(l->tilemap);
      tilemap_update_window_ortho(
         twindow, main_client->puppet->x, main_client->puppet->y
      );
   }

   /* Client drawing stuff after this. */
   scaffold_set_client();

   /* If we're on the move then update the window frame. */
   if(
      0 != main_client->puppet->steps_remaining ||
      twindow->max_x == twindow->min_x ||
      TILEMAP_REDRAW_ALL == twindow->t->redraw_state
   ) {
      tilemap_update_window_ortho(
         twindow, main_client->puppet->x, main_client->puppet->y
      );
   }

   /* If there's no puppet then there should be a load screen. */
   scaffold_assert( NULL != main_client->puppet );

   client_poll_input( main_client, l, input );
   tilemap_draw_ortho( twindow );
   vector_iterate( &(l->mobiles), callback_draw_mobiles, twindow );

   ui_draw( ui, g_screen );
#endif /* ENABLE_LOCAL_CLIENT */
cleanup:
   return keep_going;
}

static BOOL loop_connect() {
   BOOL keep_going = TRUE;
   bstring server_address = NULL;
   int bstr_result = 0;
#ifdef USE_CONNECT_DIALOG
   struct UI_WINDOW* win = NULL;
   const char* server_port_c = NULL;
   struct bstrList* server_tuple = NULL;

   if( NULL == buffer ) {
      buffer = bfromcstr( "" );
   }



   if( NULL == ui_window_by_id( ui, &str_cdialog_id ) ) {
#endif /* USE_CONNECT_DIALOG */

      /* TODO: Add fields for these to connect dialog. */
      bstr_result = bassigncstr( main_client->nick, "TestNick" );
      scaffold_check_nonzero( bstr_result );
      bstr_result = bassigncstr( main_client->realname, "Tester Tester" );
      scaffold_check_nonzero( bstr_result );
      bstr_result = bassigncstr( main_client->username, "TestUser" );
      scaffold_check_nonzero( bstr_result );

#ifdef USE_CONNECT_DIALOG
      /* Prompt for an address and port. */
      ui_window_new(
         ui, win, UI_WINDOW_TYPE_SIMPLE_TEXT, &str_cdialog_id,
         &str_cdialog_title, &str_cdialog_prompt,
         -1, -1, -1, -1
      );
      ui_window_push( ui, win );
      bstr_result =
         bassignformat( buffer, "%s:%d", bdata( &str_localhost ), server_port );
      scaffold_check_nonzero( bstr_result );
   }

   ui_draw( ui, g_screen );
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

   if( 0 != ui_poll_input( ui, input, buffer, &str_cdialog_id ) ) {
      /* Dismiss the connect dialog. */
      ui_window_destroy( ui, &str_cdialog_id );

      /* Split up the address and port. */
      server_tuple = bsplit( buffer, ':' );
      if( 2 < server_tuple->qty ) {
         bstrListDestroy( server_tuple );
         server_tuple = NULL;
         goto cleanup;
      }
      server_address = server_tuple->entry[0];
      server_port_c = bdata( server_tuple->entry[1] );
      if( NULL == server_port_c ) {
         bstrListDestroy( server_tuple );
         server_tuple = NULL;
         goto cleanup;
      }
      server_port = atoi( server_port_c );
#else
      server_address = &str_localhost;
#endif /* USE_CONNECT_DIALOG */

#ifdef USE_RANDOM_PORT
      if( NULL == str_service ) {
         str_service = bformat( "Port: %d", server_port );
         scaffold_check_null( str_service );
      }
      ui_debug_window( ui, &str_wid_debug_ip, str_service );
#endif /* USE_RANDOM_PORT */

      client_connect( main_client, server_address, server_port );

#ifdef USE_CONNECT_DIALOG
      /* Destroy this after, since server_address is a ptr inside of it. */
      bstrListDestroy( server_tuple );
      server_tuple = NULL;
   }
#endif /* USE_CONNECT_DIALOG */
cleanup:
   return keep_going; /* TODO: ESC to quit. */
#if 0
   if( 0 != scaffold_error ) {
      return TRUE; /* Try again! */
   } else {
      return FALSE;
   }
#endif
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

   if( !connected ) {
      retval = loop_connect();
   } else if( connected && !main_client_joined ) {
      main_client->ui = ui;
      twindow = scaffold_alloc( 1, struct GRAPHICS_TILE_WINDOW );
      twindow->width = GRAPHICS_SCREEN_WIDTH / GRAPHICS_SPRITE_WIDTH;
      twindow->height = GRAPHICS_SCREEN_HEIGHT / GRAPHICS_SPRITE_HEIGHT;
      twindow->g = g_screen;
      twindow->c = main_client;
      twindow->t = NULL;
      client_join_channel( main_client, &str_default_channel );
      main_client->flags |= CLIENT_FLAGS_SENT_CHANNEL_JOIN;
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
   time_t tm = 0;
#ifdef SCAFFOLD_LOG_FILE
   scaffold_log_handle = fopen( "stdout.log", "w" );
   scaffold_log_handle_err = fopen( "stderr.log", "w" );
#endif /* SCAFFOLD_LOG_FILE */

#ifdef ENABLE_LOCAL_CLIENT
   g_screen = scaffold_alloc( 1, GRAPHICS );
   scaffold_check_null( g_screen );

#ifdef _WIN32
   graphics_screen_new(
      g_screen, GRAPHICS_SCREEN_WIDTH, GRAPHICS_SCREEN_HEIGHT,
      GRAPHICS_VIRTUAL_SCREEN_WIDTH, GRAPHICS_VIRTUAL_SCREEN_HEIGHT,
      nShowCmd, hInstance
   );
#else
   graphics_screen_new(
      &g_screen, GRAPHICS_SCREEN_WIDTH, GRAPHICS_SCREEN_HEIGHT,
      GRAPHICS_VIRTUAL_SCREEN_WIDTH, GRAPHICS_VIRTUAL_SCREEN_HEIGHT, 0, NULL
   );
#endif /* _WIN32 */

   scaffold_check_nonzero( scaffold_error );

   graphics_set_window_title( g_screen, &str_title, NULL );

   input = scaffold_alloc( 1, struct INPUT );
   scaffold_check_null( input );
   input_init( input );
   ui = scaffold_alloc( 1, struct UI );
   scaffold_check_null( ui );
   ui_init( ui, g_screen );

#endif /* ENABLE_LOCAL_CLIENT */

   srand( (unsigned)time( &tm ) );

   server_new( main_server, &str_localhost );

#ifdef ENABLE_LOCAL_CLIENT
   scaffold_set_client();
   client_new( main_client, TRUE );
#endif /* ENABLE_LOCAL_CLIENT */

   do {
#ifdef USE_RANDOM_PORT
      server_port = 30000 + (rand() % 30000);
#endif /* USE_RANDOM_PORT */
      server_listen( main_server, server_port );
      graphics_sleep( 100 );
   } while( 0 != scaffold_error );

   scaffold_print_info( &module, "Listening on port: %d\n", server_port );

   while( loop_master() );

cleanup:
   bdestroy( buffer );
#ifdef ENABLE_LOCAL_CLIENT
   scaffold_free( twindow );
   input_shutdown( input );
   scaffold_free( input );
   ui_cleanup( ui );
   scaffold_free( ui );
   scaffold_set_client();
   client_free( main_client );
#endif /* ENABLE_LOCAL_CLIENT */
   scaffold_set_server();
   server_free( main_server );
   scaffold_free( main_server );
#ifdef ENABLE_LOCAL_CLIENT
#ifdef USE_RANDOM_PORT
   bdestroy( str_service );
#endif /* USE_RANDOM_PORT */
   graphics_shutdown( g_screen );
   graphics_free_bitmap( g_screen );
#endif /* ENABLE_LOCAL_CLIENT */
#ifdef SCAFFOLD_LOG_FILE
   fclose( scaffold_log_handle );
   fclose( scaffold_log_handle_err );
#endif /* SCAFFOLD_LOG_FILE */

   return 0;
}
#if defined( USE_ALLEGRO ) && defined( END_OF_MAIN )
END_OF_MAIN();
#endif /* USE_ALLEGRO */
