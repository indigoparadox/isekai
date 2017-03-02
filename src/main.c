#include "server.h"

#include "ui.h"
#include "input.h"
#include "b64/b64.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

static SERVER* server;

static struct tagbstring str_loading = bsStatic( "Loading..." );

/* TODO: Handle SIGINT */
void handle_interrupt( int arg ) {
   server->self.running = FALSE;
}

/* TODO: Is this really defined nowhere? */
void allegro_exit();

int main( int argc, char** argv ) {
   CLIENT* client = NULL;
   bstring localhost = NULL,
           buffer = NULL,
           channel = NULL;
   time_t tm = 0;
   GRAPHICS g = { 0 };
   INPUT p = { 0 };
   //GAMEDATA d;
   UI ui = { 0 };

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

   localhost = bfromcstr( "127.0.0.1" );
   channel = bfromcstr( "#testchannel" );
   buffer = bfromcstr( "" );

   server_new( server, localhost );
   client_new( client );

   p.client = client;

   do {
      server_listen( server, 33080 );
      usleep( 1000000 );
   } while( 0 != scaffold_error );

   signal( SIGINT, handle_interrupt );

   //gamedata_init_client( &d, &ui, channel );

   bdestroy( client->nick );
   client->nick = bfromcstr( "TestNick" );
   bdestroy( client->realname );
   client->realname = bfromcstr( "Tester Tester" );
   bdestroy( client->username );
   client->username = bfromcstr( "TestUser" );

   do {
      client_connect( client, localhost, 33080 );
      usleep( 1000000 );
   } while( 0 != scaffold_error );

   client_join_channel( client, channel );

   while( TRUE ) {

      graphics_sleep( 50 );

      server_poll_new_clients( server );
      client_update( client );
      server_service_clients( server );

      if( 'q' == input_get_char( &p ) ) {
         scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
         client_stop( client );
      }

      if( 0 < hashmap_count( &(client->channels) ) ) {
         /*graphics_draw_text( &g, 20, 20, ((CHANNEL*)(vector_get( &(client->channels),
                                          0 )))->name );
         graphics_draw_text( &g, 20, 40, ((CHANNEL*)(vector_get( &(client->channels),
                                          0 )))->topic );*/
      }

      graphics_flip_screen( &g );

      if( !client->running ) {
         server_stop( server );
      }

      if( !server->self.running || 0 >= hashmap_count( &(server->clients) ) ) {
         break;
      }
   }

cleanup:

   bdestroy( localhost );
   bdestroy( buffer );
   bdestroy( channel );
   client_free( client );
   assert( 0 == hashmap_count( &(server->self.channels) ) );
   server_free( server );
   free( server );
   graphics_shutdown( &g );
#ifdef USE_ALLEGRO
   allegro_exit();
#endif /* USE_ALLEGRO */

   return 0;
}
#if defined( USE_ALLEGRO ) && defined( END_OF_MAIN )
END_OF_MAIN();
#endif /* USE_ALLEGRO */
