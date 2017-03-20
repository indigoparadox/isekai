
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

static struct tagbstring str_cdialog_title = bsStatic( "Connect to Server" );
static struct tagbstring str_cdialog_prompt =
   bsStatic( "Connect to [address:port]:" );

#if defined( USE_CONNECT_DIALOG ) && !defined( USE_NETWORK )
#error Connect dialog requires network to be enabled!
#endif /* USE_CONNECT_DIALOG && !USE_NETWORK */

struct SERVER* main_server = NULL;
struct CLIENT* main_client = NULL;

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
   bstring buffer = NULL;
   time_t tm = 0;
#ifdef ENABLE_LOCAL_CLIENT
   GRAPHICS* g_screen;
   struct INPUT p = { 0 };
   struct UI ui = { 0 };
   struct GRAPHICS_TILE_WINDOW twindow = { 0 };
   struct CHANNEL* l = NULL;
#ifdef USE_CONNECT_DIALOG
   struct UI_WINDOW* win = NULL;
   const char* server_port_c = NULL;
   struct bstrList* server_tuple = NULL;
#endif /* USE_CONNECT_DIALOG */
#endif /* ENABLE_LOCAL_CLIENT */
   int bstr_result = 0;
   bstring server_address = NULL;
   BOOL post_load_finished = FALSE;
#ifdef USE_RANDOM_PORT
   bstring str_service = NULL;
#endif /* USE_RANDOM_PORT */
#ifdef SCAFFOLD_LOG_FILE
   scaffold_log_handle = fopen( "stdout.log", "w" );
   scaffold_log_handle_err = fopen( "stderr.log", "w" );
#endif /* SCAFFOLD_LOG_FILE */

