#include <stdlib.h>
#include <check.h>
#include <stdio.h>

#include "../src/chunker.h"
#include "../src/vector.h"
#include "check_data.h"

#include <unistd.h>

static struct tagbstring module = bsStatic( "check_chunker.c" );

struct tagbstring chunker_test_map_filename = bsStatic( "testdata/server/testchan.tmx" );
struct tagbstring chunker_test_img_filename = bsStatic( "testdata/server/images/terrain.bmp" );
struct tagbstring chunker_test_cachepath = bsStatic( "testdata/cache" );

struct bstrList* chunker_mapchunks = NULL;
struct VECTOR* chunker_mapchunk_starts = NULL;
BYTE* chunker_mapdata = NULL;
SCAFFOLD_SIZE chunker_mapsize;

struct bstrList* chunker_imgchunks = NULL;
struct VECTOR* chunker_imgchunk_starts = NULL;
BYTE* chunker_imgdata = NULL;
SCAFFOLD_SIZE chunker_imgsize;

void check_chunker_setup_unchecked() {
   bstring cache_file_path = NULL;
   FILE* cache_file;
   int bstr_res;

   scaffold_print_info( &module, "====== BEGIN CHUNKER TRACE ======\n" );

   cache_file_path = bstrcpy( &chunker_test_cachepath );
   scaffold_join_path( cache_file_path, (const bstring)&chunker_test_map_filename );
   cache_file = fopen( cache_file_path->data, "r" );
   if( NULL != cache_file ) {
      /* Delete it! */
      scaffold_print_info( &module, "Deleting cached test tilemap...\n" );
      fclose( cache_file );
      cache_file = NULL;
      unlink( (char*)(cache_file_path->data) );
   }

   bstr_res = bassign( cache_file_path, &chunker_test_cachepath );
   scaffold_check_nonzero( bstr_res );
   scaffold_join_path( cache_file_path, &chunker_test_img_filename );
   cache_file = fopen( cache_file_path->data, "r" );
   if( NULL != cache_file ) {
      /* Delete it! */
      scaffold_print_info( &module, "Deleting cached test image...\n" );
      fclose( cache_file );
      cache_file = NULL;
      unlink( (char*)(cache_file_path->data) );
   }

   /* Prepare the original data. */
   files_read_contents(
      &chunker_test_map_filename, &chunker_mapdata, &chunker_mapsize );
   files_read_contents(
      &chunker_test_img_filename, &chunker_imgdata, &chunker_imgsize );

   /* Prepare the chunk list. */
   chunker_mapchunks = bstrListCreate();
   chunker_mapchunk_starts = (struct VECTOR*)calloc( 1, sizeof( struct VECTOR ) );
   vector_init( chunker_mapchunk_starts );
   if( NULL == chunker_mapchunks || NULL == chunker_mapchunk_starts ) {
      ck_abort_msg( "Unable to create testing map chunks list." );
   }

   /* Prepare the chunk list. */
   chunker_imgchunks = bstrListCreate();
   chunker_imgchunk_starts = (struct VECTOR*)calloc( 1, sizeof( struct VECTOR ) );
   vector_init( chunker_imgchunk_starts );
   if( NULL == chunker_imgchunks || NULL == chunker_imgchunk_starts ) {
      ck_abort_msg( "Unable to create testing image chunks list." );
   }

cleanup:
   return;
}

void check_chunker_chunk_checked(
   const bstring filename,
   CHUNKER_DATA_TYPE type,
   BYTE* data_source,
   SCAFFOLD_SIZE data_source_len,
   struct bstrList* chunks,
   struct VECTOR* v_starts,
   SCAFFOLD_SIZE chunk_len
) {
   bstring chunk_buffer = NULL;
   SCAFFOLD_SIZE previous_pos = 0,
      * current_pos = 0;
   struct CHUNKER* h = NULL;
   int bstr_res;

   /* Verify sanity. */
   ck_assert( NULL != data_source );
   ck_assert_int_ne( 0, data_source_len );

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
      h, type, data_source, data_source_len,
      chunk_len
   );

   while( TRUE != chunker_chunk_finished( h ) ) {
      current_pos = (SCAFFOLD_SIZE*)calloc( 1, sizeof( SCAFFOLD_SIZE ) );
      *current_pos = h->raw_position;
      ck_assert( *current_pos <= h->raw_length );
      vector_add( v_starts, current_pos );
      chunker_chunk_pass( h, chunk_buffer );
      if( TRUE != chunker_chunk_finished( h ) ) {
         if( previous_pos == h->raw_position ) {
            ck_abort_msg( "Chunker position not incrementing." );
         }
         previous_pos = h->raw_position;
      }
      scaffold_list_append_string_cpy( chunks, chunk_buffer );
      bstr_res = btrunc( chunk_buffer, 0 );
      scaffold_check_nonzero( bstr_res );
   }

   /* Verify sanity. */
   ck_assert_int_eq( h->raw_length, data_source_len );
   ck_assert( NULL != chunks );
   if( NULL != chunks ) {
      ck_assert_int_ne( 0, chunks->qty );
   }

