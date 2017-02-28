
#include <stdlib.h>
#include <check.h>#include <stdio.h>

#include "../src/chunker.h"#include "check_data.h"

START_TEST( test_chunker_create ) {
   CHUNKER* h = NULL;
   bstring filename = NULL;

   filename = bfromcstr( "testchannel.tmx" );

   //chunker_new( h, 1024, 76 );

   //void chunker_set_cb( CHUNKER* h, CHUNKER_CALLBACK cb, MAILBOX* m, void* arg );

   //chunker_chunk( h, 1, filename, NULL, 0 );

cleanup:
   bdestroy( filename );
   //chunker_cleanup( h );
}
END_TEST

START_TEST( test_chunker_file ) {
   CHUNKER* h = NULL;
   size_t progress_prev = 0;
   FILE* f = NULL;
   bstring filename = NULL;

   filename = bfromcstr( "testchannel.tmx" );

   //chunker_new( h, 1024, 76 );

   //void chunker_set_cb( CHUNKER* h, CHUNKER_CALLBACK cb, MAILBOX* m, void* arg );

   //chunker_chunk( h, 1, filename, NULL, 0 );
   /*
   while( h->progress < h->src_len ) {
      progress_prev = h->progress;
      chunker_chew( h );
      ck_assert_int_ne( progress_prev, h->progress );
   }      */

cleanup:
   bdestroy( filename );
   //chunker_new( h, 1024, 76 );
}
END_TEST

Suite* chunker_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Chunker" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_test( tc_core, test_chunker_create );
   tcase_add_test( tc_core, test_chunker_file );
   //suite_add_tcase( s, tc_core );

   return s;
}
