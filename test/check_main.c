
#include <check.h>

#define main_add_test( suite_name ) \
   Suite* s_ ## suite_name = suite_name ## _suite(); \
   SRunner* sr_ ## suite_name = srunner_create( s_ ## suite_name ); \
   srunner_run_all( sr_ ## suite_name, CK_NORMAL ); \
   number_failed += srunner_ntests_failed( sr_ ## suite_name ); \
   srunner_free( sr_ ## suite_name );

int main( void ) {
   int number_failed = 0;

   main_add_test( vector );
   //main_add_test( client );
   main_add_test( b64 );
   //main_add_test( channel );   main_add_test( hashmap );

   return( number_failed == 0 ) ? 0 : 1;
}
