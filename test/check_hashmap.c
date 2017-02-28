
#include <stdlib.h>
#include <check.h>

#include "../src/hashmap.h"typedef struct _BLOB {   int id;} BLOB;

START_TEST( test_hashmap_add_get ) {
   map_t h = NULL;
   bstring filename = NULL,      badcheck = NULL;   BLOB* blob = NULL;   BLOB* test_blob = NULL;
   badcheck = bfromcstr( "testchannel.tmx" );
   filename = bfromcstr( "failcheck" );   blob = (BLOB*)calloc( 1, sizeof( BLOB ) );   blob->id = 12121;
   h = hashmap_new();   hashmap_put( h, filename, blob );   hashmap_get( h, filename, &test_blob );   ck_assert_ptr_ne( NULL, test_blob );   ck_assert_int_eq( test_blob->id, 12121 );

cleanup:   free( blob );
   bdestroy( filename );   bdestroy( badcheck );   hashmap_free( h );
}
END_TESTSTART_TEST( test_hashmap_delete ) {
   map_t h = NULL;
   bstring filename = NULL,      badcheck = NULL,      alsogood = NULL;   BLOB* blob = NULL;   BLOB* test_blob = NULL;
   badcheck = bfromcstr( "testchannel.tmx" );
   filename = bfromcstr( "failcheck" );   blob = (BLOB*)calloc( 1, sizeof( BLOB ) );   blob->id = 12121;
   h = hashmap_new();   hashmap_put( h, filename, blob );   hashmap_get( h, filename, &test_blob );   ck_assert_ptr_ne( NULL, test_blob );   ck_assert_int_eq( test_blob->id, 12121 );

cleanup:   free( blob );
   bdestroy( filename );   bdestroy( badcheck );   hashmap_free( h );
}
END_TEST

Suite* hashmap_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Hashmap" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_hashmap_add_get );
   suite_add_tcase( s, tc_core );

   return s;
}
