
#include "chunker.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "scaffold.h"
#include "b64/b64.h"
#include "callbacks.h"

static void chunker_cleanup( const struct _REF* ref ) {
   CHUNKER* h = (CHUNKER*)scaffold_container_of( ref, CHUNKER, refcount );

   /* Cleanup tracks. */
   vector_remove_cb( &(h->tracks), callback_free_generic, NULL );
   vector_free( &(h->tracks) );

   if( NULL != h->raw_ptr ) {
      free( h->raw_ptr );
   }

   bdestroy( h->filecache_path );
   bdestroy( h->filename );

   free( h );
}

void chunker_free( CHUNKER* h ) {
   ref_dec( &(h->refcount) );
}

static void chunker_chunk_setup_internal(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type, size_t tx_chunk_length
) {

   assert( NULL != h );

   if( REF_SENTINAL != h->refcount.sentinal ) {
      ref_init( &(h->refcount), chunker_cleanup );
   }

   if( NULL != h->decoder ) {
      heatshrink_decoder_free( h->decoder );
   }
   if( NULL != h->encoder ) {
      heatshrink_encoder_free( h->encoder );
   }

   h->encoder = heatshrink_encoder_alloc(
      CHUNKER_WINDOW_SIZE,
      CHUNKER_LOOKAHEAD_SIZE
   );

   if( VECTOR_SENTINAL != h->tracks.sentinal ) {
      vector_init( &(h->tracks) );
   }

   h->force_finish = FALSE;
   h->raw_position = 0;
   h->tx_chunk_length = tx_chunk_length;
   h->channel = bstrcpy( channel );
   h->type = type;
   h->filecache_path = NULL;
   h->filename = NULL;
}

/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_chunk_start(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type,  void* src_buffer,
   size_t src_length, size_t tx_chunk_length
) {
   scaffold_check_null( src_buffer );

   chunker_chunk_setup_internal( h, channel, type, tx_chunk_length );

   h->raw_length = src_length;
   h->raw_ptr = (BYTE*)calloc( src_length, sizeof( BYTE ) );
   memcpy( h->raw_ptr, src_buffer, src_length );

cleanup:
   return;
}

void chunker_chunk_start_file(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type, bstring filepath,
   size_t tx_chunk_length
) {
   scaffold_read_file_contents( filepath, &h->raw_ptr, &h->raw_length );

   chunker_chunk_setup_internal( h, channel, type, tx_chunk_length );
}

void chunker_chunk_pass( CHUNKER* h, bstring tx_buffer ) {
   HSE_sink_res sink_res;
   HSE_poll_res poll_res;
   size_t consumed = 0,
      exhumed = 0,
      hs_buffer_len = h->tx_chunk_length * 4,
      hs_buffer_pos = 0,
      raw_buffer_len = h->tx_chunk_length;
   uint8_t* hs_buffer = NULL;

   hs_buffer = (uint8_t*)calloc( hs_buffer_len, sizeof( uint8_t ) );
   heatshrink_encoder_reset( h->encoder );

   do {
      if( 0 < raw_buffer_len ) {
         /* Make sure we don't go past the end of the buffer! */
         if( h->raw_position + raw_buffer_len > h->raw_length ) {
            raw_buffer_len = h->raw_length - h->raw_position;
         }

         sink_res = heatshrink_encoder_sink(
            h->encoder,
            (uint8_t*)&(h->raw_ptr[h->raw_position]),
            raw_buffer_len,
            &consumed
         );

         if( HSER_SINK_OK != sink_res ) {
            break;
         }

         h->raw_position += consumed;
         raw_buffer_len -= consumed;

         if( 0 >= raw_buffer_len ) {
            heatshrink_encoder_finish( h->encoder );
         }
      }

      do {
         poll_res = heatshrink_encoder_poll(
            h->encoder,
            &(hs_buffer[hs_buffer_pos]),
            (hs_buffer_len - hs_buffer_pos),
            &exhumed
         );
         hs_buffer_pos += exhumed;
         assert( h->raw_position + raw_buffer_len <= h->raw_length );
      } while( HSER_POLL_MORE == poll_res );
      assert( HSER_POLL_EMPTY == poll_res );
   } while( 0 < raw_buffer_len );

   b64_encode( hs_buffer, hs_buffer_pos, tx_buffer, -1 );

   free( hs_buffer );

   return;
}

BOOL chunker_chunk_finished( CHUNKER* h ) {
   return (h->raw_position >= h->raw_length) ? TRUE : FALSE;
}

