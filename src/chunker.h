#ifndef CHUNKER_H
#define CHUNKER_H

#include "bstrlib/bstrlib.h"
#include "heatshrink/heatshrink_decoder.h"
#include "heatshrink/heatshrink_encoder.h"
#include "ref.h"
#include "vector.h"

/* b64 = ~4 * uint8_t size */

//#define CHUNKER_XMIT_BUFFER_SIZE 60
//#define CHUNKER_XMIT_BINARY_SIZE (CHUNKER_XMIT_BUFFER_SIZE * 4)
#define CHUNKER_WINDOW_SIZE 14
#define CHUNKER_LOOKAHEAD_SIZE 8

typedef enum _CHUNKER_DATA_TYPE {
   CHUNKER_DATA_TYPE_TILEMAP,
} CHUNKER_DATA_TYPE;

typedef struct _CHUNKER_TRACK {
   size_t start;
   size_t length;
} CHUNKER_TRACK;

typedef struct _CHUNKER {
   REF refcount;
   heatshrink_encoder* encoder;
   heatshrink_decoder* decoder;
   size_t raw_position;
   size_t raw_length;
   //size_t raw_chunk_length;
   uint8_t* raw_ptr;
   size_t tx_chunk_length;
   BOOL finished;
   bstring channel;
   CHUNKER_DATA_TYPE type;
   VECTOR tracks;
} CHUNKER;

void chunker_free( CHUNKER* h );
void chunker_chunk_start(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type,  void* src_buffer,
   size_t src_length, size_t tx_chunk_length
);
void chunker_chunk_pass( CHUNKER* h, bstring tx_buffer );
BOOL chunker_chunk_finished( CHUNKER* h );
void chunker_unchunk_start(
   CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type, size_t src_length
);
void chunker_unchunk_pass( CHUNKER* h, bstring rx_buffer, size_t src_chunk_start, size_t src_chunk_len );
BOOL chunker_unchunk_finished( CHUNKER* h );

#endif /* CHUNKER_H */
