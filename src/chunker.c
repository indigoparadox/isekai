
#define CHUNKER_C
#include "chunker.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "hs/hsdecode.h"
#include "hs/hsencode.h"

#include "scaffold.h"
#include "b64.h"
#include "callback.h"
#include "ref.h"
#include "files.h"

static void chunker_destroy( const struct REF* ref ) {
   struct CHUNKER* h = (struct CHUNKER*)scaffold_container_of( ref, struct CHUNKER, refcount );

   scaffold_print_debug(
      &module, "Destroying chunker for: %s\n", bdata( h->filename )
   );

   if( NULL != h->tracks ) {
      /* Cleanup tracks. */
      vector_remove_cb( h->tracks, callback_free_generic, NULL );
      vector_cleanup( h->tracks );
   }

   if( NULL != h->raw_ptr ) {
      mem_free( h->raw_ptr );
   }

   bdestroy( h->filecache_path );
   bdestroy( h->filename );
   bdestroy( h->serverpath );

#if HEATSHRINK_DYNAMIC_ALLOC
   if( NULL != h->encoder ) {
      heatshrink_encoder_free( h->encoder );
      h->decoder = NULL;
   }
   if( NULL != h->decoder ) {
      heatshrink_decoder_free( h->decoder );
      h->encoder = NULL;
   }
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

   mem_free( h );
}

void chunker_free( struct CHUNKER* h ) {
   refcount_dec( h, "chunker" );
}

static void chunker_chunk_setup_internal(
   struct CHUNKER* h, DATAFILE_TYPE type, SCAFFOLD_SIZE tx_chunk_length
) {

   scaffold_assert( NULL != h );

   if( REF_SENTINAL != h->refcount.sentinal ) {
      ref_init( &(h->refcount), chunker_destroy );
   }

#if HEATSHRINK_DYNAMIC_ALLOC
   if( NULL != h->decoder ) {
      heatshrink_decoder_free( h->decoder );
      h->decoder = NULL;
   }
   if( NULL != h->encoder ) {
      heatshrink_encoder_free( h->encoder );
      h->encoder = NULL;
   }

   h->encoder = heatshrink_encoder_alloc(
      CHUNKER_WINDOW_SIZE,
      CHUNKER_LOOKAHEAD_SIZE
   );
#else
   heatshrink_encoder_reset( &(h->encoder) );
#endif /* HEATSHRINK_DYNAMIC_ALLOC  */

   if( NULL == h->tracks ) {
      vector_new( h->tracks );
   }

   h->force_finish = FALSE;
   h->raw_position = 0;
   h->tx_chunk_length = tx_chunk_length;
   h->type = type;
   h->filecache_path = NULL;
   if( NULL != h->filename ) {
      bdestroy( h->filename );
   }
   h->filename = NULL;
   if( NULL != h->serverpath ) {
      bdestroy( h->serverpath );
   }
   h->serverpath = NULL;
   if( NULL != h->raw_ptr ) {
      mem_free( h->raw_ptr );
   }
   h->raw_ptr = NULL;

   h->last_percent = 0;

cleanup:
   return;
}

/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_chunk_start(
   struct CHUNKER* h, DATAFILE_TYPE type,  void* src_buffer,
   SCAFFOLD_SIZE src_length, SCAFFOLD_SIZE tx_chunk_length
) {
   scaffold_check_null( src_buffer );

   chunker_chunk_setup_internal( h, type, tx_chunk_length );

   h->raw_length = src_length;
   h->raw_ptr = (BYTE*)calloc( src_length, sizeof( BYTE ) );
   memcpy( h->raw_ptr, src_buffer, src_length );

cleanup:
   return;
}

