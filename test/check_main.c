#include <check.h>

#include "../src/libgoki.h"

#define main_add_test_proto( suite_name ) \
   Suite* suite_name ## _suite();

main_add_test_proto( ray )
//main_add_test_proto( vector )
main_add_test_proto( client )
main_add_test_proto( b64 )
main_add_test_proto( channel )
//main_add_test_proto( hashmap )
main_add_test_proto( chunker )
#ifdef USE_SYNCBUFF
main_add_test_proto( syncbuff )
#endif /* USE_SYNCBUFF */

#define main_add_test( suite_name ) \
   Suite* s_ ## suite_name = suite_name ## _suite(); \
   SRunner* sr_ ## suite_name = srunner_create( s_ ## suite_name ); \
   srunner_set_fork_status( sr_ ## suite_name, CK_NOFORK ); \
   srunner_run_all( sr_ ## suite_name, CK_NORMAL ); \
   number_failed += srunner_ntests_failed( sr_ ## suite_name ); \
   srunner_free( sr_ ## suite_name );

int main( void ) {
   int number_failed = 0;

   srand( time( NULL ) );

   lg_add_trace_cat( "CLIENT", LG_COLOR_CYAN );
   lg_add_trace_cat( "SERVER", LG_COLOR_GREEN );

   main_add_test( ray );
#ifdef USE_SYNCBUFF
   main_add_test( syncbuff );
#endif /* USE_SYNCBUFF */
   //main_add_test( vector );
   main_add_test( b64 );
   main_add_test( channel );
   main_add_test( client );
   //main_add_test( hashmap );
   main_add_test( chunker );

   return( number_failed == 0 ) ? 0 : 1;
}