cleanup:
   if( NULL != h ) {
      chunker_free( h );
   }
}

void check_chunker_unchunk_checked(
   const bstring filename,
   CHUNKER_DATA_TYPE type,
   BYTE* data_source,
   SCAFFOLD_SIZE data_source_len,
   struct bstrList* chunks,
   struct VECTOR* v_starts,
   SCAFFOLD_SIZE chunk_len
) {
   bstring unchunk_buffer = NULL;
   SCAFFOLD_SIZE chunk_index = 0;
   SCAFFOLD_SIZE* curr_start = NULL;
   SCAFFOLD_SIZE* next_start = NULL;
   SCAFFOLD_SIZE prev_start = 0;
   SCAFFOLD_SIZE chunks_count = 0;
   struct CHUNKER* h = NULL;

   h = (struct CHUNKER*)calloc( 1, sizeof( struct CHUNKER ) );
   ck_assert( NULL != h );

   ck_assert( NULL != chunks );
   ck_assert_int_ne( 0, chunks->qty );
   ck_assert_int_ne( 0, data_source_len );

   chunker_unchunk_start( h, type, filename, &chunker_test_cachepath );

   ck_assert_int_eq( chunks->qty, vector_count( v_starts ) );
   if( chunks->qty != vector_count( v_starts ) ) {
      ck_abort_msg( "Vector and string list size mismatch." );
   }

   while( chunks->qty > chunk_index ) {
      unchunk_buffer = chunks->entry[chunk_index];
      curr_start = (SCAFFOLD_SIZE*)vector_get( v_starts, chunk_index );
      if( NULL == unchunk_buffer ) { break; }
      next_start = (SCAFFOLD_SIZE*)vector_get( v_starts, chunk_index + 1 );
      if( NULL != next_start ) {
         ck_assert_int_ne( *next_start, *curr_start );
         ck_assert( *next_start <= data_source_len );
      }
      ck_assert( *curr_start > prev_start || *curr_start == 0 );
      if( *curr_start >= data_source_len ) {
         break;
      }
      ck_assert_int_eq( FALSE, chunker_unchunk_finished( h ) );
      chunker_unchunk_pass(
         h, unchunk_buffer, *curr_start, data_source_len, chunk_len
      );
      ck_assert_int_eq( data_source_len, h->raw_length );
      chunk_index++;
   }

   chunks_count = h->raw_length / h->tx_chunk_length;
   ck_assert_int_eq( chunk_index, chunks_count + 1 );
   printf( "%d of %d %d-byte chunks complete.\n", chunk_index, chunks_count, h->tx_chunk_length );
   ck_assert_int_eq( TRUE, chunker_unchunk_finished( h ) );

   ck_assert( NULL != h->raw_ptr );
   if( NULL != h->raw_ptr ) {
      ck_assert_int_eq( h->raw_length, data_source_len );
      ck_assert_int_eq(
         0,
         memcmp( h->raw_ptr, data_source, data_source_len )
      );
   }

   if( NULL != h ) {
      chunker_free( h );
   }
}

void check_chunker_teardown_unchecked() {
   /* Get rid of the map data after all tests are complete. */
   if( NULL != chunker_mapdata ) {
      mem_free( chunker_mapdata );
   }
   if( NULL != chunker_imgdata ) {
      mem_free( chunker_imgdata );
   }

   scaffold_print_info( &module, "====== END CHUNKER TRACE ======\n" );
}