BOOL chunker_chunk_start_file(
   struct CHUNKER* h, DATAFILE_TYPE type, bstring serverpath,
   bstring filepath, SCAFFOLD_SIZE tx_chunk_length
) {
   bstring full_file_path = NULL;
   SCAFFOLD_SIZE_SIGNED bytes_read;
   BOOL read_ok = FALSE;

   chunker_chunk_setup_internal( h, type, tx_chunk_length );

   h->filename = bstrcpy( filepath );
   scaffold_check_null( h->filename );

   h->serverpath = bstrcpy( serverpath );
   scaffold_check_null( h->serverpath );

   full_file_path = bstrcpy( h->serverpath );

   files_join_path( full_file_path, h->filename );
   scaffold_check_null( full_file_path );

   bytes_read =
      files_read_contents( full_file_path, &h->raw_ptr, &h->raw_length );
   scaffold_check_zero( bytes_read, "Zero bytes read from input file." );

   read_ok = TRUE;

cleanup:
   bdestroy( full_file_path );

   return read_ok;
}

SCAFFOLD_SIZE chunker_chunk_pass( struct CHUNKER* h, bstring tx_buffer ) {
   HSE_sink_res sink_res;
   HSE_poll_res poll_res;
   SCAFFOLD_SIZE_SIGNED
      hs_buffer_len = h->tx_chunk_length * 4,
      hs_buffer_pos = 0,
      raw_buffer_len = h->tx_chunk_length,
      start_pos = h->raw_position;
   SCAFFOLD_SIZE
      exhumed = 0,
      consumed = 0;
   uint8_t* hs_buffer = NULL;

   hs_buffer = (uint8_t*)calloc( hs_buffer_len, sizeof( uint8_t ) );
   scaffold_check_null( hs_buffer );

   heatshrink_encoder_reset( chunker_get_encoder( h ) );

   do {
      if( 0 < raw_buffer_len ) {
         /* Make sure we don't go past the end of the buffer! */
         if( h->raw_position + raw_buffer_len > h->raw_length ) {
            raw_buffer_len = h->raw_length - h->raw_position;
         }

         sink_res = heatshrink_encoder_sink(
#if HEATSHRINK_DYNAMIC_ALLOC
            h->encoder,
#else
            &(h->encoder),
#endif /* HEATSHRINK_DYNAMIC_ALLOC */
            &(h->raw_ptr[h->raw_position]),
            raw_buffer_len,
            &consumed
         );

         if( HSER_SINK_OK != sink_res ) {
            break;
         }

         h->raw_position += consumed;
         raw_buffer_len -= consumed;

         if( 0 >= raw_buffer_len ) {
            heatshrink_encoder_finish( chunker_get_encoder( h ) );
         }
      }

      do {
         poll_res = heatshrink_encoder_poll(
            chunker_get_encoder( h ),
            &(hs_buffer[hs_buffer_pos]),
            (hs_buffer_len - hs_buffer_pos),
            &exhumed
         );
         hs_buffer_pos += exhumed;
         scaffold_assert( h->raw_position + raw_buffer_len <= h->raw_length );
      } while( HSER_POLL_MORE == poll_res );
      scaffold_assert( HSER_POLL_EMPTY == poll_res );
   } while( 0 < raw_buffer_len );

   b64_encode( hs_buffer, hs_buffer_pos, tx_buffer, -1 );

cleanup:

   if( NULL != hs_buffer ) {
      mem_free( hs_buffer );
   }
   return start_pos;
}

BOOL chunker_chunk_finished( struct CHUNKER* h ) {
   return (h->raw_position >= h->raw_length) ? TRUE : FALSE;
}

