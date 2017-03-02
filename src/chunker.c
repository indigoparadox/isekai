
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
   size_t src_length
) {
   scaffold_check_null( src_buffer );
   assert( NULL != h );

   ref_init( &(h->refcount), chunker_cleanup );

   if( NULL != h->encoder ) {
      /* TODO: What if the client has a chunker already? */
   } else {
      h->encoder = heatshrink_encoder_alloc(
         CHUNKER_WINDOW_SIZE,
         CHUNKER_LOOKAHEAD_SIZE
      );
   }

   h->finished = FALSE;
   h->raw_position = 0;
   h->raw_length = src_length;
   h->raw_ptr = (uint8_t*)calloc( src_length, sizeof( uint8_t ) );
   memcpy( h->raw_ptr, src_buffer, src_length );
   h->channel = bstrcpy( channel );
   h->type = type;

cleanup:
   return;
}

void chunker_chunk_pass( CHUNKER* h, bstring xmit_buffer ) {
   size_t consumed,
      exhumed,
      xmit_buffer_pos = 0;
   uint8_t binary_compression_buffer[CHUNKER_XMIT_BINARY_SIZE] = { 0 };
   HSE_poll_res poll_res;
   HSE_sink_res sink_res;

   /* Sink enough data to fill an outgoing buffer and wait for it to process. */
   while( CHUNKER_XMIT_BINARY_SIZE > xmit_buffer_pos ) {

      assert( TRUE != h->finished );

      if( h->raw_position < h->raw_length ) {
         assert( TRUE != h->finished );
         sink_res = heatshrink_encoder_sink(
            h->encoder,
            (uint8_t*)&(h->raw_ptr[h->raw_position]),
            (size_t)(h->raw_length - h->raw_position),
            &consumed
         );
         assert( HSER_SINK_OK == sink_res );
         h->raw_position += consumed;
      } else if( TRUE != h->finished ) {
         heatshrink_encoder_finish( h->encoder );
         h->finished = TRUE;
         break;
      }

      do {
         poll_res = heatshrink_encoder_poll(
            h->encoder,
            binary_compression_buffer,
            CHUNKER_XMIT_BINARY_SIZE,
            &exhumed
         );
         xmit_buffer_pos += exhumed;

      } while( HSER_POLL_MORE == poll_res );
      assert( HSER_POLL_EMPTY == poll_res );
   }

   b64_encode(
      &binary_compression_buffer, CHUNKER_XMIT_BINARY_SIZE, xmit_buffer, 76
   );

/* cleanup: */
   return;
}

/*
chunker_chunk_end


chunker_unchunk_start
chunker_unchunk_pass
chunker_unchunk_end
*/
