
#include "chunker.h"

#include <stdlib.h>

#include "scaffold.h"
#include "b64/b64.h"

static void chunker_cleanup( const struct _REF* ref ) {
   CHUNKER* h = (CHUNKER*)scaffold_container_of( ref, CHUNKER, refcount );
   if( NULL != h->raw_ptr ) {
      free( h->raw_ptr );
   }
   free( h );
}

void chunker_free( CHUNKER* h ) {
   ref_dec( &(h->refcount) );
}

/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_chunk_start(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type,  void* src_buffer,
   size_t src_length, size_t tx_chunk_length
) {
   scaffold_check_null( src_buffer );
   assert( NULL != h );

   ref_init( &(h->refcount), chunker_cleanup );

#ifdef USE_HEATSHRINK
   if( NULL != h->encoder ) {
      /* TODO: What if the client has a chunker already? */
   } else {
      h->encoder = heatshrink_encoder_alloc(
         CHUNKER_WINDOW_SIZE,
         CHUNKER_LOOKAHEAD_SIZE
      );
   }
#endif /* USE_HEATSHRINK */

   h->finished = FALSE;
   h->raw_position = 0;
   h->raw_length = src_length;
   h->raw_ptr = (uint8_t*)calloc( src_length, sizeof( uint8_t ) );
   memcpy( h->raw_ptr, src_buffer, src_length );
   h->channel = bstrcpy( channel );
   h->type = type;
   // FIXME: Remove this?
   h->tx_chunk_length = tx_chunk_length;

cleanup:
   return;
}

void chunker_chunk_pass( CHUNKER* h, bstring tx_buffer ) {
#ifdef USE_HEATSHRINK
   size_t consumed,
      exhumed,
      tx_buffer_start = h->raw_position,
      mid_buffer_pos = 0,
      raw_chunk_length,
      mid_buffer_length = h->tx_chunk_length / 4, // 60 / 4 = 15
      tx_buffer_length = h->tx_chunk_length / 4;
      //tx_buffer_pos = 0;
   uint8_t* mid_buffer;
   static HSE_poll_res poll_res = HSER_POLL_EMPTY;
   HSE_sink_res sink_res;

   assert( NULL == h->decoder );
   assert( NULL != h->encoder );

   /* 4x character count for base64 encoding. */
   //mid_buffer_length = blength( tx_buffer ) / 4;
   //mid_buffer_length = h->tx_chunk_length / 4; // 60 / 4 = 15
   //tx_buffer_length = h->tx_chunk_length / 4;
   mid_buffer = (uint8_t*)calloc( mid_buffer_length, sizeof( uint8_t ) );
   scaffold_check_null( mid_buffer );

   /* Sink enough data to fill an outgoing buffer and wait for it to process. */
   while( mid_buffer_length > mid_buffer_pos ) {

      if( HSER_POLL_MORE != poll_res && TRUE != h->finished && h->raw_position < h->raw_length ) {
         assert( TRUE != h->finished );
         //raw_chunk_length = tx_buffer_length - (h->raw_position - tx_buffer_start);
         sink_res = heatshrink_encoder_sink(
            h->encoder,
            &(h->raw_ptr[h->raw_position]), // tx_buffer_start + tx_buffer_pos
            //raw_chunk_length,
            h->raw_length - h->raw_position,
            &consumed
         );
         assert( HSER_SINK_OK == sink_res );
         h->raw_position += consumed;
         //tx_buffer_pos -= consumed;
      }

      if( TRUE != h->finished && h->raw_position >= h->raw_length ) {
         heatshrink_encoder_finish( h->encoder );
         h->finished = TRUE;
         break;
      }

      do {
         poll_res = heatshrink_encoder_poll(
            h->encoder,
            &(mid_buffer[mid_buffer_pos]),
            mid_buffer_length - mid_buffer_pos,
            &exhumed
         );
         mid_buffer_pos += exhumed;

      } while( HSER_POLL_MORE == poll_res && mid_buffer_pos < mid_buffer_length );
      assert( HSER_POLL_EMPTY == poll_res || HSER_POLL_MORE == poll_res );
   }

   b64_encode( mid_buffer, mid_buffer_length, tx_buffer, 76 );
#else
   if( h->raw_position + xmit_chunk_length > h->raw_length ) {
      xmit_chunk_length = h->raw_length % xmit_chunk_length;
   }

   b64_encode( &(h->raw_ptr[h->raw_position]), xmit_chunk_length, xmit_buffer, 76 );
   h->raw_position += xmit_chunk_length;

   if( h->raw_position >= h->raw_length ) {
      h->finished = TRUE;
   }
#endif /* USE_HEATSHRINK */

cleanup:
#ifdef USE_HEATSHRINK
   if( NULL != mid_buffer ) {
      free( mid_buffer );
   }
#endif /* USE_HEATSHRINK */
   return;
}