/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_unchunk_start(
   struct CHUNKER* h, DATAFILE_TYPE type,
   const bstring filename, const bstring filecache_path
) {
   char* filename_c = NULL;

   scaffold_assert( NULL != h );
   scaffold_assert( NULL != filename );
   scaffold_assert( NULL == h->raw_ptr );

   if( REF_SENTINAL != h->refcount.sentinal ) {
      ref_init( &(h->refcount), chunker_destroy );
   }

#if HEATSHRINK_DYNAMIC_ALLOC
   if( NULL != h->decoder ) {
      heatshrink_decoder_free( h->decoder );
   }
   if( NULL != h->encoder ) {
      heatshrink_encoder_free( h->encoder );
   }
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

   if( NULL == h->tracks ) {
      vector_new( h->tracks );
   }

   /* TODO: Shouldn't need to check for NULL here since chunker should be     *
    *       destroyed after user.                                             */
   h->force_finish = FALSE;
   h->raw_position = 0;
   h->raw_length = 0;
   h->type = type;
   if( NULL != h->filename ) {
      bdestroy( h->filename );
   }
   h->filename = bstrcpy( filename );

   filename_c = bdata( filename );
   scaffold_check_null( filename_c );

#ifdef USE_FILE_CACHE
   if( NULL != filecache_path && TRUE == files_check_directory( filecache_path ) ) {
      scaffold_print_debug(
         &module,
         "Chunker: Activating cache: %s\n",
         bdata( filecache_path )
      );
      h->filecache_path = bstrcpy( filecache_path );
      chunker_unchunk_check_cache( h );
   } else {
      h->filecache_path = NULL;
   }
#endif /* USE_FILE_CACHE */

cleanup:
   return;
}

void chunker_unchunk_pass(
   struct CHUNKER* h, bstring rx_buffer, SCAFFOLD_SIZE src_chunk_start,
   SCAFFOLD_SIZE src_len, SCAFFOLD_SIZE src_chunk_len
) {
   SCAFFOLD_SIZE consumed = 0,
      exhumed = 0,
      tail_output_alloc = 0,
      tail_output_pos = 0;
   HSD_poll_res poll_res;
   HSD_sink_res sink_res;
   uint8_t* mid_buffer = NULL,
      * tail_output_buffer = NULL;
   CHUNKER_TRACK* track = NULL;
   SCAFFOLD_SIZE mid_buffer_length = blength( rx_buffer ) * 2,
      mid_buffer_pos = 0;
   SCAFFOLD_SIZE_SIGNED verr;
#ifdef DEBUG
   int b64_res = 0;
#endif /* DEBUG */

#if HEATSHRINK_DYNAMIC_ALLOC
   scaffold_assert( NULL == h->encoder );
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

   if( 0 >= h->tx_chunk_length ) {
      h->tx_chunk_length = src_chunk_len;
   } else {
      scaffold_assert( h->tx_chunk_length == src_chunk_len );
   }

#ifdef USE_FILE_CACHE
   if( TRUE == h->force_finish ) {
      goto cleanup;
   } else
#endif
   if( NULL == h->raw_ptr && 0 == h->raw_length ) {
#if HEATSHRINK_DYNAMIC_ALLOC
      scaffold_assert( NULL == h->decoder );
      h->decoder = heatshrink_decoder_alloc(
         mid_buffer_length, /* TODO */
         CHUNKER_WINDOW_SIZE,
         CHUNKER_LOOKAHEAD_SIZE
      );
#endif /* HEATSHRINK_DYNAMIC_ALLOC */
      h->raw_length = src_len;
      h->raw_ptr = (BYTE*)mem_alloc( src_len, BYTE );
   } else {
#if HEATSHRINK_DYNAMIC_ALLOC
      scaffold_assert( NULL != h->decoder );
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

      /* TODO: Delete corrupt files that don't match. */
      scaffold_assert( src_len == h->raw_length );
   }

   mid_buffer = (uint8_t*)mem_alloc( mid_buffer_length, uint8_t );
   scaffold_check_null( mid_buffer );

   h->raw_position = src_chunk_start;

#ifdef DEBUG
   b64_res =
#endif /* DEBUG */
      b64_decode( rx_buffer, mid_buffer, &mid_buffer_length );
   scaffold_assert( 0 == b64_res );

   heatshrink_decoder_reset( chunker_get_decoder( h ) );

   /* Add a tracker to the list. */
   track = (CHUNKER_TRACK*)mem_alloc( 1, CHUNKER_TRACK );
   track->start = src_chunk_start;
   track->length = src_chunk_len;
   verr = vector_add( h->tracks, track );
   if( 0 > verr ) {
      mem_free( track );
      track = NULL;
      goto cleanup;
   }

   /* Sink enough data to fill an outgoing buffer and wait for it to process. */
   do {
      if( 0 < mid_buffer_length ) {
         sink_res = heatshrink_decoder_sink(
            chunker_get_decoder( h ),
            &(mid_buffer[mid_buffer_pos]),
            mid_buffer_length - mid_buffer_pos,
            &consumed
         );

         if( HSDR_SINK_OK != sink_res ) {
            break;
         }

         mid_buffer_pos += consumed;
         mid_buffer_length -= consumed;

         if( 0 >= mid_buffer_length ) {
            heatshrink_decoder_finish( chunker_get_decoder( h ) );
         }
      }

      do {
         poll_res = heatshrink_decoder_poll(
            chunker_get_decoder( h ),
            (uint8_t*)&(h->raw_ptr[h->raw_position]),
            (h->raw_length - h->raw_position),
            &exhumed
         );
         h->raw_position += exhumed;
         scaffold_assert( h->raw_position <= h->raw_length );

      } while( HSDR_POLL_MORE == poll_res && 0 != exhumed );
   } while( 0 < mid_buffer_length );

   scaffold_assert( h->raw_position <= h->raw_length );

   if( HSDR_POLL_MORE == poll_res ) {
      tail_output_alloc = 1;
      tail_output_buffer = (uint8_t*)calloc( tail_output_alloc, sizeof( uint8_t ) );
   }

   while( HSDR_POLL_MORE == poll_res ) {
      poll_res = heatshrink_decoder_poll(
         chunker_get_decoder( h ),
         &(tail_output_buffer[tail_output_pos]),
         tail_output_alloc - tail_output_pos,
         &exhumed
      );
      tail_output_pos += exhumed;
      scaffold_assert( tail_output_pos <= tail_output_alloc );
      if( tail_output_pos == tail_output_alloc ) {
         tail_output_alloc *= 2;
         tail_output_buffer = mem_realloc( tail_output_buffer, tail_output_alloc, BYTE );
      }
   }

   scaffold_assert( h->raw_position <= h->raw_length );
   scaffold_assert( HSDR_POLL_EMPTY == poll_res );

   if( 0 < tail_output_pos ) {
      h->raw_position -= tail_output_pos;
      memcpy( &(h->raw_ptr[h->raw_position]), tail_output_buffer, tail_output_pos );
   }

cleanup:
   if( NULL != mid_buffer ) {
      mem_free( mid_buffer );
   }
   if( NULL != tail_output_buffer ) {
      mem_free( tail_output_buffer );
   }
   return;
}

