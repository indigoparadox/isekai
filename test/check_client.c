
#include <stdlib.h>
#include <check.h>

#include "../src/client.h"
#include "../src/server.h"

START_TEST( test_client_lifecycle ) {
   //CLIENT* c;

   //client_new( c, NULL );

//cleanup:
//   client_cleanup( c );
}
END_TEST

START_TEST( test_client_server_channel ) {

}
END_TEST

Suite* client_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Client" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_client_lifecycle );
   suite_add_tcase( s, tc_core );

   return s;
}

