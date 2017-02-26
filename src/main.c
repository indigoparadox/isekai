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
   GAMEDATA d = { 0 };
   UI ui = { 0 };
#ifdef DEBUG_B64
   bstring b64_test = NULL;
   size_t b64_test_len = 0;
   char* b64_test_decode = NULL;
#endif /* DEBUG_B64 */

#if !defined( USE_CURSES ) || (defined( USE_CURSES ) && !defined( DEBUG ))
   graphics_screen_init( &g, 640, 480 );
   scaffold_check_nonzero( scaffold_error );
#endif /* DEBUG */
   input_init( &p );
   ui_init( &ui, &g );

#ifdef DEBUG_B64
   scaffold_print_debug( "Testing Base64:\n" );
   b64_test = bfromcstralloc( 100, "" );
   b64_encode( (BYTE*)"abcdefghijk", 11, b64_test, 20 );
   scaffold_print_debug( "Base64 Encoded: %s\n", bdata( b64_test ) );
   assert( 0 == strncmp( "YWJjZGVmZ2hpams=", bdata( b64_test ), 16 ) );
   b64_test_decode = b64_decode( &b64_test_len, b64_test );
   scaffold_print_debug(
      "Base64 Decoding Got: %s, Length: %d\n", b64_test_decode, b64_test_len
   );
   assert( 0 == strncmp( "abcdefghijk", b64_test_decode, 11 ) );
   free( b64_test_decode );
#endif /* DEBUG_B64 */

   srand( (unsigned)time( &tm ) );

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

   gamedata_init_client( &d, &ui, channel );

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

#ifdef DEBUG_TEST_CHANNELS
   server_service_clients( server );
   assert( 1 == vector_count( &(server->clients) ) );
   CLIENT* client_b = NULL;
   client_new( client_b );
   bdestroy( client_b->nick );
   client_b->nick = bfromcstr( "TestUnit" );
   bdestroy( client_b->realname );
   client_b->realname = bfromcstr( "Unit Tester" );
   bdestroy( client_b->username );
   client_b->username = bfromcstr( "TestUnit" );
   do {
      client_connect( client_b, localhost, 33080 );
      usleep( 1000000 );
   } while( 0 != scaffold_error );
   server_service_clients( server );
   assert( 0 == vector_count( &(server->self.channels) ) );
   CHANNEL* l = server_add_channel( server, channel, client );
   assert( 1 == vector_count( &(server->self.channels) ) );
   assert( 1 == vector_count( &(l->clients) ) );
   assert( 2 == vector_count( &(server->clients) ) );
   CHANNEL* l_b = server_add_channel( server, channel, client_b );
   assert( 1 == vector_count( &(server->self.channels) ) );
   assert( l == l_b );
   assert( 2 == vector_count( &(l->clients) ) );
   assert( 2 == vector_count( &(l_b->clients) ) );
   assert( 2 == vector_count( &(server->clients) ) );
   server_drop_client( server, client_b->nick );
   assert( 1 == vector_count( &(server->self.channels) ) );
   assert( l == l_b );
   //assert( 1 == vector_count( &(l->clients) ) );
   //assert( 1 == vector_count( &(l_b->clients) ) );
   assert( 1 == vector_count( &(server->clients) ) );
#endif /* DEBUG */

   graphics_set_color_ex( &g, 255, 255, 255, 255 );

   client_join_channel( client, channel );

   while( TRUE ) {

      graphics_sleep( 50 );

      client_update( client, &d );
      server_service_clients( server );

      if( 'q' == input_get_char( &p ) ) {
         server_stop( server );
      }

      if( 0 < vector_count( &(client->channels) ) ) {
         graphics_draw_text( &g, 20, 20, ((CHANNEL*)(vector_get( &(client->channels),
                                          0 )))->name );
         graphics_draw_text( &g, 20, 40, ((CHANNEL*)(vector_get( &(client->channels),
                                          0 )))->topic );
      }

      graphics_flip_screen( &g );

      if( !server->self.running ) {
         break;
      }
   }

   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
   client_leave_channel( client, channel );

cleanup:

   bdestroy( localhost );
   bdestroy( buffer );
   bdestroy( channel );
   client_cleanup( client );
   server_cleanup( server );
   free( server );
   graphics_shutdown( &g );
#ifdef USE_ALLEGRO
   allegro_exit();
#endif /* USE_ALLEGRO */

   return 0;
}
#ifdef USE_ALLEGRO
END_OF_MAIN();
#endif /* USE_ALLEGRO */
