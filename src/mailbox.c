
#include "mailbox.h"

#include <stdlib.h>

size_t mailbox_listen( MAILBOX* mailbox ) {
    /* vector_init( &(mailbox->envelopes) ); */
    if( 0 >= mailbox->last_socket ) {
       mailbox->last_socket++;
    }
    return mailbox->last_socket++;
}

void* mailbox_envelope_cmp( VECTOR* v, void* iter, void* arg ) {
   MAILBOX_ENVELOPE* envelope = (MAILBOX_ENVELOPE*)iter;
   bstring str_search = (bstring)arg;

   if( 0 == bstrcmp( str_search, envelope->contents ) ) {
      return envelope;
   }

   return NULL;
}

size_t mailbox_accept( MAILBOX* mailbox, size_t socket_dest ) {
   MAILBOX_ENVELOPE* top_envelope = NULL;
   size_t socket_out = -1;

   if( 0 >= socket_dest ) {
      goto cleanup;
   }

   top_envelope = vector_get( &(mailbox->envelopes), 0 );
   if(
      (NULL != top_envelope &&
      MAILBOX_ENVELOPE_SPECIAL_CONNECT == top_envelope->special) &&
      socket_dest == top_envelope->socket_dest
   ) {
      socket_out = top_envelope->socket_src;
      bdestroy( top_envelope->contents );
      free( top_envelope );
      vector_delete( &(mailbox->envelopes), 0 );
   } else if( NULL != top_envelope && NULL != top_envelope->callback ) {
      top_envelope->callback( mailbox, top_envelope );
      if( MAILBOX_ENVELOPE_SPECIAL_DELETE == top_envelope->special ) {
         free( top_envelope );
         vector_delete( &(mailbox->envelopes), 0 );
      }
   }

cleanup:
   return socket_out;
}

void mailbox_call( MAILBOX* mailbox, MAILBOX_CALLBACK callback, void* arg ) {
   MAILBOX_ENVELOPE* outgoing = NULL;
   BOOL ok = FALSE;

   outgoing = (MAILBOX_ENVELOPE*)calloc( 1, sizeof( MAILBOX_ENVELOPE ) );
   scaffold_check_null( outgoing );
   outgoing->contents = NULL;
   outgoing->callback = callback;
   outgoing->socket_src = 0;
   outgoing->socket_dest = 0;
   outgoing->cb_arg = arg;

   ok = TRUE;
   vector_add( &(mailbox->envelopes), outgoing );

cleanup:
   if( TRUE != ok ) {
      free( outgoing );
   }
   return;

}

void mailbox_send(
   MAILBOX* mailbox, size_t socket_src, size_t socket_dest, bstring message
) {
   MAILBOX_ENVELOPE* outgoing = NULL;
   BOOL ok = FALSE;

   outgoing = (MAILBOX_ENVELOPE*)calloc( 1, sizeof( MAILBOX_ENVELOPE ) );
   scaffold_check_null( outgoing );
   outgoing->contents = bstrcpy( message );
   scaffold_check_null( outgoing->contents );
   outgoing->socket_src = socket_src;
   outgoing->socket_dest = socket_dest;

   ok = TRUE;
   vector_add( &(mailbox->envelopes), outgoing );

cleanup:
   if( TRUE != ok ) {
      free( outgoing );
   }
   return;
}

size_t mailbox_connect(
   MAILBOX* mailbox, ssize_t socket_src, size_t socket_dest
) {
   MAILBOX_ENVELOPE* outgoing = NULL;
   size_t socket_out = -1;

   if( 0 > mailbox->last_socket ) {
      mailbox->last_socket = 0;
   }

   outgoing = (MAILBOX_ENVELOPE*)calloc( 1, sizeof( MAILBOX_ENVELOPE ) );
   scaffold_check_null( outgoing );
   if( 0 > socket_src ) {
      socket_out = mailbox->last_socket++;
   } else {
      socket_out = socket_src;
   }
   outgoing->socket_src = socket_out;
   outgoing->socket_dest = socket_dest;
   outgoing->special = MAILBOX_ENVELOPE_SPECIAL_CONNECT;
   vector_add( &(mailbox->envelopes), outgoing );

cleanup:
   return socket_out;
}

size_t mailbox_read( MAILBOX* mailbox, size_t socket_dest, bstring buffer ) {
   MAILBOX_ENVELOPE* top_envelope = NULL;
   int length_out = 0;

   top_envelope = vector_get( &(mailbox->envelopes), 0 );
   if(
      NULL != top_envelope &&
      MAILBOX_ENVELOPE_SPECIAL_NONE == top_envelope->special &&
      socket_dest == top_envelope->socket_dest
   ) {
      scaffold_check_null( top_envelope->contents );
      bassign( buffer, top_envelope->contents );
      length_out = blength( buffer );
      bdestroy( top_envelope->contents );
      free( top_envelope );
      vector_delete( &(mailbox->envelopes), 0 );
   } else {
      bassigncstr( buffer, "" );
   }

cleanup:
   return length_out;
}
