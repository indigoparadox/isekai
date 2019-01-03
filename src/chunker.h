#ifndef CHUNKER_H
#define CHUNKER_H

#include "libvcol.h"
#include "ref.h"
#include "datafile.h"

#include "hs/hs_com.h"
#include "hs/hs_conf.h"

#ifdef USE_B64
#include "b64.h"
#endif /* USE_B64 */

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

struct CHUNKER_PROGRESS {
   SCAFFOLD_SIZE current;
   SCAFFOLD_SIZE total;
   SCAFFOLD_SIZE chunk_size;
   bstring data;
   bstring filename;
   DATAFILE_TYPE type;
   struct CHUNKER_PARMS* parms;
};

typedef struct _CHUNKER_TRACK {
   SCAFFOLD_SIZE start;
   SCAFFOLD_SIZE length;
} CHUNKER_TRACK;

/* If we're using dynamic allocation, then we're using pointers so we don't
 * need the full struct definition, but if we're using static allocation, the
 * struct is embedded, so we do. But the C99 stuff is disabled, so we won't
 * get all of the useless warnings in that case.
 */
#if HEATSHRINK_DYNAMIC_ALLOC
struct heatshrink_decoder;
struct heatshrink_encoder;
#else
#include "hs/hsdecode.h"
#include "hs/hsencode.h"
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

struct CHUNKER_PARMS {
   int version;
   VBOOL compression;
};

struct CHUNKER {
   struct REF refcount;
//#if HEATSHRINK_DYNAMIC_ALLOC
   struct heatshrink_encoder* encoder;
   struct heatshrink_decoder* decoder;
//#else
//   struct heatshrink_encoder encoder;
//   struct heatshrink_decoder decoder;
//#endif /* HEATSHRINK_DYNAMIC_ALLOC */
   size_t raw_position;
   size_t raw_length;
   BYTE* raw_ptr;
   size_t tx_chunk_length;
   VBOOL force_finish;
   DATAFILE_TYPE type;
   struct VECTOR* tracks;
   bstring filecache_path;
   bstring filename;
   bstring serverpath;
   size_t last_percent;
   struct CHUNKER_PARMS* parms;
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
   struct CHUNKER* h, DATAFILE_TYPE type,  void* src_buffer,
   SCAFFOLD_SIZE src_length, SCAFFOLD_SIZE tx_chunk_length
);
VBOOL chunker_chunk_start_file(
   struct CHUNKER* h, DATAFILE_TYPE type, bstring serverpath,
   bstring filepath, SCAFFOLD_SIZE tx_chunk_length
);
SCAFFOLD_SIZE chunker_chunk_pass( struct CHUNKER* h, bstring tx_buffer )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
VBOOL chunker_chunk_finished( struct CHUNKER* h );
void chunker_unchunk_start(
   struct CHUNKER* h, DATAFILE_TYPE type,
   const bstring filename, const bstring filecache_path
);
void chunker_unchunk_pass(
   struct CHUNKER* h, bstring rx_buffer, SCAFFOLD_SIZE src_chunk_start,
   SCAFFOLD_SIZE src_len, SCAFFOLD_SIZE src_chunk_len
);
VBOOL chunker_unchunk_save_cache( struct CHUNKER* h );
void chunker_unchunk_check_cache( struct CHUNKER* h );
VBOOL chunker_unchunk_finished( struct CHUNKER* h );
int8_t chunker_unchunk_percent_progress( struct CHUNKER* h, VBOOL force );
VBOOL chunker_unchunk_cached( struct CHUNKER* h );
struct CHUNKER_PARMS* chunker_decode_block( bstring block );
bstring chunker_encode_block( struct CHUNKER_PARMS* parms );

#ifdef CHUNKER_C
struct tagbstring chunker_type_names[] = {
   bsStatic( "no type" ),
   bsStatic( "tilemap definiton" ),
   bsStatic( "tileset definition" ),
   bsStatic( "tileset sprites" ),
   bsStatic( "mobile sprites" ),
   bsStatic( "mobile definition" ),
   bsStatic( "item catalog" ),
   bsStatic( "item catalog sprites" )
};
#else
extern struct tagbstring chunker_type_names[];
#endif /* CHUNKER_C */

#endif /* CHUNKER_H */