/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_unchunk_start(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type, size_t src_length
) {
   assert( NULL != h );
   assert( 0 != src_length );

   ref_init( &(h->refcount), chunker_cleanup );

#ifdef USE_HEATSHRINK
   if( NULL != h->decoder ) {
      /* TODO: What if the client has a chunker already? */
   } else {
      h->decoder = heatshrink_decoder_alloc(
         src_length, /* TODO */
         CHUNKER_WINDOW_SIZE,
         CHUNKER_LOOKAHEAD_SIZE
      );
   }
#endif /* USE_HEATSHRINK */

   h->finished = FALSE;
   h->raw_position = 0;
   h->raw_length = src_length;
   h->raw_ptr = (uint8_t*)calloc( src_length, sizeof( uint8_t ) );
   h->channel = bstrcpy( channel );
   h->type = type;

cleanup:
   return;
}

void chunker_unchunk_pass( CHUNKER* h, bstring rx_buffer, size_t src_chunk_start, size_t src_chunk_len ) {
#ifdef USE_HEATSHRINK
   size_t consumed,
      exhumed;
   size_t mid_buffer_length = blength( rx_buffer ),
      mid_buffer_pos = 0;
   uint8_t* mid_buffer = NULL;
   HSD_poll_res poll_res;
   HSD_sink_res sink_res;
#endif /* USE_HEATSHRINK */
   size_t working_raw_start;

#ifdef USE_HEATSHRINK
   assert( NULL != h->decoder );
   assert( NULL == h->encoder );

   mid_buffer = (uint8_t*)calloc( mid_buffer_length, sizeof( uint8_t ) );
   scaffold_check_null( mid_buffer );
#endif /* USE_HEATSHRINK */

   h->raw_position = src_chunk_start;

#ifdef USE_HEATSHRINK
   b64_decode( rx_buffer, mid_buffer, &mid_buffer_length );

   /* Sink enough data to fill an outgoing buffer and wait for it to process. */
   while( (src_chunk_start + src_chunk_len) > h->raw_position ) {
      if( mid_buffer_pos < mid_buffer_length ) {
         sink_res = heatshrink_decoder_sink(
            h->decoder,
            mid_buffer,
            mid_buffer_length,
            &consumed
         );
         assert( HSDR_SINK_OK == sink_res );
         mid_buffer_pos += consumed;
      } else if( TRUE != h->finished ) {
         break;
      }

      do {
         poll_res = heatshrink_decoder_poll(
            h->decoder,
            (uint8_t*)&(h->raw_ptr[h->raw_position]),
            src_chunk_len,
            &exhumed
         );
         h->raw_position += exhumed;

      } while( HSDR_POLL_MORE == poll_res && 0 != exhumed );
      assert( HSDR_POLL_EMPTY == poll_res );
   }
#else
   b64_decode( rx_buffer, &(h->raw_ptr[h->raw_position]), &rx_chunk_len );
#endif /* USE_HEATSHRINK */

cleanup:
#ifdef USE_HEATSHRINK
   if( NULL != mid_buffer ) {
      free( mid_buffer );
   }
#endif /* USE_HEATSHRINK */
   return;
}