/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_unchunk_start(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type, size_t src_length,
   const bstring filename, const bstring filecache_path
) {
   assert( NULL != h );
   assert( 0 != src_length );
   assert( NULL != filename );

   if( REF_SENTINAL != h->refcount.sentinal ) {
      ref_init( &(h->refcount), chunker_cleanup );
   }

   if( NULL != h->decoder ) {
      heatshrink_decoder_free( h->decoder );
   }
   if( NULL != h->encoder ) {
      heatshrink_encoder_free( h->encoder );
   }

   if( VECTOR_SENTINAL != h->tracks.sentinal ) {
      vector_init( &(h->tracks) );
   }

   h->decoder = heatshrink_decoder_alloc(
      src_length, /* TODO */
      CHUNKER_WINDOW_SIZE,
      CHUNKER_LOOKAHEAD_SIZE
   );

   h->force_finish = FALSE;
   h->raw_position = 0;
   h->raw_length = src_length;
   h->raw_ptr = (BYTE*)calloc( src_length, sizeof( BYTE ) );
   h->channel = bstrcpy( channel );
   h->type = type;
   h->filename = bstrcpy( filename );

   chunker_unchunk_check_cache( h, filecache_path );
   if( SCAFFOLD_ERROR_NONE == scaffold_error ) {
      scaffold_print_debug(
         "Chunker: Activating cache: %s\n",
         bdata( filecache_path )
      );
      h->filecache_path = bstrcpy( filecache_path );
   } else {
      h->filecache_path = NULL;
   }

/* cleanup: */
   return;
}

void chunker_unchunk_pass( CHUNKER* h, bstring rx_buffer, size_t src_chunk_start, size_t src_chunk_len ) {
   size_t consumed,
      exhumed;
   size_t mid_buffer_length = blength( rx_buffer ) * 2,
      mid_buffer_pos = 0,
      tail_output_alloc = 0,
      tail_output_pos = 0;
   uint8_t* mid_buffer = NULL,
      * tail_output_buffer = NULL;
   HSD_poll_res poll_res;
   HSD_sink_res sink_res;
   int b64_res;
   CHUNKER_TRACK* track = NULL;

   assert( NULL != h->decoder );
   assert( NULL == h->encoder );

   mid_buffer = (uint8_t*)calloc( mid_buffer_length, sizeof( uint8_t ) );
   scaffold_check_null( mid_buffer );

   h->raw_position = src_chunk_start;

   b64_res = b64_decode( rx_buffer, mid_buffer, &mid_buffer_length );
   assert( 0 == b64_res );

   heatshrink_decoder_reset( h->decoder );

   /* Add a tracker to the list. */
   track = (CHUNKER_TRACK*)calloc( 1, sizeof( CHUNKER_TRACK ) );
   track->start = src_chunk_start;
   track->length = src_chunk_len;
   vector_add( &(h->tracks), track );

   /* Sink enough data to fill an outgoing buffer and wait for it to process. */
   do {
      if( 0 < mid_buffer_length ) {
         sink_res = heatshrink_decoder_sink(
            h->decoder,
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
            heatshrink_decoder_finish( h->decoder );
         }
      }

      do {
         poll_res = heatshrink_decoder_poll(
            h->decoder,
            (uint8_t*)&(h->raw_ptr[h->raw_position]),
            (h->raw_length - h->raw_position),
            &exhumed
         );
         h->raw_position += exhumed;
         assert( h->raw_position <= h->raw_length );

      } while( HSDR_POLL_MORE == poll_res && 0 != exhumed );
   } while( 0 < mid_buffer_length );

   assert( h->raw_position <= h->raw_length );

   if( HSDR_POLL_MORE == poll_res ) {
      tail_output_alloc = 1;
      tail_output_buffer = (uint8_t*)calloc( tail_output_alloc, sizeof( uint8_t ) );
   }

   while( HSDR_POLL_MORE == poll_res ) {
      poll_res = heatshrink_decoder_poll(
         h->decoder,
         &(tail_output_buffer[tail_output_pos]),
         tail_output_alloc - tail_output_pos,
         &exhumed
      );
      tail_output_pos += exhumed;
      assert( tail_output_pos <= tail_output_alloc );
      if( tail_output_pos == tail_output_alloc ) {
         tail_output_alloc *= 2;
         tail_output_buffer = (uint8_t*)realloc( tail_output_buffer, tail_output_alloc * sizeof( uint8_t ) );
      }
   }

   assert( h->raw_position <= h->raw_length );
   assert( HSDR_POLL_EMPTY == poll_res );

   if( 0 < tail_output_pos ) {
      h->raw_position -= tail_output_pos;
      memcpy( &(h->raw_ptr[h->raw_position]), tail_output_buffer, tail_output_pos );
   }

cleanup:
   if( NULL != mid_buffer ) {
      free( mid_buffer );
   }
   if( NULL != tail_output_buffer ) {
      free( tail_output_buffer );
   }
   return;
}

#ifdef USE_FILE_CACHE

