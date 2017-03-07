
#include <stdlib.h>
#include <check.h>
#include <stdio.h>

#include "../src/ipc/syncbuff.h"
#include "check_data.h"

extern struct tagbstring str_key_also1;
extern struct tagbstring str_key_also2;
extern struct tagbstring str_key_also3;

void check_syncbuff_setup_unchecked() {
}

void check_syncbuff_setup_checked() {
   syncbuff_listen();
   syncbuff_connect();
}

void check_syncbuff_teardown_checked() {

}

void check_syncbuff_teardown_unchecked() {

}

START_TEST( test_syncbuff_write_to_server ) {
   bstring buffer = NULL;
   ssize_t res = 0;

   buffer = bfromcstralloc( 80, "" );

   res = syncbuff_write( &str_key_also1, SYNCBUFF_DEST_SERVER );
   ck_assert_int_eq( res, str_key_also1.slen );
   res = syncbuff_write( &str_key_also2, SYNCBUFF_DEST_SERVER );
   ck_assert_int_eq( res, str_key_also2.slen );

   bdestroy( buffer );
}
END_TEST

START_TEST( test_syncbuff_write_to_client ) {
   bstring buffer = NULL;
   ssize_t res = 0;

   buffer = bfromcstralloc( 80, "" );

   res = syncbuff_write( &str_key_also1, SYNCBUFF_DEST_CLIENT );
   ck_assert_int_eq( res, str_key_also1.slen );
   res = syncbuff_write( &str_key_also2, SYNCBUFF_DEST_CLIENT );
   ck_assert_int_eq( res, str_key_also2.slen );

   bdestroy( buffer );
}
END_TEST

START_TEST( test_syncbuff_read_from_server ) {
   bstring buffer = NULL;
   ssize_t res = 0;

   buffer = bfromcstralloc( 80, "" );

   res = syncbuff_read( buffer, SYNCBUFF_DEST_CLIENT );
   ck_assert_str_eq( bdata( buffer ), str_key_also1.data );
   ck_assert_int_eq( res, str_key_also1.slen );
   res = syncbuff_read( buffer, SYNCBUFF_DEST_CLIENT );
   ck_assert_str_eq( bdata( buffer ), str_key_also2.data );
   ck_assert_int_eq( res, str_key_also2.slen );
   res = syncbuff_read( buffer, SYNCBUFF_DEST_CLIENT );
   ck_assert_int_eq( res, 0 );

   bdestroy( buffer );
}
END_TEST

START_TEST( test_syncbuff_read_from_client ) {
   bstring buffer = NULL;
   ssize_t res = 0;

   buffer = bfromcstralloc( 80, "" );

   res = syncbuff_read( buffer, SYNCBUFF_DEST_SERVER );
   ck_assert_int_eq( res, str_key_also1.slen );
   res = syncbuff_read( buffer, SYNCBUFF_DEST_SERVER );
   ck_assert_int_eq( res, str_key_also2.slen );
   res = syncbuff_read( buffer, SYNCBUFF_DEST_SERVER );
   ck_assert_int_eq( res, 0 );

   bdestroy( buffer );
}
END_TEST

Suite*syncbuff_suite( void ) {
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
