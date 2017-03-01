
#include <stdlib.h>
#include <check.h>

#include "../src/vector.h"
#include "check_data.h"

START_TEST( test_vector_create ) {
   VECTOR* v;

   vector_new( v );

   ck_assert_int_eq( vector_count( v ), 0 );
   //ck_assert_str_eq( money_currency(m), "USD");

cleanup:

   vector_free( v );
}
END_TEST

START_TEST( test_vector_add ) {
   VECTOR* v;
   BLOB* blob = NULL;
   int i;

   vector_new( v );

   ck_assert_int_eq( vector_count( v ), 0 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = create_blob( 1212, 16, 3, 4545 );
      //printf( "Blob ref before: %d\n", blob->refcount.count );
      ck_assert_int_eq( 1, blob->refcount.count );

      vector_add( v, blob );
      //printf( "Blob ref after: %d\n", blob->refcount.count );
      ck_assert_int_eq( 2, blob->refcount.count );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 0 ; 2 > i ; i++ ) {
      blob = (BLOB*)vector_get( v, i );
      ck_assert_int_eq( 2, blob->refcount.count );

      vector_delete( v, i );
      ck_assert_int_eq( 1, blob->refcount.count );

      /* Delete the blob. */
      ref_dec( &(blob->refcount) );
   }

cleanup:

   vector_free( v );
}
END_TEST

START_TEST( test_vector_get ) {
   VECTOR* v;
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

   free( ((BLOB*)vector_get( v, 0 )) );
   free( ((BLOB*)vector_get( v, 1 )) );
   vector_free( v );
}
END_TEST

START_TEST( test_vector_delete ) {
   VECTOR* v;
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
      vector_delete( v, i );
      ck_assert_int_eq( blob->sentinal_start, 10101 * i );
      ref_dec( &(blob->refcount) );
   }

   ck_assert_int_eq( vector_count( v ), 0 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = create_blob( 10101 * i, 12, 12, 1010 );
      vector_add( v, blob );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = (BLOB*)vector_get( v, 0 );
      vector_delete( v, 0 );
      ck_assert_int_eq( blob->sentinal_start, 10101 * i );
      ref_dec( &(blob->refcount) );
   }

   ck_assert_int_eq( vector_count( v ), 0 );
   //ck_assert_str_eq( money_currency(m), "USD");

cleanup:

   vector_free( v );
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
   tcase_add_test( tc_core, test_vector_get );
   tcase_add_test( tc_core, test_vector_delete );
   suite_add_tcase( s, tc_core );

   return s;
}
