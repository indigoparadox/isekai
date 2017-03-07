#ifndef CHUNKER_H
#define CHUNKER_H

#include "bstrlib/bstrlib.h"
#include "heatshrink/heatshrink_decoder.h"
#include "heatshrink/heatshrink_encoder.h"
#include "ref.h"
#include "vector.h"

#define CHUNKER_WINDOW_SIZE 14
#define CHUNKER_LOOKAHEAD_SIZE 8

typedef enum _CHUNKER_DATA_TYPE {
   CHUNKER_DATA_TYPE_TILEMAP,
   CHUNKER_DATA_TYPE_TILESET_IMG,
   CHUNKER_DATA_TYPE_MOBSPRITES
} CHUNKER_DATA_TYPE;

struct CHUNKER_PROGRESS {
   size_t current;
   size_t total;
   size_t chunk_size;
   bstring data;
   bstring filename;
   CHUNKER_DATA_TYPE type;
};

typedef struct _CHUNKER_TRACK {
   size_t start;
   size_t length;
} CHUNKER_TRACK;

struct CHUNKER {
   struct REF refcount;
   heatshrink_encoder* encoder;
   heatshrink_decoder* decoder;
   size_t raw_position;
   size_t raw_length;
   BYTE* raw_ptr;
   size_t tx_chunk_length;
   BOOL force_finish;
   bstring channel;
   CHUNKER_DATA_TYPE type;
   struct VECTOR tracks;
   bstring filecache_path;
   bstring filename;
   bstring serverpath;
};

void chunker_free( struct CHUNKER* h );
void chunker_chunk_start(
   struct CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type,  void* src_buffer,
   size_t src_length, size_t tx_chunk_length
);
void chunker_chunk_start_file(
   struct CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type, bstring serverpath,
   bstring filepath, size_t tx_chunk_length
);
void chunker_chunk_pass( struct CHUNKER* h, bstring tx_buffer );
BOOL chunker_chunk_finished( struct CHUNKER* h );
void chunker_unchunk_start(
   struct CHUNKER* h, bstring channel, CHUNKER_DATA_TYPE type, size_t src_length,
   const bstring filename, const bstring filecache_path
);
void chunker_unchunk_pass( struct CHUNKER* h, bstring rx_buffer, size_t src_chunk_start, size_t src_chunk_len );
void chunker_unchunk_save_cache( struct CHUNKER* h );
void chunker_unchunk_check_cache( struct CHUNKER* h, const bstring filecache_path );
BOOL chunker_unchunk_finished( struct CHUNKER* h );

#endif /* CHUNKER_H */
