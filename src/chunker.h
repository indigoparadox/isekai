
#ifndef CHUNKER_H
#define CHUNKER_H

#include "bstrlib/bstrlib.h"
#include "mailbox.h"

#include <stdint.h>

#define CHUNKER_DEFAULT_ALLOC_B64 1024
#define CHUNKER_DEFAULT_ALLOC_ZIP 1024
#define CHUNKER_DEFAULT_INGEST_CHUNK 256
#define CHUNKER_DEFAULT_LINE_LENGTH_OUT 1024
//#define CHUNKER_INITIAL_LINESZ 45

typedef struct _CHUNKER CHUNKER;

typedef void (*CHUNKER_CALLBACK)( CHUNKER* h );

typedef enum _CHUNKER_STATUS {
   CHUNKER_STATUS_NONE,
   CHUNKER_STATUS_WORKING,
   CHUNKER_STATUS_FINISHED,
   CHUNKER_STATUS_ERROR,
   CHUNKER_STATUS_DELETE,
} CHUNKER_STATUS;

typedef struct _CHUNKER {
   size_t alloc;
   size_t progress;
   size_t chunk_size_src;
   size_t chunk_size_line;
   uint8_t* src_buffer;
   size_t src_len;
   bstring dest_buffer;
   bstring filename;
   CHUNKER_CALLBACK callback;
   void* cb_arg;
   MAILBOX* mailqueue;
   CHUNKER_STATUS status;
} CHUNKER;

#define chunker_new( h, chunk_size_src, line_len_out ) \
    h = calloc( 1, sizeof( CHUNKER ) ); \
    scaffold_check_null( h ); \
    chunker_init( h, chunk_size_src, line_len_out );

void chunker_init( CHUNKER* h, ssize_t chunk_size_src, ssize_t line_len_out );
void chunker_set_cb( CHUNKER* h, CHUNKER_CALLBACK cb, MAILBOX* m, void* arg );
void chunker_cleanup( CHUNKER* h );
void chunker_chunk( CHUNKER* h, bstring filename, uint8_t* data, size_t len );
void chunker_chew( CHUNKER* h );

#endif /* CHUNKER_H */