#ifdef ENABLE_LOCAL_CLIENT
#ifdef _WIN32
   graphics_screen_new(
      &g, GRAPHICS_SCREEN_WIDTH, GRAPHICS_SCREEN_HEIGHT,
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

   input_init( &p );
   ui_init( &ui, g_screen );

#endif /* ENABLE_LOCAL_CLIENT */

   srand( (unsigned)time( &tm ) );

   /*
   graphics_set_color_ex( &g, 255, 255, 255, 255 );
   graphics_draw_text( &g, 20, 20, &str_loading );
   graphics_flip_screen( &g );
   */

   buffer = bfromcstr( "" );
#ifdef USE_RANDOM_PORT
   str_service = bfromcstralloc( 8, "" );
#endif /* USE_RANDOM_PORT */

   server_new( main_server, &str_localhost );

#ifdef ENABLE_LOCAL_CLIENT
   scaffold_set_client();
   client_new( main_client, TRUE );
#endif /* ENABLE_LOCAL_CLIENT */

   do {
#ifdef USE_RANDOM_PORT
      server_port = 30000 + (rand() % 30000);
#endif /* USE_RANDOM_PORT */
#if defined( USE_RANDOM_PORT ) || defined( USE_CONNECT_DIALOG )
      bstr_result = bassignformat( str_service, "Port: %d", server_port );
#endif /* USE_RANDOM_PORT || USE_CONNECT_DIALOG */
      scaffold_check_nonzero( bstr_result );
      server_listen( main_server, server_port );
      //graphics_sleep( 100 );
   } while( 0 != scaffold_error );

#ifdef ENABLE_LOCAL_CLIENT
   bstr_result = bassigncstr( main_client->nick, "TestNick" );
   scaffold_check_nonzero( bstr_result );
   bstr_result = bassigncstr( main_client->realname, "Tester Tester" );
   scaffold_check_nonzero( bstr_result );
   bstr_result = bassigncstr( main_client->username, "TestUser" );
   scaffold_check_nonzero( bstr_result );

   main_client->ui = &ui;

   do {
#ifdef USE_CONNECT_DIALOG
      /* Prompt for an address and port. */
      ui_window_new(
         &ui, win, UI_WINDOW_TYPE_SIMPLE_TEXT, NULL,
         &str_cdialog_title, &str_cdialog_prompt,
         -1, -1, -1, -1
      );
      ui_window_push( &ui, win );
      bstr_result =
         bassignformat( buffer, "%s:%d", bdata( &str_localhost ), server_port );
      scaffold_check_nonzero( bstr_result );
      do {
         graphics_start_fps_timer();
         ui_draw( &ui, g_screen );
         input_get_event( &p );
         graphics_flip_screen( g_screen );
         graphics_wait_for_fps_timer();
      } while( 0 == ui_poll_input( &ui, &p, buffer, NULL ) );
      ui_window_pop( &ui );

      /* Split up the address and port. */
      server_tuple = bsplit( buffer, ':' );
      if( 2 < server_tuple->qty ) {
         bstrListDestroy( server_tuple );
         server_tuple = NULL;
         continue;
      }
      server_address = server_tuple->entry[0];
      server_port_c = bdata( server_tuple->entry[1] );
      if( NULL == server_port_c ) {
         bstrListDestroy( server_tuple );
         server_tuple = NULL;
         continue;
      }
      server_port = atoi( server_port_c );
#else
      server_address = &str_localhost;
#endif /* USE_CONNECT_DIALOG */

      client_connect( main_client, server_address, server_port );

#ifdef USE_CONNECT_DIALOG
      /* Destroy this after, since server_address is a ptr inside of it. */
      bstrListDestroy( server_tuple );
      server_tuple = NULL;
#endif /* USE_CONNECT_DIALOG */

      graphics_sleep( 100 );
   } while( 0 != scaffold_error );

   client_join_channel( main_client, &str_default_channel );

   twindow.width = GRAPHICS_SCREEN_WIDTH / GRAPHICS_SPRITE_WIDTH;
   twindow.height = GRAPHICS_SCREEN_HEIGHT / GRAPHICS_SPRITE_HEIGHT;
   twindow.g = g_screen;
   twindow.c = main_client;
   twindow.t = NULL;

#ifdef USE_RANDOM_PORT
   ui_debug_window( &ui, &str_wid_debug_ip, str_service );
#endif /* USE_RANDOM_PORT */

#ifdef DEBUG_FPS
   graphics_debug_fps( &ui );
#endif /* DEBUG_FPS */

#else
   scaffold_print_info( &module, "Listening on port: %d\n", server_port );
#endif /* ENABLE_LOCAL_CLIENT */

   while( TRUE ) {
#ifdef ENABLE_LOCAL_CLIENT
      graphics_start_fps_timer();
#endif /* ENABLE_LOCAL_CLIENT */

      if( !main_server->self.running ) {
         break;
      }

#ifdef ENABLE_LOCAL_CLIENT
      if( !main_client->running ) {
         server_stop( main_server );
      }
#endif /* ENABLE_LOCAL_CLIENT */

      server_poll_new_clients( main_server );
      server_service_clients( main_server );
#ifdef ENABLE_LOCAL_CLIENT
      client_update( main_client, g_screen );

      /* Do drawing. */
      l = hashmap_get_first( &(main_client->channels) );
      if(
         NULL == l ||
         NULL == main_client->puppet
      ) {
         client_poll_input( main_client, l, &p );
         graphics_draw_text(
            g_screen, GRAPHICS_SCREEN_WIDTH / 2, GRAPHICS_SCREEN_HEIGHT / 2,
            GRAPHICS_TEXT_ALIGN_CENTER, GRAPHICS_COLOR_WHITE,
            GRAPHICS_FONT_SIZE_16, &str_loading
         );
         graphics_flip_screen( g_screen );
         graphics_wait_for_fps_timer();
         continue;
      } else if( TRUE != post_load_finished ) {
         twindow.t = &(l->tilemap);
         tilemap_update_window_ortho(
            &twindow, main_client->puppet->x, main_client->puppet->y
         );
         post_load_finished = TRUE;
      }

      /* Client drawing stuff after this. */
      scaffold_set_client();

      /* If we're on the move then update the window frame. */
      if(
         0 != main_client->puppet->steps_remaining ||
         twindow.max_x == twindow.min_x ||
         TILEMAP_REDRAW_ALL == twindow.t->redraw_state
      ) {
         tilemap_update_window_ortho(
            &twindow, main_client->puppet->x, main_client->puppet->y
         );
      }

      /* If there's no puppet then there should be a load screen. */
      scaffold_assert( NULL != main_client->puppet );

      client_poll_input( main_client, l, &p );
      tilemap_draw_ortho( &twindow );
      vector_iterate( &(l->mobiles), callback_draw_mobiles, &twindow );

      ui_draw( &ui, g_screen );

      graphics_flip_screen( g_screen );

      graphics_wait_for_fps_timer();
#endif /* ENABLE_LOCAL_CLIENT */
   }

cleanup:
   bdestroy( buffer );
#ifdef ENABLE_LOCAL_CLIENT
   input_shutdown( &p );
   ui_cleanup( &ui );
#ifdef USE_RANDOM_PORT
   bdestroy( str_service );
#endif /* USE_RANDOM_PORT */
   scaffold_set_client();
   client_free( main_client );
#endif /* ENABLE_LOCAL_CLIENT */
   scaffold_set_server();
   server_free( main_server );
   scaffold_free( main_server );
#ifdef ENABLE_LOCAL_CLIENT
   graphics_shutdown( g_screen );
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
