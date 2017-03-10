
#include "server.h"
#include "ui.h"
#include "input.h"
#include "b64.h"
#include "scaffold.h"

#include <stdlib.h>
#include <time.h>

#ifndef _WIN32
#include <unistd.h>
#endif /* _WIN32 */

SERVER* main_server = NULL;
struct CLIENT* main_client = NULL;

static struct tagbstring str_loading = bsStatic( "Loading..." );
static struct tagbstring str_localhost = bsStatic( "localhost" );
static struct tagbstring str_default_channel = bsStatic( "#testchannel" );

#ifdef USE_ALLEGRO
/* TODO: Is this really defined nowhere? */
void allegro_exit();
#endif /* USE_ALLEGRO */

int main( int argc, char** argv ) {
   bstring buffer = NULL;
   time_t tm = 0;
   GRAPHICS g = { 0 };
   struct INPUT p = { 0 };
   struct UI ui = { 0 };
#ifdef SCAFFOLD_LOG_FILE
   scaffold_log_handle = fopen( "stdout.log", "w" );
   scaffold_log_handle_err = fopen( "stderr.log", "w" );
#endif /* SCAFFOLD_LOG_FILE */

#if !defined( USE_CURSES ) || (defined( USE_CURSES ) && !defined( DEBUG ))
   graphics_screen_init( &g, 640, 480 );
   scaffold_check_nonzero( scaffold_error );
#endif /* DEBUG */
   input_init( &p );
   ui_init( &ui, &g );

   srand( (unsigned)time( &tm ) );

   graphics_set_color_ex( &g, 255, 255, 255, 255 );
   graphics_draw_text( &g, 20, 20, &str_loading );
   graphics_flip_screen( &g );

   buffer = bfromcstr( "" );

   server_new( main_server, &str_localhost );
   client_new( main_client );

   do {
      server_listen( main_server, 33080 );
      graphics_sleep( 100 );
   } while( 0 != scaffold_error );

   bassigncstr( main_client->nick, "TestNick" );
   bassigncstr( main_client->realname, "Tester Tester" );
   bassigncstr( main_client->username, "TestUser" );

   do {
      client_connect( main_client, &str_localhost, 33080 );
      graphics_sleep( 100 );
   } while( 0 != scaffold_error );

   client_join_channel( main_client, &str_default_channel );

   while( TRUE ) {

      graphics_sleep( 50 );

      /*
      if( !main_client->running ) {
         server_stop( main_server );
      }
      */

      if( !main_server->self.running ) {
         break;
      } else {
         server_poll_new_clients( main_server );
         client_update( main_client );
         server_service_clients( main_server );
         gamedata_update_client( main_client, &g, &ui );
      }
   }

cleanup:

   bdestroy( buffer );
#if 0
   client_free( main_client );
   scaffold_assert( 0 == hashmap_count( &(main_server->self.channels) ) );
#endif
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
