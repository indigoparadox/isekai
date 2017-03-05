
#include <stdlib.h>
#include <check.h>

#include "../src/connection.h"
#include "check_data.h"

extern struct tagbstring str_key_also1;
extern struct tagbstring str_key_also2;

struct tagbstring str_localhost = bsStatic( "127.0.0.1" );

CONNECTION* check_con_client = NULL;
CONNECTION* check_con_server = NULL;
CONNECTION* check_con_server_client = NULL;

void check_connection_setup_checked() {
   check_con_server = (CONNECTION*)calloc( 1, sizeof( CONNECTION ) );
   check_con_client = (CONNECTION*)calloc( 1, sizeof( CONNECTION ) );
   check_con_server_client = (CONNECTION*)calloc( 1, sizeof( CONNECTION ) );
}

void check_connection_teardown_fixed() {
   connection_cleanup( check_con_client );
   free( check_con_client );
   check_con_client = NULL;

   connection_cleanup( check_con_server );
   free( check_con_server );
   check_con_server = NULL;

   connection_cleanup( check_con_server_client );
   free( check_con_server_client );
   check_con_server_client = NULL;
}

void check_connection_write( CONNECTION* n, bstring message, BOOL client ) {
#ifdef DEBUG
   scaffold_trace_path = client ? SCAFFOLD_TRACE_CLIENT : SCAFFOLD_TRACE_SERVER;
#endif /* DEBUG */
   connection_write_line( n, message, client );
}

ssize_t check_connection_read( CONNECTION* n, bstring buffer, BOOL client ) {
#ifdef DEBUG
   scaffold_trace_path = client ? SCAFFOLD_TRACE_CLIENT : SCAFFOLD_TRACE_SERVER;
#endif /* DEBUG */
   return connection_read_line( n, buffer, client );
}

START_TEST( test_connection_establish ) {
   connection_listen( check_con_server, 8080 );
   connection_connect( check_con_client, &str_localhost, 8080 );
}
END_TEST

START_TEST( test_connection_chat ) {
   bstring message = NULL;
   ssize_t read_count = 0;

   message = bfromcstralloc( 80, "" );

   connection_listen( check_con_server, 8080 );
   connection_connect( check_con_client, &str_localhost, 8080 );
   connection_register_incoming( check_con_server, check_con_server_client );

   /* Server writes, client reads. */
   check_connection_write( check_con_server_client, &str_key_also2, FALSE );
   read_count = check_connection_read( check_con_client, message, TRUE );
   ck_assert_int_eq( read_count, str_key_also2.slen );
   ck_assert( &str_key_also2 != message );

   /* Client writes, server reads. */
   check_connection_write( check_con_server_client, &str_key_also1, TRUE );
   read_count = check_connection_read( check_con_client, message, FALSE );
   ck_assert_int_eq( read_count, str_key_also1.slen );
   ck_assert( &str_key_also1 != message );

   /* Server writes, client reads. */
   check_connection_write( check_con_server_client, &str_key_also2, FALSE );
   read_count = check_connection_read( check_con_client, message, TRUE );
   ck_assert_int_eq( read_count, str_key_also2.slen );
   ck_assert( &str_key_also2 != message );

   bdestroy( message );
}
END_TEST

START_TEST( test_connection_hammer ) {
   bstring message = NULL;
   ssize_t read_count = 0;
   int i;

   message = bfromcstralloc( 80, "" );

   connection_listen( check_con_server, 8080 );
   connection_connect( check_con_client, &str_localhost, 8080 );
   connection_register_incoming( check_con_server, check_con_server_client );

   /* Server writes, client reads. */
   for( i = 0 ; 100 > i ; i++ ) {
      check_connection_write( check_con_server_client, &str_key_also2, FALSE );
   }
   for( i = 0 ; 100 > i ; i++ ) {
      read_count = check_connection_read( check_con_client, message, TRUE );
      ck_assert_int_eq( read_count, str_key_also2.slen );
      ck_assert( &str_key_also2 != message );
   }

   bdestroy( message );
}
END_TEST

Suite* connection_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Connection" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_checked_fixture( tc_core, check_connection_setup_checked, check_connection_teardown_fixed );
   tcase_add_test( tc_core, test_connection_establish );
   tcase_add_test( tc_core, test_connection_chat );
   tcase_add_test( tc_core, test_connection_hammer );
   suite_add_tcase( s, tc_core );

   return s;
}
