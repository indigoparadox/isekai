

#include <stdlib.h>
#include <check.h>

#include "../src/client.h"
#include "../src/server.h"
#include "../src/channel.h"

#if 0
START_TEST( test_channel_server_channel ) {
   server_new( server, localhost );
   client_new( client );

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
}

Suite* channel_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Channel" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_channel_lifecycle );
   suite_add_tcase( s, tc_core );

   return s;
}
#endif

