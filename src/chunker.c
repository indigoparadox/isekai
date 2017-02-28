#include "chunker.h"

#if 0
/* The chunker should NOT free or modify any buffers passed to it. */
void chunker_chunk_start( CHUNKER* h, bstring src_buffer ) {
   scaffold_check_null( buffer );

   if( NULL != c->chunker.encoder ) {
      /* TODO: What if the client has a chunker already? */
   }
   h = heatshrink_encoder_alloc(
      PARSER_HS_WINDOW_SIZE,
      PARSER_HS_LOOKAHEAD_SIZE
   );
   scaffold_check_null( h );
   c->chunker.encoder = h;
   c->chunker.pos = 0;
   c->chunker.foreign_buffer = l->gamedata.tmap.serialize_buffer;
   c->chunker.filename = l->gamedata.tmap.serialize_filename;

   // FIXME: Encode before transmission.
	const char* foreign_buffer_c = bdata( c->chunker.foreign_buffer );

   /* Sink the map data into the encoder. */
   /* TODO: Finish this accross multiple requests. */
   while( blength( c->chunker.foreign_buffer ) > c->chunker.pos ) {
      hse_res = heatshrink_encoder_sink(
         h,
         (uint8_t*)&(foreign_buffer_c[c->chunker.pos]),
         (size_t)blength( c->chunker.foreign_buffer ) - c->chunker.pos,
         &consumed
      );
      assert( HSER_SINK_OK == hse_res );
      c->chunker.pos += consumed;
   }
   if( c->chunker.pos == blength( c->chunker.foreign_buffer ) ) {
      heatshrink_encoder_finish( h );
   }
   assert( c->chunker.pos == blength( c->chunker.foreign_buffer ) );

   c->chunker.pos = 0;

   server_xmit_chunk( s );

}

void chunker_chunk_pass( CHUNKER* h ) {

}


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
