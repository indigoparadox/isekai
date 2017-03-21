
#include <stdlib.h>
#include <check.h>

#include "check_data.h"

#include "../src/client.h"
#include "../src/server.h"
#include "../src/channel.h"
#include "../src/proto.h"

#define CHECK_CHANNEL_CLIENT_COUNT 3
#define CHECK_CHANNEL_CLIENT_CONNECT_COUNT 5

static struct tagbstring module = bsStatic( "check_channel.c" );

static struct tagbstring localhost = bsStatic( "127.0.0.1" );
static struct tagbstring testchannel = bsStatic( "#testchan" );
static struct tagbstring testmessage = bsStatic( "This is a test message." );
static struct tagbstring testdest = bsStatic( "TestNick2" );

struct SERVER server;
struct CLIENT clients[CHECK_CHANNEL_CLIENT_COUNT];
uint32_t port;

void check_channel_setup_checked() {
   int i;
   bstring nick = NULL,
      uname = NULL,
      rname = NULL;
   BOOL connected = FALSE;
   int attempts = CHECK_CHANNEL_CLIENT_CONNECT_COUNT;

   nick = bfromcstr( "" );
   uname = bfromcstr( "" );
   rname = bfromcstr( "" );

   port = (rand() % 40000) + 20000;
   printf( "Server Port: %d\n", port );

   server_init( &server, &localhost );

   do {
      server_listen( &server, port );
      graphics_sleep( 1000 );
   } while( 0 != scaffold_error );

   for( i = 0 ; CHECK_CHANNEL_CLIENT_COUNT > i ; i++ ) {

      scaffold_print_debug_color(
         &module, CHECK_BEGIN_END_COLOR,
         "===== BEGIN CLIENT CONNECT: %d =====\n", i
      );

      /* Setup the client and connect. */
      scaffold_set_client();
      client_init( &clients[i], TRUE );
      bassignformat( nick, "TestNick%d", i );
      bassignformat( uname, "Test Username %d", i );
      bassignformat( rname, "Test Real Name %d", i );
      client_set_names( &clients[i], nick, uname, rname );
      attempts = CHECK_CHANNEL_CLIENT_CONNECT_COUNT;
      scaffold_print_debug(
         &module,
         "Client %d attempting to connect to: %b:%d\n",
         i,
         &localhost,
         port
      );
      do {
         client_connect( &clients[i], &localhost, port );
         graphics_sleep( 1000 );
         attempts--;
         ck_assert_int_ne( 0, attempts );
      } while( 0 != scaffold_error );

      /* Receive the connection. */
      connected = FALSE;
      attempts = CHECK_CHANNEL_CLIENT_CONNECT_COUNT;
      do {
         scaffold_set_server();
         connected = server_poll_new_clients( &server );
         attempts--;
         ck_assert_int_ne( 0, attempts );
      } while( FALSE == connected );
      do {
         scaffold_set_server();
         server_service_clients( &server );
         scaffold_set_client();
         client_update( &clients[i], NULL );
      } while( i + 1 < hashmap_count( &(server.clients) ) );

      scaffold_print_debug_color(
         &module, CHECK_BEGIN_END_COLOR,
         "===== END CLIENT CONNECT: %d =====\n", i
      );
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
      scaffold_print_debug_color(
         &module, CHECK_BEGIN_END_COLOR,
         "===== BEGIN CLIENT DISCONNECT: %d =====\n", i
      );
      scaffold_print_debug( &module, "Client %d stopping...\n", i );
      scaffold_set_client();
      proto_client_stop( &clients[i] );
      do {
         scaffold_set_server();
         server_service_clients( &server );
         scaffold_set_client();
         client_update( &clients[i], NULL );
      } while( 0 < i && clients[i].running );
      scaffold_print_debug_color(
         &module, CHECK_BEGIN_END_COLOR,
         "===== END CLIENT DISCONNECT: %d =====\n", i
      );
   }

   scaffold_print_debug( &module, "Server stopping...\n" );
   scaffold_set_server();
   while( TRUE == server_service_clients( &server ) );

   ck_assert_int_eq( 0, hashmap_count( &(server.self.channels) ) );

   server_stop( &server );
   server_free( &server );
}

void check_channel_setup_unchecked() {
   scaffold_print_debug_color(
      &module, CHECK_BEGIN_END_COLOR,
      "====== BEGIN CHANNEL TRACE ======\n"
   );
}

void check_channel_teardown_unchecked() {
   scaffold_print_debug_color(
      &module, CHECK_BEGIN_END_COLOR,
      "====== END CHANNEL TRACE ======\n"
   );
}

START_TEST( test_channel_server_channel ) {
   int i;
   struct CHANNEL* l = NULL;

   assert( 0 == hashmap_count( &(server.self.channels) ) );

   for( i = 0 ; CHECK_CHANNEL_CLIENT_COUNT > i ; i++ ) {

      /* Join the channel. */
      scaffold_set_client();
      client_join_channel( &clients[i], &testchannel  );

      /* Finish server processing. */
      scaffold_set_server();
      while( TRUE == server_service_clients( &server ) );

      l = server_get_channel_by_name( &server, &testchannel );
      ck_assert_ptr_ne( NULL, l );

      ck_assert_int_eq( i + 1, hashmap_count( &(l->clients) ) );

      ck_assert_int_eq( 1, hashmap_count( &(server.self.channels) ) );
   }
}
END_TEST

START_TEST( test_channel_privmsg ) {
   proto_send_msg( &clients[0], &testdest, &testmessage );
}
END_TEST

Suite* channel_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Channel" );

   tc_core = tcase_create( "Core" );

   tcase_add_unchecked_fixture(
      tc_core, check_channel_setup_unchecked, check_channel_teardown_unchecked );
   tcase_add_checked_fixture(
      tc_core, check_channel_setup_checked, check_channel_teardown_checked );
   tcase_add_test( tc_core, test_channel_server_channel );
   tcase_add_test( tc_core, test_channel_privmsg );
   suite_add_tcase( s, tc_core );

   return s;
}