void chunker_unchunk_save_cache( CHUNKER* h ) {
   bstring cache_filename = NULL;

   cache_filename = bstrcpy( h->filecache_path );
   scaffold_check_silence(); /* Caching disabled is a non-event. */
   scaffold_check_null( cache_filename );

   scaffold_join_path( cache_filename, h->filename );

   scaffold_write_file( cache_filename, h->raw_ptr, h->raw_length, TRUE );

cleanup:
   scaffold_check_unsilence();
   return;
}

void chunker_unchunk_check_cache( CHUNKER* h, bstring filecache_path ) {
   FILE* cached_copy_f;
   struct stat cachedir_info = { 0 };
   char* filecache_path_c = NULL;
   bstring cache_filename = NULL;

   scaffold_error = 0;
   scaffold_check_silence();

   scaffold_check_null( filecache_path );
   filecache_path_c = bdata( filecache_path );
   scaffold_check_nonzero( stat( filecache_path_c, &cachedir_info ) );
   scaffold_check_zero( (cachedir_info.st_mode & S_IFDIR) );

   cache_filename = bstrcpy( filecache_path );
   scaffold_check_unsilence(); /* Hint */
   scaffold_check_null( cache_filename );

   scaffold_join_path( cache_filename, h->filename );
   scaffold_check_null( cache_filename );

   /* TODO: Compared file hashes. */
   cached_copy_f = fopen( bdata( cache_filename ), "rb" );
   if( NULL == cached_copy_f ) {
      /* No scaffold error, since this isn't a problem. */
      scaffold_print_error(
         "Chunker: Unable to open cache file for reading: %s\n",
         bdata( cache_filename )
      );
      goto cleanup;
   }
   scaffold_print_info(
      "Chunker: Cached copy found: %s\n",
      bdata( cache_filename )
   );

   /* Allocate enough space to hold the file. */
   fseek( cached_copy_f, 0, SEEK_END );
   h->raw_length = ftell( cached_copy_f );
   h->raw_ptr = (BYTE*)calloc( h->raw_length, sizeof( BYTE ) + 1 ); /* +1 for term. */
   scaffold_check_null( h->raw_ptr );
   fseek( cached_copy_f, 0, SEEK_SET );

   /* Read and close the cache file. */
   fread( h->raw_ptr, sizeof( uint8_t ), h->raw_length, cached_copy_f );
   fclose( cached_copy_f );
   cached_copy_f = NULL;
   h->force_finish = TRUE;

cleanup:
   scaffold_check_unsilence();
   switch( scaffold_error ) {
   case SCAFFOLD_ERROR_NULLPO:
      scaffold_print_info( "Chunker: Cache directory not set. Ignoring.\n" );
      break;

   case SCAFFOLD_ERROR_NONZERO:
      scaffold_print_error(
         "Chunker: Unable to open cache directory: %s\n",
         bdata( filecache_path )
      );
      break;

   case SCAFFOLD_ERROR_ZERO:
      scaffold_print_error(
         "Chunker: Cache directory is not a directory: %s\n",
         bdata( filecache_path )
      );
      break;

   default:
      break;
   }
}

#endif /* USE_FILE_CACHE */

BOOL chunker_unchunk_finished( CHUNKER* h ) {
   CHUNKER_TRACK* prev_track = NULL,
      * iter_track = NULL;
   size_t i,
      tracks_count;
   BOOL finished = TRUE;

   if( TRUE == h->force_finish ) {
      /* Force finish, probably due to cache. */
      scaffold_print_info(
         "Chunker: Assuming cached file finished: %s\n",
         bdata( h->filename )
      );
      finished = TRUE;
      goto cleanup;
   }

   /* Ensure chunks are contiguous and complete. */
   vector_sort_cb( &(h->tracks), callback_sort_chunker_tracks );
   vector_lock( &(h->tracks), TRUE );
   tracks_count = vector_count( &(h->tracks) );
   if( 0  == tracks_count ) {
      finished = FALSE;
      goto cleanup;
   }
   for( i = 0 ; tracks_count > i ; i++ ) {
      prev_track = (CHUNKER_TRACK*)vector_get( &(h->tracks), i );
      iter_track = (CHUNKER_TRACK*)vector_get( &(h->tracks), i + 1 );
      if(
         (NULL != iter_track && NULL != prev_track &&
            (prev_track->start + prev_track->length) < iter_track->start) ||
         (NULL == iter_track && NULL != prev_track &&
            (prev_track->start + prev_track->length) < h->raw_length)
      ) {
         finished = FALSE;
      }
   }
   vector_lock( &(h->tracks), FALSE );

   /* If the file is complete and the cache is enabled, then do that. */
   if( TRUE == finished ) {
      scaffold_print_info(
         "Chunker: Saving cached copy of finished file: %s\n",
         bdata( h->filename )
      );
      chunker_unchunk_save_cache( h );
   }

cleanup:

   return finished;
}