#ifdef USE_FILE_CACHE

void chunker_unchunk_save_cache( struct CHUNKER* h ) {
   bstring cache_filename = NULL;
   SCAFFOLD_SIZE written;

   cache_filename = bstrcpy( h->filecache_path );
   scaffold_check_silence(); /* Caching disabled is a non-event. */
   scaffold_check_null( cache_filename );

   files_join_path( cache_filename, h->filename );

   written =
      files_write( cache_filename, h->raw_ptr, h->raw_length, TRUE );
   if( 0 >= written ) {
      scaffold_print_error(
         &module, "Error writing cache file: %s\n", bdata( cache_filename )
      );
   }

cleanup:
   scaffold_check_unsilence();
   return;
}

void chunker_unchunk_check_cache( struct CHUNKER* h ) {
   bstring cache_filename = NULL;
   SCAFFOLD_SIZE_SIGNED sz_read = -1;

   cache_filename = bstrcpy( h->filecache_path );
   scaffold_check_null( cache_filename );

   files_join_path( cache_filename, h->filename );
   scaffold_check_nonzero( scaffold_error );

   /* TODO: Compare file hashes. */
   scaffold_error_silent = TRUE;
   sz_read = files_read_contents(
      cache_filename, &(h->raw_ptr), &(h->raw_length)
   );
   scaffold_error_silent = FALSE;
   if( 0 != scaffold_error ) {
      scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS;
      scaffold_print_error(
         &module, "Unable to open: %s\n", bdata(cache_filename )
      );
      goto cleanup;
   }
   scaffold_check_negative( sz_read );

   scaffold_print_debug(
      &module, "Chunker: Cached copy read: %s\n",
      bdata( cache_filename )
   );

   h->force_finish = TRUE;

cleanup:

   switch( scaffold_error ) {
   case SCAFFOLD_ERROR_NEGATIVE:
   case SCAFFOLD_ERROR_OUTOFBOUNDS:
      scaffold_print_error(
         &module, "Chunker: Cache file could not be opened: %s\n",
         bdata( cache_filename )
      );
      break;

   default:
      break;
   }
   bdestroy( cache_filename );
}

