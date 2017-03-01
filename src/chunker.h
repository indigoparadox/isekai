#ifndef CHUNKER_H
#define CHUNKER_H

#include "bstrlib/bstrlib.h"
#include "heatshrink/heatshrink_decoder.h"
#include "heatshrink/heatshrink_encoder.h"
#include "ref.h"

/* b64 = ~4 * uint8_t size */

#define CHUNKER_XMIT_BUFFER_SIZE 60
#define CHUNKER_XMIT_BINARY_SIZE (CHUNKER_XMIT_BUFFER_SIZE / 4)
#define CHUNKER_WINDOW_SIZE 14
#define CHUNKER_LOOKAHEAD_SIZE 8

typedef struct _CHUNKER {
   REF refcount;
   heatshrink_encoder* encoder;
   heatshrink_decoder* decoder;
   size_t raw_position;
   size_t raw_length;
   void* raw_ptr;
   BOOL finished;
} CHUNKER;

void chunker_chunk_start( CHUNKER* h, void* src_buffer, size_t src_length );
void chunker_chunk_pass( CHUNKER* h, bstring xmit_buffer );

#endif /* CHUNKER_H */
