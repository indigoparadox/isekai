
#include <stdlib.h>
#include <check.h>
#include <stdio.h>

//#include "../src/ipc/syncbuff.h"
#include "check_data.h"

extern struct tagbstring str_key_also1;
extern struct tagbstring str_key_also2;
extern struct tagbstring str_key_also3;

void check_syncbuff_setup_unchecked() {
}

void check_syncbuff_setup_checked() {
   ipc_setup();
   ipc_listen();
   ipc_connect();
}

void check_syncbuff_teardown_checked() {

}

void check_syncbuff_teardown_unchecked() {

}

START_TEST( test_syncbuff_write_to_server ) {
   bstring buffer = NULL;
   ssize_t res = 0;
   struct CONNECTION* ipc_test;

   ipc_test = ipc_alloc();

   buffer = bfromcstralloc( 80, "" );

   res = ipc_write( ipc_test, &str_key_also1, VFALSE );
   ck_assert_int_eq( res, str_key_also1.slen );
   ck_assert_int_eq( 1, syncbuff_get_count( VFALSE ) );
   res = ipc_write( ipc_test, &str_key_also2, VFALSE );
   ck_assert_int_eq( res, str_key_also2.slen );
   ck_assert_int_eq( 2, syncbuff_get_count( VFALSE ) );

   mem_free( ipc_test );
   bdestroy( buffer );
}
END_TEST

START_TEST( test_syncbuff_write_to_client ) {
   bstring buffer = NULL;
   ssize_t res = 0;
   struct CONNECTION* ipc_test;

   ipc_test = ipc_alloc();

   buffer = bfromcstralloc( 80, "" );

   res = ipc_write( ipc_test, &str_key_also1, VTRUE );
   ck_assert_int_eq( res, str_key_also1.slen );
   res = ipc_write( ipc_test, &str_key_also2, VTRUE );
   ck_assert_int_eq( res, str_key_also2.slen );

   mem_free( ipc_test );
   bdestroy( buffer );
}
END_TEST

START_TEST( test_syncbuff_read_from_server ) {
   bstring buffer = NULL;
   ssize_t res = 0;
   const char* bdata_c = NULL;
   struct CONNECTION* ipc_test;

   ipc_test = ipc_alloc();

   buffer = bfromcstralloc( 80, "" );

   res = ipc_read( ipc_test, buffer, VTRUE );
   bdata_c = bdata( buffer );
   ck_assert_str_eq( bdata_c, (const char*)str_key_also1.data );
   ck_assert_int_eq( res, str_key_also1.slen );
   ck_assert_int_eq( 1, syncbuff_get_count( VTRUE ) );

   res = ipc_read( ipc_test, buffer, VTRUE );
   bdata_c = bdata( buffer );
   ck_assert_str_eq( bdata_c, (const char*)str_key_also2.data );
   ck_assert_int_eq( res, str_key_also2.slen );
   ck_assert_int_eq( 0, syncbuff_get_count( VTRUE ) );

   res = ipc_read( ipc_test, buffer, VTRUE );
   ck_assert_int_eq( res, 0 );

   mem_free( ipc_test );
   bdestroy( buffer );
}
END_TEST

START_TEST( test_syncbuff_read_from_client ) {
   bstring buffer = NULL;
   ssize_t res = 0;
   struct CONNECTION* ipc_test;

   ipc_test = ipc_alloc();

   buffer = bfromcstralloc( 80, "" );

   res = ipc_read( ipc_test, buffer, VFALSE );
   ck_assert_int_eq( res, str_key_also1.slen );
   res = ipc_read( ipc_test, buffer, VFALSE );
   ck_assert_int_eq( res, str_key_also2.slen );
   res = ipc_read( ipc_test, buffer, VFALSE );
   ck_assert_int_eq( res, 0 );

   mem_free( ipc_test );
   bdestroy( buffer );
}
END_TEST

Suite* syncbuff_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Syncbuff" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_checked_fixture( tc_core, check_syncbuff_setup_checked, check_syncbuff_teardown_checked );
   tcase_add_unchecked_fixture( tc_core, check_syncbuff_setup_unchecked, check_syncbuff_teardown_unchecked );
   tcase_add_test( tc_core, test_syncbuff_write_to_server );
   tcase_add_test( tc_core, test_syncbuff_write_to_client );
   tcase_add_test( tc_core, test_syncbuff_read_from_server );
   tcase_add_test( tc_core, test_syncbuff_read_from_client );
   suite_add_tcase( s, tc_core );

   return s;
}
