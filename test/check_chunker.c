#include <stdlib.h>
#include <check.h>
#include <stdio.h>

#include "../src/chunker.h"
#include "../src/vector.h"
#include "check_data.h"

const struct tagbstring chunker_test_filename = bsStatic( "testdata/server/testchannel.tmx" );
const struct tagbstring chunker_test_cachepath = bsStatic( "testdata/cache" );

struct bstrList* chunker_mapchunks = NULL;
struct VECTOR* chunker_mapchunk_starts;
char* chunker_mapdata;
SCAFFOLD_SIZE chunker_mapsize;
FILE* chunker_mapfile = NULL;
struct CHUNKER* h;

void check_chunker_setup_unchecked() {

   /* Read the test data into memory for all tests. */
   chunker_mapfile = fopen( (const char*)(chunker_test_filename.data), "rb" );
   ck_assert( NULL != chunker_mapfile );
   if( NULL == chunker_mapfile ) {
      ck_abort_msg( "Unable to open testing map file." );
      return;
   }

   /* Allocate enough space to hold the map. */
   fseek( chunker_mapfile, 0, SEEK_END );
   chunker_mapsize = ftell( chunker_mapfile );
   if( 0 == chunker_mapsize ) {
      ck_abort_msg( "Testing map file has 0 length." );
   }
   chunker_mapdata = (char*)calloc( chunker_mapsize, sizeof( char ) + 1 ); /* +1 for term. */
   if( NULL == chunker_mapfile ) {
      ck_abort_msg( "Unable to allocate map data buffer." );
   }
   fseek( chunker_mapfile, 0, SEEK_SET );

   /* Read and close the map. */
   fread( chunker_mapdata, sizeof( BYTE ), chunker_mapsize, chunker_mapfile );

   /* Prepare the chunk list. */
   chunker_mapchunks = bstrListCreate();
   chunker_mapchunk_starts = (struct VECTOR*)calloc( 1, sizeof( struct VECTOR ) );
   vector_init( chunker_mapchunk_starts );
   if( NULL == chunker_mapchunks || NULL == chunker_mapchunk_starts ) {
      ck_abort_msg( "Unable to create testing map chunks list." );
   }
}

void check_chunker_setup_checked() {
   bstring chunk_buffer = NULL;
   SCAFFOLD_SIZE previous_pos = 0,
      * current_pos = 0;

   /* Setup the chunker. */
   h = (struct CHUNKER*)calloc( 1, sizeof( struct CHUNKER ) );
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
      CHUNKER_DEFAULT_CHUNK_SIZE
   );

   while( TRUE != chunker_chunk_finished( h ) ) {
      current_pos = (SCAFFOLD_SIZE*)calloc( 1, sizeof( SCAFFOLD_SIZE ) );
      *current_pos = h->raw_position;
      ck_assert( *current_pos <= h->raw_length );
      vector_add( chunker_mapchunk_starts, current_pos );
      chunker_chunk_pass( h, chunk_buffer );
      if( TRUE != chunker_chunk_finished( h ) ) {
         if( previous_pos == h->raw_position ) {
            ck_abort_msg( "Chunker position not incrementing." );
         }
         previous_pos = h->raw_position;
      }
      scaffold_list_append_string_cpy( chunker_mapchunks, chunk_buffer );
      btrunc( chunk_buffer, 0 );
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
   h = (struct CHUNKER*)calloc( 1, sizeof( struct CHUNKER ) );
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
   bstring unchunk_buffer = NULL;
   SCAFFOLD_SIZE chunk_index = 0;
   SCAFFOLD_SIZE* curr_start = NULL;
   SCAFFOLD_SIZE* next_start = NULL;
   SCAFFOLD_SIZE current_chunk_len = CHUNKER_DEFAULT_CHUNK_SIZE;
   SCAFFOLD_SIZE prev_start = 0;
   SCAFFOLD_SIZE chunks_count = 0;
#ifdef CHECK_WRITE_FILES
   FILE* check_chunk_out = NULL;
   SCAFFOLD_SIZE i;
#endif /* CHECK_WRITE_FILES */

   ck_assert( NULL != chunker_mapchunks );
   ck_assert_int_ne( 0, chunker_mapchunks->qty );
   ck_assert_int_ne( 0, chunker_mapsize );

   chunker_unchunk_start(
      h,
      (const bstring)&chunker_test_filename,
      CHUNKER_DATA_TYPE_TILEMAP,
      (const bstring)&chunker_test_filename,
      NULL
   );

   ck_assert_int_eq( chunker_mapchunks->qty, vector_count( chunker_mapchunk_starts ) );
   if( chunker_mapchunks->qty != vector_count( chunker_mapchunk_starts ) ) {
      ck_abort_msg( "Vector and string list size mismatch." );
   }

   while( chunker_mapchunks->qty > chunk_index ) {
      unchunk_buffer = chunker_mapchunks->entry[chunk_index];
      curr_start = (SCAFFOLD_SIZE*)vector_get( chunker_mapchunk_starts, chunk_index );
      if( NULL == unchunk_buffer ) { break; }
      next_start = (SCAFFOLD_SIZE*)vector_get( chunker_mapchunk_starts, chunk_index + 1 );
      if( NULL != next_start ) {
         ck_assert_int_ne( *next_start, *curr_start );
         ck_assert( *next_start <= chunker_mapsize );
      }
      ck_assert( *curr_start > prev_start || *curr_start == 0 );
      if( *curr_start >= chunker_mapsize ) {
         break;
      }
      ck_assert_int_eq( FALSE, chunker_unchunk_finished( h ) );
      chunker_unchunk_pass( h, unchunk_buffer, *curr_start, chunker_mapsize, current_chunk_len );
      chunk_index++;
   }

   chunks_count = h->raw_length / h->tx_chunk_length;
   ck_assert_int_eq( chunk_index, chunks_count + 1 );
   printf( "%d of %d %d-byte chunks complete.\n", chunk_index, chunks_count, h->tx_chunk_length );
   ck_assert_int_eq( TRUE, chunker_unchunk_finished( h ) );


checkup:

   ck_assert( NULL != h->raw_ptr );
   if( NULL != h->raw_ptr ) {
      ck_assert_int_eq( h->raw_length, chunker_mapsize );
      ck_assert_int_eq( h->raw_length, strlen( (const char*)h->raw_ptr ) );
      ck_assert_int_eq( 0, strcmp( (const char*)h->raw_ptr, (const char*)chunker_mapdata ) );
   }
}
END_TEST