START_TEST( test_chunker_chunk_unchunk_tilemap ) {
   check_chunker_chunk_checked(
      &chunker_test_map_filename,
      CHUNKER_DATA_TYPE_TILEMAP,
      chunker_mapdata,
      chunker_mapsize,
      chunker_mapchunks,
      chunker_mapchunk_starts,
      CHUNKER_DEFAULT_CHUNK_SIZE
   );
   check_chunker_unchunk_checked(
      &chunker_test_map_filename,
      CHUNKER_DATA_TYPE_TILEMAP,
      chunker_mapdata,
      chunker_mapsize,
      chunker_mapchunks,
      chunker_mapchunk_starts,
      CHUNKER_DEFAULT_CHUNK_SIZE
   );
}
END_TEST

START_TEST( test_chunker_chunk_unchunk_image ) {
   check_chunker_chunk_checked(
      &chunker_test_img_filename,
      CHUNKER_DATA_TYPE_MOBSPRITES,
      chunker_imgdata,
      chunker_imgsize,
      chunker_imgchunks,
      chunker_imgchunk_starts,
      CHUNKER_DEFAULT_CHUNK_SIZE
   );
   check_chunker_unchunk_checked(
      &chunker_test_img_filename,
      CHUNKER_DATA_TYPE_MOBSPRITES,
      chunker_imgdata,
      chunker_imgsize,
      chunker_imgchunks,
      chunker_imgchunk_starts,
      CHUNKER_DEFAULT_CHUNK_SIZE
   );
}
END_TEST

START_TEST( test_chunker_unchunk_cache_integrity ) {
   bstring cache_file_path = NULL;
   BYTE* cache_file_contents = NULL;
   SCAFFOLD_SIZE cache_file_size = 0;
   int bstr_res;

   cache_file_path = bstrcpy( &chunker_test_cachepath );
   scaffold_join_path( cache_file_path, (const bstring)&chunker_test_map_filename );
   scaffold_error_silent = TRUE;
   files_read_contents(
      cache_file_path,
      &cache_file_contents,
      &cache_file_size
   );
   scaffold_error_silent = FALSE;

   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      ck_abort_msg( "Unable to open cached tilemap file." );
   }
   ck_assert_int_eq( cache_file_size, chunker_mapsize );
   ck_assert_int_eq(
      0,
      memcmp( cache_file_contents, chunker_mapdata, chunker_mapsize )
   );

   mem_free( cache_file_contents );
   cache_file_contents = NULL;
   cache_file_size = 0;

   bstr_res = bassign( cache_file_path, &chunker_test_cachepath );
   scaffold_check_nonzero( bstr_res );
   scaffold_join_path( cache_file_path, (const bstring)&chunker_test_img_filename );
   scaffold_error_silent = TRUE;
   files_read_contents(
      cache_file_path,
      &cache_file_contents,
      &cache_file_size
   );
   scaffold_error_silent = FALSE;

   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      ck_abort_msg( "Unable to open cached image file." );
   }
   ck_assert_int_eq( cache_file_size, chunker_imgsize );
   ck_assert_int_eq(
      0,
      memcmp( cache_file_contents, chunker_imgdata, chunker_imgsize )
   );

cleanup:
   return;
}
END_TEST

#if 0

START_TEST( test_chunker_chunk_unchunk_tilemap_cached ) {
}
END_TEST

START_TEST( test_chunker_chunk_unchunk_image_cached ) {
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
#endif

Suite* chunker_suite( void ) {
   Suite* s;
   TCase* tc_core;

   s = suite_create( "Chunker" );

   /* Core test case */
   tc_core = tcase_create( "Core" );

   tcase_set_timeout( tc_core, 8000 );

   /* tcase_add_checked_fixture(
      tc_core, check_chunker_setup_checked, check_chunker_teardown_checked ); */
   tcase_add_unchecked_fixture(
      tc_core, check_chunker_setup_unchecked, check_chunker_teardown_unchecked
   );
   tcase_add_test( tc_core, test_chunker_chunk_unchunk_tilemap );
   tcase_add_test( tc_core, test_chunker_chunk_unchunk_image );
   tcase_add_test( tc_core, test_chunker_unchunk_cache_integrity );
   suite_add_tcase( s, tc_core );

   return s;
}
