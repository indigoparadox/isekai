
#include "chunker.h"

#include "scaffold.h"

static void chunker_cleanup( const struct _REF* ref ) {

}

/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_chunk_start( CHUNKER* h, void* src_buffer, size_t src_length ) {
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
   memcpy( h->raw_ptr, h->raw_length, src_length );

cleanup:
   return;
}

void chunker_chunk_pass( CHUNKER* h, bstring xmit_buffer ) {
   size_t consumed,
      exhumed,
      xmit_buffer_pos = 0;
   uint8_t binary_compression_buffer[CHUNKER_XMIT_BINARY_SIZE] = { 0 };
   int i;
   HSE_poll_res poll_res;
   HSE_sink_res sink_res;

   // FIXME: Encode before transmission.
//	const char* foreign_buffer_c = bdata( c->chunker.foreign_buffer );

   /* Sink the map data into the encoder. */
   /* TODO: Finish this accross multiple requests. */
   //while( h->raw_length < h->raw_position ) {
   while( CHUNKER_XMIT_BINARY_SIZE > xmit_buffer_pos ) {
      if( h->raw_position < h->raw_length ) {
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
      }

      poll_res = heatshrink_encoder_poll(
         h->encoder,
         &binary_compression_buffer,
         CHUNKER_XMIT_BINARY_SIZE,
         &exhumed
      );
      xmit_buffer_pos += exhumed;

      if( HSER_POLL_MORE != poll_res ) {
         break;
      }
   }

   b64_encode(
      &binary_compression_buffer, CHUNKER_XMIT_BINARY_SIZE, xmit_buffer, 76
   );
cleanup:
   return;
}

#if 0
void server_xmit_chunk( SERVER* s, CLIENT* c ) {
   heatshrink_encoder* h = (heatshrink_encoder*)c->chunker.encoder;
   size_t consumed, exhumed;
   uint8_t* outbuffer = NULL;
   bstring outbuffer64 = NULL;

   /* Allocate temporary buffers. */
   outbuffer = (uint8_t*)calloc( PARSER_FILE_XMIT_BUFFER, sizeof( uint8_t ) );
   scaffold_check_null( outbuffer );
   outbuffer64 = bfromcstralloc( (4 * PARSER_FILE_XMIT_BUFFER), "" );
   scaffold_check_null( outbuffer64 );

   c->chunker.hsp_last_res = heatshrink_encoder_poll(
      h, outbuffer, PARSER_FILE_XMIT_BUFFER, &exhumed
   );
   //assert( HSER_SINK_OK == hsp_res );

   b64_encode( outbuffer, exhumed, outbuffer64, 100 );

   server_client_printf(
      s, c, ":%b GDB %b TILEMAP %b %d %d : %b",
      s->self.remote, c->nick, c->chunker.filename, c->chunker.pos,
      blength( c->chunker.foreign_buffer ), outbuffer64
   );

   c->chunker.pos += exhumed;

cleanup:
   if( NULL != outbuffer ) {
      free( outbuffer );
   }
   bdestroy( outbuffer64 );
}

static void* server_prn_chunk( VECTOR* v, size_t idx, void* iter, void* arg ) {
   SERVER* s = (SERVER*)arg;
   CLIENT* c = (CLIENT*)iter;
   heatshrink_encoder* h = (heatshrink_encoder*)c->chunker.encoder;
   HSE_sink_res hse_res;

   if( NULL == c->chunker.encoder ) {
      goto cleanup;
   }


/*
	while (sunk < dataLen) {
	bool ok = heatshrink_encoder_sink(&hse, &data[sunk], dataLen - sunk, &count) >= 0;
	assert(ok);
	sunk += count;
	if (sunk == dataLen) {
	heatshrink_encoder_finish(&hse);
	}
// FIXME
   //do {
   hsp_res = heatshrink_encoder_poll(
      h, outbuffer, PARSER_FILE_XMIT_BUFFER, &exhumed
   ); */
   //assert( HSER_SINK_OK == hsp_res );
   //c->chunker.pos += exhumed;
	//} while( HSER_POLL_MORE ==hsp_res );

   if( HSER_POLL_MORE == c->chunker.hsp_last_res ) {
      server_xmit_chunk( s, c );
   } else {
      assert( HSER_POLL_EMPTY == c->chunker.hsp_last_res );
      /*if( sunk == dataLen ) {
         heatshrink_encoder_finish(&hse);
      }*/
   }

   //chunker_unchunk( h, filename, data, progress );
   scaffold_print_debug( "%d out of %d\n", c->chunker.pos, blength( c->chunker.foreign_buffer ) );

cleanup:
   return NULL;
}
#endif // 0

/*
chunker_chunk_end


chunker_unchunk_start
chunker_unchunk_pass
chunker_unchunk_end
*/
