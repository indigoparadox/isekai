
#include <stdlib.h>
#include <check.h>

#include "../src/b64.h"

static struct tagbstring b64t_testdata = bsStatic( "abcdefghijk" );
static struct tagbstring b64t_result = bsStatic( "YWJjZGVmZ2hpams=" );

START_TEST( test_b64_encode ) {
   bstring b64_test = NULL;
   //SCAFFOLD_SIZE b64_test_len = 0;
   //char* b64_test_decode = NULL;

   //scaffold_print_debug( "Testing Base64:\n" );
   b64_test = bfromcstralloc( 100, "" );
   b64_encode( (BYTE*)(b64t_testdata.data), b64t_testdata.slen, b64_test, 20 );

   //scaffold_print_debug( "Base64 Encoded: %s\n", bdata( b64_test ) );
   //assert( 0 == strncmp( , bdata( b64_test ), 16 ) );
   ck_assert_str_eq( (const char*)b64t_result.data, (const char*)b64_test->data );

//cleanup:
   bdestroy( b64_test );
}
END_TEST

START_TEST( test_b64_decode ) {
   bstring b64_test = NULL;
   unsigned char* outbuffer = calloc( 20, sizeof( char ) );
   SCAFFOLD_SIZE outlen = 20;

   b64_decode( &b64t_result, outbuffer, &outlen );
   /* scaffold_print_debug(
      "Base64 Decoding Got: %s, Length: %d\n", b64_test_decode, b64_test_len
   ); */
   //assert( 0 == strncmp( "abcdefghijk", b64_test_decode, 11 ) );
   ck_assert_str_eq( (const char*)b64t_testdata.data, (char*)outbuffer );

   free( b64_test );
}
END_TEST

Suite* b64_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Base64" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_b64_encode );
   tcase_add_test( tc_core, test_b64_decode );
   suite_add_tcase( s, tc_core );

   return s;
}


