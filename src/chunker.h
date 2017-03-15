#ifndef CHUNKER_H
#define CHUNKER_H

#include "bstrlib/bstrlib.h"
#include "ref.h"
#include "vector.h"

#ifdef USE_B64
#include "b64.h"
#endif /* USE_B64 */

#include "hs/hsdecode.h"
#include "hs/hsencode.h"

#define CHUNKER_WINDOW_SIZE 14
#define CHUNKER_LOOKAHEAD_SIZE 8
#define CHUNKER_FILENAME_ALLOC 80
#define CHUNKER_DEFAULT_CHUNK_SIZE 64

#if defined( USE_HEATSHRINK ) && !defined( USE_B64 )
#error Heatshrink requires Base64 to be enabled!
#endif /* USE_HEATSHRINK && !USE_B64 */

#if defined( USE_SHM ) && defined( USE_B64 )
#error SHM cannot be used with Base64!
#endif /* USE_SHM && USE_B64 */

#if defined( USE_SHM ) && defined( USE_HEATSHRINK )
#error SHM cannot be used with Heatshrink!
#endif /* USE_SHM && USE_HEATSHRINK */

#if defined( USE_SHM ) && defined( USE_FILE_CACHE )
#error SHM cannot be used with file cache!
#endif /* USE_SHM && USE_FILE_CACHE */

typedef enum _CHUNKER_DATA_TYPE {
   CHUNKER_DATA_TYPE_TILEMAP,
   CHUNKER_DATA_TYPE_TILESET_IMG,
   CHUNKER_DATA_TYPE_MOBSPRITES,
   CHUNKER_DATA_TYPE_MOBDEF
} CHUNKER_DATA_TYPE;

struct CHUNKER_PROGRESS {
   SCAFFOLD_SIZE current;
   SCAFFOLD_SIZE total;
   SCAFFOLD_SIZE chunk_size;
   bstring data;
   bstring filename;
   CHUNKER_DATA_TYPE type;
};

typedef struct _CHUNKER_TRACK {
   SCAFFOLD_SIZE start;
   SCAFFOLD_SIZE length;
} CHUNKER_TRACK;

struct CHUNKER {
   struct REF refcount;
#if HEATSHRINK_DYNAMIC_ALLOC
   heatshrink_encoder* encoder;
   heatshrink_decoder* decoder;
#else
   heatshrink_encoder encoder;
   heatshrink_decoder decoder;
#endif /* HEATSHRINK_DYNAMIC_ALLOC */
   SCAFFOLD_SIZE raw_position;
   SCAFFOLD_SIZE raw_length;
   BYTE* raw_ptr;
   SCAFFOLD_SIZE tx_chunk_length;
   BOOL force_finish;
   CHUNKER_DATA_TYPE type;
   struct VECTOR tracks;
   bstring filecache_path;
   bstring filename;
   bstring serverpath;
   SCAFFOLD_SIZE last_percent;
   void* placeholder; /*!< For use by a parser function. */
};

#if HEATSHRINK_DYNAMIC_ALLOC
#define chunker_get_encoder( h ) (h->encoder)
#define chunker_get_decoder( h ) (h->decoder)
#else
#define chunker_get_encoder( h ) (&(h->encoder))
#define chunker_get_decoder( h ) (&(h->decoder))
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

void chunker_free( struct CHUNKER* h );
void chunker_chunk_start(
   struct CHUNKER* h, CHUNKER_DATA_TYPE type,  void* src_buffer,
   SCAFFOLD_SIZE src_length, SCAFFOLD_SIZE tx_chunk_length
);
void chunker_chunk_start_file(
   struct CHUNKER* h, CHUNKER_DATA_TYPE type, bstring serverpath,
   bstring filepath, SCAFFOLD_SIZE tx_chunk_length
);
SCAFFOLD_SIZE chunker_chunk_pass( struct CHUNKER* h, bstring tx_buffer )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
BOOL chunker_chunk_finished( struct CHUNKER* h );
void chunker_unchunk_start(
   struct CHUNKER* h, CHUNKER_DATA_TYPE type,
   const bstring filename, const bstring filecache_path
);
void chunker_unchunk_pass( struct CHUNKER* h, bstring rx_buffer, SCAFFOLD_SIZE src_chunk_start, SCAFFOLD_SIZE src_len, SCAFFOLD_SIZE src_chunk_len );
void chunker_unchunk_save_cache( struct CHUNKER* h );
void chunker_unchunk_check_cache( struct CHUNKER* h );
BOOL chunker_unchunk_finished( struct CHUNKER* h );
int8_t chunker_unchunk_percent_progress( struct CHUNKER* h, BOOL force );
BOOL chunker_unchunk_cached( struct CHUNKER* h );

#ifdef CHUNKER_C
SCAFFOLD_MODULE( "chunker.c" );
#endif /* CHUNKER_C */

#endif /* CHUNKER_H */
