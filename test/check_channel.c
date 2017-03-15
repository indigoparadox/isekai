

#include <stdlib.h>
#include <check.h>

#include "../src/client.h"
#include "../src/server.h"
#include "../src/channel.h"

#define CHECK_CHANNEL_CLIENT_COUNT 3

static struct tagbstring module = bsStatic( "check_channel.c" );

static struct tagbstring localhost = bsStatic( "127.0.0.1" );
static struct tagbstring testchannel = bsStatic( "#testchannel" );

struct SERVER server;
struct CLIENT clients[CHECK_CHANNEL_CLIENT_COUNT];
uint32_t port;

void check_channel_setup_checked() {
   int i;
   bstring nick = NULL,
      uname = NULL,
      rname = NULL;
   BOOL connected = FALSE;

   nick = bfromcstr( "" );
   uname = bfromcstr( "" );
   rname = bfromcstr( "" );

   port = (rand() % 45000) + 20000;

   server_init( &server, &localhost );
   server_listen( &server, port );

   for( i = 0 ; CHECK_CHANNEL_CLIENT_COUNT > i ; i++ ) {
      /* Setup the client and connect. */
      scaffold_set_client();
      client_init( &clients[i], TRUE );
      bassignformat( nick, "Test Nick %d", i );
      bassignformat( uname, "Test Username %d", i );
      bassignformat( rname, "Test Real Name %d", i );
      client_set_names( &clients[i], nick, uname, rname );
      do {
         client_connect( &clients[i], &localhost, port );
         graphics_sleep( 100 );
      } while( 0 != scaffold_error );

      /* Receive the connection. */
      connected = FALSE;
      do {
         scaffold_set_server();
         connected = server_poll_new_clients( &server );
      } while( FALSE == connected );
      do {
         scaffold_set_server();
         server_service_clients( &server );
         scaffold_set_client();
         client_update( &clients[i], NULL );
      } while( i + 1 < vector_count( &(server.clients) ) );
   }

cleanup:
   bdestroy( nick );
   bdestroy( uname );
   bdestroy( rname );
   return;
}

void check_channel_teardown_checked() {
   int i;

   for( i = CHECK_CHANNEL_CLIENT_COUNT - 1 ; 0 <= i ; i-- ) {
      scaffold_set_client();
      client_stop( &clients[i] );
      do {
         scaffold_set_server();
         server_service_clients( &server );
         scaffold_set_client();
         client_update( &clients[i], NULL );
      } while( 0 < i && i <= vector_count( &(server.clients) ) );
   }

   scaffold_set_server();
   while( TRUE == server_service_clients( &server ) );
}

void check_channel_setup_unchecked() {
   scaffold_print_info( &module, "====== BEGIN CHANNEL TRACE ======\n" );
}

void check_channel_teardown_unchecked() {
   scaffold_print_info( &module, "====== END CHANNEL TRACE ======\n" );
}

START_TEST( test_channel_server_channel ) {
   int i;

   assert( 0 == vector_count( &(server.self.channels) ) );

   for( i = 0 ; CHECK_CHANNEL_CLIENT_COUNT > i ; i++ ) {

      /* Join the channel. */
      scaffold_set_client();
      client_join_channel( &clients[i], &testchannel  );

      /* Finish server processing. */
      scaffold_set_server();
      while( TRUE == server_service_clients( &server ) );

      assert( 1 == vector_count( &(server.self.channels) ) );
   }

   /* struct CHANNEL* l = server_add_channel( &server, channel, client );
   assert( 1 == vector_count( &(server->self.channels) ) );
   assert( 1 == vector_count( &(l->clients) ) );
   assert( 2 == vector_count( &(server->clients) ) );
   struct CHANNEL* l_b = server_add_channel( server, channel, client_b );
   assert( 1 == vector_count( &(server->self.channels) ) );
   assert( l == l_b );
   assert( 2 == vector_count( &(l->clients) ) );
   assert( 2 == vector_count( &(l_b->clients) ) );
   assert( 2 == vector_count( &(server->clients) ) );
   server_drop_client( server, client_b->nick );
   assert( 1 == vector_count( &(server->self.channels) ) );
   assert( l == l_b );
   assert( 1 == vector_count( &(server->clients) ) ); */
}
END_TEST

Suite* channel_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Channel" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_unchecked_fixture(
      tc_core, check_channel_setup_unchecked, check_channel_teardown_unchecked );
   tcase_add_checked_fixture(
      tc_core, check_channel_setup_checked, check_channel_teardown_checked );
   tcase_add_test( tc_core, test_channel_server_channel );
   suite_add_tcase( s, tc_core );

   return s;
}