#endif /* USE_FILE_CACHE */

BOOL chunker_unchunk_finished( struct CHUNKER* h ) {
   BOOL finished = TRUE;
   CHUNKER_TRACK* prev_track = NULL,
      * iter_track = NULL;
   SCAFFOLD_SIZE i,
      tracks_count;
   BOOL chunks_locked = FALSE;

#ifdef USE_FILE_CACHE
   if( TRUE == h->force_finish ) {
      /* Force finish, probably due to cache. */
      scaffold_print_debug(
         &module, "Chunker: Assuming cached file finished: %s\n",
         bdata( h->filename )
      );
      finished = TRUE;
      goto cleanup;
   }
#endif /* USE_FILE_CACHE */

   /* Ensure chunks are contiguous and complete. */
   vector_sort_cb( h->tracks, callback_sort_chunker_tracks );
   vector_lock( h->tracks, TRUE );
   tracks_count = vector_count( h->tracks );
   chunks_locked = TRUE;
   if( 0  == tracks_count ) {
      finished = FALSE;
      goto cleanup;
   }
   for( i = 0 ; tracks_count > i ; i++ ) {
      prev_track = (CHUNKER_TRACK*)vector_get( h->tracks, i );
      iter_track = (CHUNKER_TRACK*)vector_get( h->tracks, i + 1 );
      if(
         (NULL != iter_track && NULL != prev_track &&
            (prev_track->start + prev_track->length) < iter_track->start) ||
         (NULL == iter_track && NULL != prev_track &&
            (prev_track->start + prev_track->length) < h->raw_length)
      ) {
         finished = FALSE;
      }
   }

#ifdef USE_FILE_CACHE
   /* If the file is complete and the cache is enabled, then do that. */
   if( TRUE == finished ) {
      scaffold_print_debug(
         &module,
         "Chunker: Saving cached copy of finished file: %s\n",
         bdata( h->filename )
      );
      chunker_unchunk_save_cache( h );
   }
#endif /* USE_FILE_CACHE */
cleanup:
   if( FALSE != chunks_locked ){
      vector_lock( h->tracks, FALSE );
   }

   return finished;
}

BOOL chunker_unchunk_cached(struct CHUNKER* h) {
#ifdef USE_FILE_CACHE
   return h->force_finish;
#else
   return FALSE;
#endif /* USE_FILE_CACHE */
}

int8_t chunker_unchunk_percent_progress( struct CHUNKER* h, BOOL force ) {
   SCAFFOLD_SIZE new_percent = 0;
   SCAFFOLD_SIZE current_bytes = 0;
   CHUNKER_TRACK* iter_track;
   SCAFFOLD_SIZE i;

   for( i = 0 ; vector_count( h->tracks ) > i ; i++ ) {
      iter_track = (CHUNKER_TRACK*)vector_get( h->tracks, i );
      current_bytes += iter_track->length;
   }

   if( 0 < h->raw_length ) {
      new_percent = (current_bytes * 100) / h->raw_length;
   }

   if( new_percent > h->last_percent || TRUE == force ) {
      h->last_percent = new_percent;
      return new_percent;
   } else {
      return -1;
   }
}
