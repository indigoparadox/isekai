
#include <check.h>

#include "../src/vector.h"
#include "check_data.h"

static struct tagbstring module = bsStatic( "check_vector.c" );

START_TEST( test_vector_create ) {
   struct VECTOR* v;

   vector_new( v );

   ck_assert_int_eq( vector_count( v ), 0 );

cleanup:

   vector_cleanup( v );
}
END_TEST

START_TEST( test_vector_add ) {
   struct VECTOR* v;
   BLOB* blob = NULL;
   int i;

   vector_new( v );

   ck_assert_int_eq( vector_count( v ), 0 );

   /* Add 3 blobs. */
   for( i = 0 ; 3 > i ; i++ ) {
      blob = create_blob( 1212, 16, 3, 4545 );
      ck_assert_int_eq( 1, blob->refcount.count );

      vector_add( v, blob );
      ck_assert_int_eq( 2, blob->refcount.count );
   }

   /* Verify count of 3 blobs. */
   ck_assert_int_eq( vector_count( v ), 3 );

   /* Remove front 2 blobs. */
   for( i = 0 ; 2 > i ; i++ ) {
      blob = (BLOB*)vector_get( v, i );
      ck_assert_int_eq( 2, blob->refcount.count );

      vector_remove( v, i );
      ck_assert_int_eq( 1, blob->refcount.count );

      /* Delete the blob. */
      free_blob( blob );
   }

   /* Verify count of 1 blobs. */
   ck_assert_int_eq( vector_count( v ), 1 );

   /* Delete the remaining blob. */
   blob = (BLOB*)vector_get( v, 0 );
   vector_remove( v, 0 );
   free_blob( blob );

   vector_cleanup( v );

cleanup:
   return;
}
END_TEST

START_TEST( test_vector_add_scalar ) {
   struct VECTOR* v;
   int i;

   vector_new( v );

   ck_assert_int_eq( vector_count( v ), 0 );

   for( i = 0 ; 3 > i ; i++ ) {
      vector_add_scalar( v, 3, TRUE );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 0 ; 3 > i ; i++ ) {
      vector_add_scalar( v, 3 * i, FALSE );
   }

   ck_assert_int_eq( vector_count( v ), 5 );

   for( i = 0 ; 3 > i ; i++ ) {
      vector_add_scalar( v, 2 * i, TRUE );
   }

   ck_assert_int_eq( vector_count( v ), 8 );

   for( i = 0 ; 8 > i ; i++ ) {
      vector_remove_scalar( v, 0 );
   }

   ck_assert_int_eq( vector_count( v ), 0 );

cleanup:

   vector_cleanup( v );
}
END_TEST

START_TEST( test_vector_get ) {
   struct VECTOR* v;
   BLOB* blob = NULL;

   vector_new( v );

   ck_assert_int_eq( vector_count( v ), 0 );

   blob = create_blob( 12345, 12, 12, 3456 );
   vector_add( v, blob );

   ck_assert_int_eq( vector_count( v ), 1 );

   blob = create_blob( 4567, 12, 12, 3456 );
   vector_add( v, blob );
   ck_assert_int_eq( vector_count( v ), 2 );


   ck_assert_int_eq( ((BLOB*)vector_get( v, 0 ))->sentinal_start, 12345 );
   ck_assert_int_eq( ((BLOB*)vector_get( v, 1 ))->sentinal_start, 4567 );

cleanup:

   free_blob( (BLOB*)vector_get( v, 0 ) );
   free_blob( (BLOB*)vector_get( v, 1 ) );
   vector_remove( v, 0 );
   vector_remove( v, 0 );
   vector_cleanup( v );
}
END_TEST

START_TEST( test_vector_delete ) {
   struct VECTOR* v;
   BLOB* blob = NULL;
   int i;

   vector_new( v );

   ck_assert_int_eq( vector_count( v ), 0 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = create_blob( 10101 * i, 12, 12, 1010 );
      vector_add( v, blob );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 2 ; 0 <= i ; i-- ) {
      blob = (BLOB*)vector_get( v, i );
      vector_remove( v, i );
      ck_assert_int_eq( blob->sentinal_start, 10101 * i );
      free_blob( blob );
   }

   ck_assert_int_eq( vector_count( v ), 0 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = create_blob( 10101 * i, 12, 12, 1010 );
      vector_add( v, blob );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = (BLOB*)vector_get( v, 0 );
      vector_remove( v, 0 );
      ck_assert_int_eq( blob->sentinal_start, 10101 * i );
      free_blob( blob );
   }

   ck_assert_int_eq( vector_count( v ), 0 );

cleanup:

   vector_cleanup( v );
}
END_TEST

Suite* vector_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Vector" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_vector_create );
   tcase_add_test( tc_core, test_vector_add );
   tcase_add_test( tc_core, test_vector_add_scalar );
   tcase_add_test( tc_core, test_vector_get );
   tcase_add_test( tc_core, test_vector_delete );
   suite_add_tcase( s, tc_core );

   return s;
}
