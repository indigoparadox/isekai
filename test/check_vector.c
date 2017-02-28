
#include <stdlib.h>
#include <check.h>

#include "../src/vector.h"

typedef struct _BLOB {
   int sentinal;
} BLOB;

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
      blob = (BLOB*)calloc( 1, sizeof( BLOB ) );
      blob->sentinal = 10101;
      vector_add( v, blob );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = (BLOB*)vector_get( v, i );
      free( blob );
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

   blob = (BLOB*)calloc( 1, sizeof( BLOB ) );
   blob->sentinal = 12345;
   vector_add( v, blob );

   ck_assert_int_eq( vector_count( v ), 1 );

   blob = (BLOB*)calloc( 1, sizeof( BLOB ) );
   blob->sentinal = 4567;
   vector_add( v, blob );
   ck_assert_int_eq( vector_count( v ), 2 );


   ck_assert_int_eq( ((BLOB*)vector_get( v, 0 ))->sentinal, 12345 );
   ck_assert_int_eq( ((BLOB*)vector_get( v, 1 ))->sentinal, 4567 );

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
      blob = (BLOB*)calloc( 1, sizeof( BLOB ) );
      blob->sentinal = 10101 * i;
      vector_add( v, blob );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 2 ; 0 <= i ; i-- ) {
      blob = (BLOB*)vector_get( v, i );
      vector_delete( v, i );
      ck_assert_int_eq( blob->sentinal, 10101 * i );
      free( blob );
   }

   ck_assert_int_eq( vector_count( v ), 0 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = (BLOB*)calloc( 1, sizeof( BLOB ) );
      blob->sentinal = 10101 * i;
      vector_add( v, blob );
   }

   ck_assert_int_eq( vector_count( v ), 3 );

   for( i = 0 ; 3 > i ; i++ ) {
      blob = (BLOB*)vector_get( v, 0 );
      vector_delete( v, 0 );
      ck_assert_int_eq( blob->sentinal, 10101 * i );
      free( blob );
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
