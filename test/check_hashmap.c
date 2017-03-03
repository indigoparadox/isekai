#include <stdlib.h>
#include <check.h>

#include "../src/hashmap.h"
#include "check_data.h"

extern struct tagbstring str_key_hit;
extern struct tagbstring str_key_also1;
extern struct tagbstring str_key_also2;

START_TEST( test_hashmap_add_get ) {
   HASHMAP h;
   BLOB* blob = NULL;
   BLOB* test_blob = NULL;
   blob = create_blob( 12121, 65000, 32, 4545 );
   hashmap_init( &h );

   hashmap_put( &h, &str_key_hit, blob );

   test_blob = hashmap_get( &h, &str_key_hit );

   //ck_assert_ptr_ne( NULL, test_blob );
   ck_assert_int_eq( test_blob->sentinal_start, 12121 );

   ref_dec( &(test_blob->refcount) );
   hashmap_remove( &h, &str_key_hit );

   /* cleanup: */
   //free_blob( blob );
   hashmap_cleanup( &h );
}
END_TEST

START_TEST( test_hashmap_delete ) {
   HASHMAP h;
   BLOB* blob = NULL;
   BLOB* test_blob = NULL;

   hashmap_init( &h );
   blob = create_blob( 7171, 4350, 32, 4311 );
   hashmap_put( &h, &str_key_also2, blob );
   ck_assert_int_eq( hashmap_count( &h ), 1 );
   //free_blob( blob );
   blob = create_blob( 12121, 4300, 32, 4545 );
   hashmap_put( &h, &str_key_hit, blob );
   ck_assert_int_eq( hashmap_count( &h ), 2 );
   blob = create_blob( 7878, 4300, 32, 4300 );
   hashmap_put( &h, &str_key_also1, blob );
   ck_assert_int_eq( hashmap_count( &h ), 3 );
   //free_blob( blob );

   hashmap_remove( &h, &str_key_also2 );
   ck_assert_int_eq( hashmap_count( &h ), 2 );
   hashmap_remove( &h, &str_key_also1 );
   ck_assert_int_eq( hashmap_count( &h ), 1 );

   test_blob = hashmap_get( &h, &str_key_hit );

   //ck_assert_ptr_ne( NULL, test_blob );
   ck_assert_int_eq( test_blob->sentinal_start, 12121 );

   ref_dec( &(test_blob->refcount) );
   hashmap_remove( &h, &str_key_hit );

   /* cleanup: */
   //free_blob( blob );
   hashmap_cleanup( &h );
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