START_TEST( test_chunker_unchunk_output ) {
   bstring unchunk_buffer = NULL;
   SCAFFOLD_SIZE chunk_index = 0;
   SCAFFOLD_SIZE* curr_start = NULL;
   SCAFFOLD_SIZE* next_start = NULL;
   SCAFFOLD_SIZE current_chunk_len;
   SCAFFOLD_SIZE prev_start = 0;
   FILE* check_chunk_out = NULL;
   SCAFFOLD_SIZE i;

   ck_assert( NULL != chunker_mapchunks );
   ck_assert_int_ne( 0, chunker_mapchunks->qty );
   ck_assert_int_ne( 0, chunker_mapsize );

   chunker_unchunk_start(
      h,
      (const bstring)&chunker_test_filename,
      CHUNKER_DATA_TYPE_TILEMAP,
      (const bstring)&chunker_test_filename,
      (const bstring)&chunker_test_cachepath
   );

   ck_assert_int_eq( chunker_mapchunks->qty, vector_count( chunker_mapchunk_starts ) );
   if( chunker_mapchunks->qty != vector_count( chunker_mapchunk_starts ) ) {
      ck_abort_msg( "Vector and string list size mismatch." );
   }

   while( chunker_mapchunks->qty > chunk_index ) {
      unchunk_buffer = chunker_mapchunks->entry[chunk_index];
      curr_start = (SCAFFOLD_SIZE*)vector_get( chunker_mapchunk_starts, chunk_index );
      if( NULL == unchunk_buffer ) break;
      next_start = (SCAFFOLD_SIZE*)vector_get( chunker_mapchunk_starts, chunk_index + 1 );
      if( NULL != next_start ) {
         current_chunk_len = *next_start - *curr_start;
         ck_assert_int_ne( *next_start, *curr_start );
         ck_assert( *next_start <= chunker_mapsize );
      } else {
         current_chunk_len = chunker_mapsize - *curr_start;
      }
      ck_assert( *curr_start > prev_start || *curr_start == 0 );
      if( *curr_start >= chunker_mapsize ) {
         break;
      }
      ck_assert_int_eq( FALSE, chunker_unchunk_finished( h ) );
      chunker_unchunk_pass( h, unchunk_buffer, *curr_start, chunker_mapsize, current_chunk_len );
      chunk_index++;
   }

   check_chunk_out = fopen( "testdata/check_chunk_out.tmx.b64","w" );
   for( i = 0 ; chunker_mapchunks->qty > i ; i++ ) {
      fwrite( bdata( chunker_mapchunks->entry[i] ), sizeof( char ), blength( chunker_mapchunks->entry[i] ), check_chunk_out );
      fputc( '\n', check_chunk_out );
   }
   fclose( check_chunk_out );

   check_chunk_out = fopen( "testdata/check_chunk_out.tmx","wb" );
   fwrite( h->raw_ptr, sizeof( char ), h->raw_length, check_chunk_out );
   fclose( check_chunk_out );
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
