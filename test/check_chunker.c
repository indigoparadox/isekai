#include <stdlib.h>
#include <check.h>
#include <stdio.h>

#include "../src/chunker.h"
#include "../src/vector.h"
#include "check_data.h"

const struct tagbstring chunker_test_filename = bsStatic( "testchannel.tmx" );
struct bstrList* chunker_mapchunks = NULL;
VECTOR chunker_mapchunk_starts;
//bstring mapdata_original = NULL;
char* chunker_mapdata;
size_t chunker_mapsize;
FILE* chunker_mapfile = NULL;
CHUNKER* h;

void check_chunker_setup_unchecked() {

   /* Read the test data into memory for all tests. */
   chunker_mapfile = fopen( bdata( &chunker_test_filename ), "rb" );
   ck_assert( NULL != chunker_mapfile );
   if( NULL == chunker_mapfile ) {
      ck_abort_msg( "Unable to open testing map file." );
   }

   /* Allocate enough space to hold the map. */
   fseek( chunker_mapfile, 0, SEEK_END );
   chunker_mapsize = ftell( chunker_mapfile );
   if( 0 == chunker_mapsize ) {
      ck_abort_msg( "Testing map file has 0 length." );
   }
   chunker_mapdata = (BYTE*)calloc( chunker_mapsize, sizeof( BYTE ) + 1 ); /* +1 for term. */
   if( NULL == chunker_mapfile ) {
      ck_abort_msg( "Unable to allocate map data buffer." );
   }
   fseek( chunker_mapfile, 0, SEEK_SET );

   /* Read and close the map. */
   fread( chunker_mapdata, sizeof( BYTE ), chunker_mapsize, chunker_mapfile );

   /* Prepare the chunk list. */
   chunker_mapchunks = bstrListCreate();
   vector_init( &(chunker_mapchunk_starts ) );
   if( NULL == chunker_mapchunks ) {
      ck_abort_msg( "Unable to create testing map chunks list." );
   }
}

void check_chunker_setup_checked() {
   bstring chunk_buffer = NULL;
   size_t previous_pos = 0,
      * current_pos = 0;

   /* Setup the chunker. */
   h = (CHUNKER*)calloc( 1, sizeof( CHUNKER ) );
   ck_assert( NULL != h );

   /* Setup the chunking buffer. */
   chunk_buffer = bfromcstralloc( 80, "" );
   ck_assert( NULL != chunk_buffer );
   if( NULL ==  chunk_buffer ) {
      ck_abort_msg( "Unable to allocate chunking buffer." );
   }

   /* Chunk the testing data into the chunk list. */
   chunker_chunk_start(
      h,
      (const bstring)&chunker_test_filename,
      CHUNKER_DATA_TYPE_TILEMAP,
      chunker_mapdata,
      chunker_mapsize,
      240
   );
   while( TRUE != h->finished ) {
      chunker_chunk_pass( h, chunk_buffer );
      if( TRUE != h->finished ) {
         /*
         if( previous_pos == h->raw_position ) {
            ck_abort_msg( "Chunker position not incrementing." );
         }
         */
         previous_pos = h->raw_position;
      }
      scaffold_list_append_string_cpy( chunker_mapchunks, chunk_buffer );
      current_pos = (size_t*)calloc( 1, sizeof( size_t ) );
      *current_pos = h->raw_position;
      vector_add( &(chunker_mapchunk_starts), &current_pos );
   }

   /* Verify sanity. */
   ck_assert( NULL != chunker_mapdata );
   ck_assert_int_ne( 0, chunker_mapsize );
   ck_assert_int_eq( h->raw_length, chunker_mapsize );
   ck_assert( NULL != chunker_mapchunks );
   if( NULL != chunker_mapchunks ) {
      ck_assert_int_ne( 0, chunker_mapchunks->qty );
   }

   /* Setup the chunker for the tests. */
   if( NULL != h ) {
      chunker_free( h );
   }
   h = (CHUNKER*)calloc( 1, sizeof( CHUNKER ) );
   ck_assert( NULL != h );
}

void check_chunker_teardown_checked() {
   /* Dispose of the chunker after each test. */
   chunker_free( h );
}

void check_chunker_teardown_unchecked() {
   /* Get rid of the map data after all tests are complete. */
   if( NULL != chunker_mapdata ) {
      free( chunker_mapdata );
   }

   if( NULL != chunker_mapfile ) {
      fclose( chunker_mapfile );
      chunker_mapfile = NULL;
   }
}

START_TEST( test_chunker_unchunk ) {
   size_t previous_pos = 0;
   bstring unchunk_buffer = NULL;
   size_t chunk_index = 0;
   size_t* unchunk_buffer_pos = NULL;
   size_t* next_start;
   size_t current_chunk_len;

   ck_assert( NULL != chunker_mapchunks );
   ck_assert_int_ne( 0, chunker_mapchunks->qty );
   ck_assert_int_ne( 0, chunker_mapsize );

   chunker_unchunk_start(
      h,
      (const bstring)&chunker_test_filename,
      CHUNKER_DATA_TYPE_TILEMAP,
      chunker_mapsize
   );

   while( chunker_mapchunks->qty > chunk_index ) {
      //ck_assert( chunker_mapchunks->qty > chunk_index );
      unchunk_buffer = chunker_mapchunks->entry[chunk_index];
      unchunk_buffer_pos = (size_t*)vector_get( &(chunker_mapchunk_starts), chunk_index );
      if( NULL == unchunk_buffer ) break;
      next_start = vector_get( &chunker_mapchunk_starts, chunk_index + 1 );
      if( NULL != next_start ) {
         current_chunk_len = *next_start - *unchunk_buffer_pos;
      } else {
         current_chunk_len = chunker_mapsize - *unchunk_buffer_pos;
      }
      chunker_unchunk_pass( h, unchunk_buffer, *unchunk_buffer_pos, current_chunk_len );
      /*if( TRUE != h->finished ) {
         if( previous_pos == h->raw_position ) {
            ck_abort_msg( "Chunker position not incrementing." );
         }
         previous_pos = h->raw_position;
         chunk_index++;
      }*/
      chunk_index++;
   }

   FILE* foo = fopen ("testdata/foo.tmx","wb");
   fwrite(h->raw_ptr, sizeof( char ), h->raw_length, foo);
   fclose( foo);

   ck_assert( NULL != h->raw_ptr );
   if( NULL != h->raw_ptr ) {
      ck_assert_str_eq( chunker_mapdata, (char*)h->raw_ptr );
   }
}
END_TEST

Suite* chunker_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Chunker" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_add_checked_fixture( tc_core, check_chunker_setup_checked, check_chunker_teardown_checked );
   tcase_add_unchecked_fixture( tc_core, check_chunker_setup_unchecked, check_chunker_teardown_unchecked );
   tcase_add_test( tc_core, test_chunker_unchunk );
   suite_add_tcase( s, tc_core );

   return s;
}
