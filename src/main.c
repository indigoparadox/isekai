
#include "server.h"
#include "ui.h"
#include "input.h"
#include "b64.h"
#include "scaffold.h"
#include "callback.h"

#include <stdlib.h>
#include <time.h>

#ifndef _WIN32
#include <unistd.h>
#endif /* _WIN32 */

SERVER* main_server = NULL;
struct CLIENT* main_client = NULL;

static struct tagbstring str_loading = bsStatic( "Loading..." );
static struct tagbstring str_localhost = bsStatic( "127.0.0.1" );
static struct tagbstring str_default_channel = bsStatic( "#testchannel" );
static uint32_t server_port = 33080;

#ifdef USE_ALLEGRO
/* TODO: Is this really defined nowhere? */
void allegro_exit();
#endif /* USE_ALLEGRO */

#ifdef WIN32
int CALLBACK WinMain(
   _In_ HINSTANCE hInstance,
   _In_ HINSTANCE hPrevInstance,
   _In_ LPSTR     lpCmdLine,
   _In_ int       nShowCmd
) {
#else
int main( int argc, char** argv ) {
#endif /* WIN32 */
   bstring buffer = NULL;
   time_t tm = 0;
   GRAPHICS g = { 0 };
   struct INPUT p = { 0 };
   struct UI ui = { 0 };
   struct GRAPHICS_TILE_WINDOW twindow = { 0 };
   struct CHANNEL* l = NULL;
   int bstr_result = 0;
#ifdef USE_RANDOM_PORT
   bstring str_service = NULL;
#endif /* USE_RANDOM_PORT */
#ifdef SCAFFOLD_LOG_FILE
   scaffold_log_handle = fopen( "stdout.log", "w" );
   scaffold_log_handle_err = fopen( "stderr.log", "w" );
#endif /* SCAFFOLD_LOG_FILE */

#ifdef WIN32
   graphics_screen_init( &g, 640, 480, nShowCmd, hInstance );
#else
   graphics_screen_init( &g, 640, 480, 0, NULL );
#endif /* WIN32 */
   scaffold_check_nonzero( scaffold_error );
   input_init( &p );
   ui_init( &ui, &g );

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
   client_new( main_client, TRUE );

   do {
#ifdef USE_RANDOM_PORT
      server_port = 30000 + (rand() % 30000);
      bstr_result = bassignformat( str_service, "Port: %d", server_port );
      scaffold_check_nonzero( bstr_result );
#endif /* USE_RANDOM_PORT */
      server_listen( main_server, server_port );
      graphics_sleep( 100 );
   } while( 0 != scaffold_error );

   bstr_result = bassigncstr( main_client->nick, "TestNick" );
   scaffold_check_nonzero( bstr_result );
   bstr_result = bassigncstr( main_client->realname, "Tester Tester" );
   scaffold_check_nonzero( bstr_result );
   bstr_result = bassigncstr( main_client->username, "TestUser" );
   scaffold_check_nonzero( bstr_result );

   do {
      client_connect( main_client, &str_localhost, server_port );
      graphics_sleep( 100 );
   } while( 0 != scaffold_error );

   client_join_channel( main_client, &str_default_channel );

   twindow.width = GRAPHICS_SCREEN_WIDTH / GRAPHICS_SPRITE_WIDTH;
   twindow.height = GRAPHICS_SCREEN_HEIGHT / GRAPHICS_SPRITE_HEIGHT;
   twindow.g = &g;
   twindow.c = main_client;

   while( TRUE ) {

      graphics_wait_for_fps_timer();

      if( !main_server->self.running ) {
         break;
      }

      if( !main_client->running ) {
         server_stop( main_server );
      }

      server_poll_new_clients( main_server );
      client_update( main_client, &g );
      server_service_clients( main_server );

      /* Do drawing. */
      l = hashmap_get_first( &(main_client->channels) );
      if(
         NULL == l ||
         NULL == main_client->puppet /* ||
         TILEMAP_SENTINAL != l->tilemap.sentinal */
      ) {
         graphics_draw_text(
            &g, GRAPHICS_SCREEN_WIDTH / 2, GRAPHICS_SCREEN_HEIGHT / 2,
            &str_loading
         );
         continue;
      }

      twindow.t = &(l->tilemap);

      /* If there's no puppet then there should be a load screen. */
      scaffold_assert( NULL != main_client->puppet );

      /* If we're on the move then update the window frame. */
      if(
         0 != main_client->puppet->steps_remaining ||
         twindow.max_x == twindow.min_x
      ) {
         tilemap_update_window_ortho( &twindow, main_client );
      }

      tilemap_draw_ortho( &twindow );
      client_poll_input( main_client );
      vector_iterate( &(l->mobiles), callback_draw_mobiles, &twindow );
#ifdef USE_RANDOM_PORT
      graphics_draw_text( &g, 40, 10, str_service );
#endif /* USE_RANDOM_PORT */
      graphics_flip_screen( &g );
   }

cleanup:

   bdestroy( buffer );
#ifdef USE_RANDOM_PORT
   bdestroy( str_service );
#endif /* USE_RANDOM_PORT */
   client_free( main_client );
   server_free( main_server );
   free( main_server );
   /*graphics_shutdown( &g );*/
#ifdef SCAFFOLD_LOG_FILE
   fclose( scaffold_log_handle );
   fclose( scaffold_log_handle_err );
#endif // SCAFFOLD_LOG_FILE
#ifdef USE_ALLEGRO
   allegro_exit();
#endif /* USE_ALLEGRO */

   return 0;
}
#if defined( USE_ALLEGRO ) && defined( END_OF_MAIN )
END_OF_MAIN();
#endif /* USE_ALLEGRO */
