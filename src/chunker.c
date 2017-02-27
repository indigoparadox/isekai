
#include "chunker.h"

#define MINIZ_HEADER_FILE_ONLY

#include "miniz.c"
#include "b64/b64.h"
#include "scaffold.h"

void chunker_init( CHUNKER* h, ssize_t chunk_size_src, ssize_t line_len_out ) {
   h->status = CHUNKER_STATUS_NONE;
   h->alloc = 0;
   h->src_buffer = NULL;
   h->src_len = 0;
   h->dest_buffer = bfromcstralloc( CHUNKER_DEFAULT_ALLOC_B64, "" );
   h->filename = bfromcstralloc( 15, "" );
   h->progress = 0;
   h->chunk_size_src = 0 < chunk_size_src ? chunk_size_src : CHUNKER_DEFAULT_INGEST_CHUNK;
   h->chunk_size_line = 0 < line_len_out ? line_len_out : CHUNKER_DEFAULT_LINE_LENGTH_OUT;
   h->callback = NULL;
   h->mailqueue = NULL;
}

void chunker_cleanup( CHUNKER* h ) {
   bdestroy( h->dest_buffer );
   bdestroy( h->filename );
}


void chunker_set_cb( CHUNKER* h, CHUNKER_CALLBACK cb, MAILBOX* m, void* arg ) {
   h->callback = cb;
   h->mailqueue = m;
   h->cb_arg = arg;
}

static void chunker_mailbox_cb( MAILBOX* m, MAILBOX_ENVELOPE* e ) {
   CHUNKER* h = (CHUNKER*)e->cb_arg;

   chunker_chew( h );

   if( NULL != h->callback ) {
      h->callback( h, e->socket_src );
   }

   if( CHUNKER_STATUS_DELETE == h->status ) {
      chunker_cleanup( h );
      free( h );
   }

   if(
      CHUNKER_STATUS_FINISHED == h->status || CHUNKER_STATUS_DELETE == h->status
   ) {
      e->callback = NULL;
      e->special = MAILBOX_ENVELOPE_SPECIAL_DELETE;
   }
}

void chunker_chunk( CHUNKER* h, ssize_t socket, bstring filename, BYTE* data, size_t len ) {
   /* Ensure sanity. */
   scaffold_check_null( h );
   scaffold_check_null( data );
   scaffold_check_null( h->filename );
   scaffold_check_null( h->dest_buffer );

   h->alloc = len;
   h->src_len = len;
   h->src_buffer = data;
   btrunc( h->dest_buffer, 0 );
   bassign( h->filename, filename );
   h->status = CHUNKER_STATUS_WORKING;

   if( NULL != h->mailqueue ) {
      mailbox_call( h->mailqueue, socket, chunker_mailbox_cb, h );
   }

cleanup:
   return;
}

void chunker_chew( CHUNKER* h ) {
   mz_zip_archive buffer_archive = { 0 };
   mz_bool zip_result = 0;
   void* zip_buffer = NULL;
   size_t zip_buffer_size = 0;
   size_t increment = 0;
   int bstr_result;

   scaffold_error = 0;

   scaffold_check_null( h );
   scaffold_check_null( h->dest_buffer );
   scaffold_check_null( h->src_buffer );
   scaffold_check_null( h->filename );

   memset( &buffer_archive, '\0', sizeof( mz_zip_archive ) );

   /* Make sure we don't chunk beyond the end of the source. */
   if( h->progress + h->chunk_size_src > h->src_len ) {
      h->status = CHUNKER_STATUS_FINISHED;
      increment = h->src_len - h->progress;
   } else {
      increment = h->chunk_size_src;
   }

   assert( h->progress + increment > h->progress );
   assert( NULL != h->dest_buffer );
   assert( NULL != h->src_buffer );

   /* Zip the chunk. */
   zip_result = mz_zip_writer_init_heap(
      &buffer_archive, 0, CHUNKER_DEFAULT_ALLOC_ZIP
   );
   scaffold_check_zero( zip_result );
   zip_result = mz_zip_writer_add_mem(
      &buffer_archive,
      bdata( h->filename ),
      &(h->src_buffer[h->progress]),
      increment,
      MZ_BEST_SPEED
   );
   scaffold_check_zero( zip_result );
   h->progress += increment;
   zip_result = mz_zip_writer_finalize_heap_archive(
      &buffer_archive, &zip_buffer, &zip_buffer_size
   );
   scaffold_check_zero( zip_result );
   zip_result = mz_zip_writer_end( &buffer_archive );
   scaffold_check_zero( zip_result );
   b64_encode(
      zip_buffer, zip_buffer_size, h->dest_buffer,
      h->chunk_size_line
   );

#ifdef DEBUG
   bstr_result = bstrchr( h->dest_buffer, '\n' );
   assert( BSTR_ERR == bstr_result );
#endif /* DEBUG */

cleanup:
   if( SCAFFOLD_ERROR_NONE != scaffold_error ) {
      scaffold_print_error( "Chunker encountered an error.\n" );
      h->status = CHUNKER_STATUS_ERROR;
   }
   if( NULL != zip_buffer ) {
      free( zip_buffer );
   }
   return;
}

void chunker_unchunk( CHUNKER* h, bstring buffer ) {

}

void chunker_unchew( CHUNKER* h, bstring buffer ) {

}
