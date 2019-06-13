
#include <stdlib.h>
#include <check.h>

#include "check_data.h"

#include "../src/client.h"
#include "../src/server.h"
#include "../src/channel.h"
#include "../src/proto.h"

#define CHECK_CHANNEL_CLIENT_COUNT 3
#define CHECK_CHANNEL_CLIENT_CONNECT_COUNT 5


/* static struct tagbstring module = bsStatic( "check_channel.c" ); */

static struct tagbstring localhost = bsStatic( "127.0.0.1" );
static struct tagbstring testchannel = bsStatic( "#testchan" );
static struct tagbstring testmessage = bsStatic( "This is a test message." );
static struct tagbstring testdest = bsStatic( "TestNick2" );

struct SERVER* server;
struct CLIENT* clients[CHECK_CHANNEL_CLIENT_COUNT];
uint32_t port;

void check_channel_setup_checked() {
   int i;
   bstring nick = NULL,
      uname = NULL,
      rname = NULL;
   bool connected = false,
      server_ret = false,
      client_ret = false;
   int attempts = CHECK_CHANNEL_CLIENT_CONNECT_COUNT;

   ipc_setup();
   proto_setup();

   nick = bfromcstr( "" );
   uname = bfromcstr( "" );
   rname = bfromcstr( "" );

   port = (rand() % 40000) + 20000;
   printf( "Server Port: %d\n", port );

   /* server_init( &server, &localhost ); */
   server = server_new( &localhost );

   do {
      server_listen( server, port );
      graphics_sleep( 1000 );
   } while( LGC_ERROR_NONE != lgc_error );

   for( i = 0 ; CHECK_CHANNEL_CLIENT_COUNT > i ; i++ ) {

      lg_color(
         __FILE__, CHECK_BEGIN_END_COLOR,
         "===== BEGIN CLIENT CONNECT: %d =====\n", i
      );

      /* Setup the client and connect. */
      /* memset( &clients[i], '\0', sizeof( struct CLIENT* ) ); */
      clients[i] = client_new();
      scaffold_set_client();
      /* client_init( clients[i] ); */
      client_set_local( clients[i], true );
      bassignformat( nick, "TestNick%d", i );
      bassignformat( uname, "Test Username %d", i );
      bassignformat( rname, "Test Real Name %d", i );
      client_set_names( clients[i], nick, uname, rname );
      attempts = CHECK_CHANNEL_CLIENT_CONNECT_COUNT;
      lg_debug(
         __FILE__,
         "Client %d attempting to connect to: %b:%d\n",
         i,
         &localhost,
         port
      );
      do {
         client_connect( clients[i], &localhost, port );
         //ck_assert_int_ne( 0, &clients[i].link.socket );
         graphics_sleep( 1000 );
         attempts--;
         ck_assert_int_ne( 0, attempts );
      } while( LGC_ERROR_NONE != lgc_error );

      /* Receive the connection. */
      connected = false;
      attempts = CHECK_CHANNEL_CLIENT_CONNECT_COUNT;
      do {
         scaffold_set_server();
         connected = server_poll_new_clients( server );
         attempts--;
         ck_assert_int_ne( 0, attempts );
      } while( false == connected );
      do {
         scaffold_set_server();
         server_ret = server_service_clients( server ) ? true : server_ret;
         scaffold_set_client();
         client_ret = client_update( clients[i], NULL ) ? true : client_ret;
      } while( !server_ret && !client_ret );
      ck_assert( i + 1 == server_get_client_count( server ) );

      lg_color(
         __FILE__, CHECK_BEGIN_END_COLOR,
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
   bool server_ret = false,
      client_ret = false;

   for( i = CHECK_CHANNEL_CLIENT_COUNT - 1 ; 0 <= i ; i-- ) {
      lg_color(
         __FILE__, CHECK_BEGIN_END_COLOR,
         "===== BEGIN CLIENT DISCONNECT: %d =====\n", i
      );
      lg_debug( __FILE__, "Client %d stopping...\n", i );
      scaffold_set_client();
      proto_client_stop( clients[i] );
      do {
         scaffold_set_server();
         server_ret = server_service_clients( server ) ? true : server_ret;
         scaffold_set_client();
         client_ret = client_update( clients[i], NULL ) ? true : client_ret;
      } while( !server_ret && !client_ret );
      //ck_assert( client_connected( &(clients[i]) ) );
      lg_color(
         __FILE__, CHECK_BEGIN_END_COLOR,
         "===== END CLIENT DISCONNECT: %d =====\n", i
      );
   }

   lg_debug( __FILE__, "Server stopping...\n" );
   scaffold_set_server();
   while( false != server_service_clients( server ) );

   ck_assert_int_eq( 0, server_get_channels_count( server ) );

   server_stop( server );
   server_free( server );
}

void check_channel_setup_unchecked() {
   lg_color(
      __FILE__, CHECK_BEGIN_END_COLOR,
      "====== BEGIN CHANNEL TRACE ======\n"
   );
}

void check_channel_teardown_unchecked() {
   lg_color(
      __FILE__, CHECK_BEGIN_END_COLOR,
      "====== END CHANNEL TRACE ======\n"
   );
}

START_TEST( test_channel_server_channel ) {
   int i;
   struct CHANNEL* l = NULL;

   assert( 0 == server_get_channels_count( server ) );

   for( i = 0 ; CHECK_CHANNEL_CLIENT_COUNT > i ; i++ ) {

      /* Join the channel. */
      scaffold_set_client();
      //client_join_channel( &clients[i], &testchannel  );

      /* Finish server processing. */
      scaffold_set_server();
      while( false != server_service_clients( server ) );

      l = server_get_channel_by_name( server, &testchannel );
      ck_assert_ptr_ne( NULL, l );

      ck_assert_int_eq( i + 1, channel_get_clients_count( l ) );

      ck_assert_int_eq( 1, server_get_channels_count( server ) );
   }
}
END_TEST

START_TEST( test_channel_privmsg ) {
   proto_send_msg( clients[0], &testdest, &testmessage );
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

