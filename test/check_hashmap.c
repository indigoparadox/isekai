
#include <stdlib.h>
#include <check.h>

#include "../src/hashmap.h"#include "check_data.h"

START_TEST( test_hashmap_add_get ) {
   map_t h = NULL;   BLOB* blob = NULL;   BLOB* test_blob = NULL;
   blob = create_blob( 12121, 65000, 32, 4545 );
   h = hashmap_new();   hashmap_put( h, &str_key_hit, blob );   hashmap_get( h, &str_key_hit, &test_blob );   ck_assert_ptr_ne( NULL, test_blob );   ck_assert_int_eq( test_blob->sentinal_start, 12121 );

cleanup:   free_blob( blob );   hashmap_free( h );
}
END_TESTSTART_TEST( test_hashmap_delete ) {
   map_t h = NULL;
   bstring filename = NULL,      badcheck = NULL,      alsogood = NULL;   BLOB* blob = NULL;   BLOB* test_blob = NULL;
   h = hashmap_new();
   blob = create_blob( 7171, 4350, 32, 4311 );   hashmap_put( h, &str_key_also2, blob );   //free_blob( blob );   blob = create_blob( 12121, 4300, 32, 4545 );   hashmap_put( h, &str_key_hit, blob );   blob = create_blob( 7878, 4300, 32, 4300 );   hashmap_put( h, &str_key_also1, blob );   //free_blob( blob );   hashmap_remove( h, &str_key_also2 );   hashmap_remove( h, &str_key_also1 );   hashmap_get( h, &str_key_hit, &test_blob );   ck_assert_ptr_ne( NULL, test_blob );   ck_assert_int_eq( test_blob->sentinal_start, 12121 );

cleanup:   free_blob( blob );   hashmap_free( h );
}
END_TEST

Suite* hashmap_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Hashmap" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_hashmap_add_get );
   tcase_add_test( tc_core, test_hashmap_delete );
   suite_add_tcase( s, tc_core );

   return s;
}
