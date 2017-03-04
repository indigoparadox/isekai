
#include "mailbox.h"

#include <stdlib.h>

size_t mailbox_listen( MAILBOX* mailbox ) {
    vector_init( &(mailbox->envelopes) );
    vector_init( &(mailbox->sockets_assigned ) );
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

static void mailbox_envelope_cleanup( const struct _REF* ref ) {
   MAILBOX_ENVELOPE* e = (MAILBOX_ENVELOPE*)scaffold_container_of( ref, MAILBOX_ENVELOPE, refcount );
   bdestroy( e->contents );
   e->callback = NULL;
   e->socket_src = -1;
   e->socket_dest = -1;
   e->cb_arg = NULL;
   e->special = MAILBOX_ENVELOPE_SPECIAL_DELETE;
}

MAILBOX_ENVELOPE* mailbox_envelope_new(
   bstring contents,
   MAILBOX_CALLBACK callback,
   void* cb_arg,
   size_t src,
   size_t dest,
   MAILBOX_ENVELOPE_SPECIAL special
) {
   MAILBOX_ENVELOPE* outgoing;

   outgoing = (MAILBOX_ENVELOPE*)calloc( 1, sizeof( MAILBOX_ENVELOPE ) );
   scaffold_check_null( outgoing );

#ifdef DEBUG
   if( NULL != contents ) {
      assert( MAILBOX_ENVELOPE_SPECIAL_NONE == special );
      assert( NULL == callback );
   }
#endif /* DEBUG */

   ref_init( &(outgoing->refcount), mailbox_envelope_cleanup );

   outgoing->contents = NULL != contents ? bstrcpy( contents ) : NULL;
   outgoing->callback = callback;
   outgoing->socket_src = src;
   outgoing->socket_dest = dest;
   outgoing->cb_arg = cb_arg;
   outgoing->special = special;

cleanup:
   return outgoing;
}

void mailbox_envelope_free( MAILBOX_ENVELOPE* e ) {
   if( ref_dec( &(e->refcount) ) ) {
      free( e );
   }
}

size_t mailbox_accept( MAILBOX* mailbox, size_t socket_dest ) {
   MAILBOX_ENVELOPE* top_envelope = NULL;
   size_t socket_out = -1;
   size_t i = 0;
#ifdef DEBUG
   size_t starting_envelope_count = 0;

   starting_envelope_count = vector_count( &(mailbox->envelopes) );
#endif /* DEBUG */

   if( 0 >= socket_dest ) {
      goto cleanup;
   }

   /* Iterate through jobs and handle urgent things, like new clients. */
   top_envelope = vector_get( &(mailbox->envelopes), i );
   while( NULL!= top_envelope ) {
      assert( NULL == top_envelope || NULL == top_envelope->callback || NULL == top_envelope->contents );
      if(
         MAILBOX_ENVELOPE_SPECIAL_CONNECT == top_envelope->special &&
         socket_dest == top_envelope->socket_dest
      ) {
         socket_out = top_envelope->socket_src;
         //vector_add_scalar( &(mailbox->sockets_assigned), socket_out );
         //FIXME: Deallocate this envelope!
         //mailbox_envelope_free( top_envelope );
         vector_remove( &(mailbox->envelopes), i );
#ifdef DEBUG
         assert( starting_envelope_count > vector_count( &(mailbox->envelopes) ) );
#endif /* DEBUG */
      }
      top_envelope = vector_get( &(mailbox->envelopes), ++i );
   }

   if( 0 >= vector_count( &(mailbox->envelopes) ) ) {
      goto cleanup;
   }

   /* Do one callback job per cycle. */
   //VECTOR* envelopes =  &(mailbox->envelopes);
   top_envelope = vector_get( &(mailbox->envelopes), 0 );
   assert( NULL == top_envelope || NULL == top_envelope->callback || NULL == top_envelope->contents );
   if( NULL != top_envelope && NULL != top_envelope->callback ) {
      top_envelope->callback( mailbox, top_envelope );
      if( MAILBOX_ENVELOPE_SPECIAL_DELETE == top_envelope->special ) {
         mailbox_envelope_free( top_envelope );
         vector_remove( &(mailbox->envelopes), 0 );
#ifdef DEBUG
         assert( starting_envelope_count > vector_count( &(mailbox->envelopes) ) );
#endif /* DEBUG */

         scaffold_print_debug(
            "Job removed from queue. %d jobs total.\n",
            vector_count( &(mailbox->envelopes ) )
         );
      }
   }

cleanup:
   return socket_out;
}

void mailbox_call( MAILBOX* mailbox, ssize_t socket_src, MAILBOX_CALLBACK callback, void* arg ) {
   MAILBOX_ENVELOPE* outgoing = NULL;
   BOOL ok = FALSE;

   outgoing = mailbox_envelope_new(
      NULL, callback, arg, socket_src, 0, MAILBOX_ENVELOPE_SPECIAL_NONE
   );
   scaffold_check_null( outgoing );

   ok = TRUE;
   vector_add( &(mailbox->envelopes), outgoing );

   scaffold_print_debug(
      "Job added to queue. %d jobs total.\n",
      vector_count( &(mailbox->envelopes ) )
   );

cleanup:
   if( TRUE != ok ) {
      mailbox_envelope_free( outgoing );
   }
   return;

}

void mailbox_send(
   MAILBOX* mailbox, size_t socket_src, size_t socket_dest, bstring message
) {
   MAILBOX_ENVELOPE* outgoing = NULL;
   BOOL ok = FALSE;

   scaffold_check_null( message );

   outgoing = mailbox_envelope_new(
      message,
      NULL,
      NULL,
      socket_src,
      socket_dest,
      MAILBOX_ENVELOPE_SPECIAL_NONE
   );
   scaffold_check_null( outgoing );

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

   outgoing = mailbox_envelope_new(
      NULL, NULL, NULL, -1, socket_dest, MAILBOX_ENVELOPE_SPECIAL_CONNECT
   );
   scaffold_check_null( outgoing );

   if( 0 > socket_src ) {
      socket_out = mailbox->last_socket++;
   } else {
      socket_out = socket_src;
   }
   vector_add_scalar( &(mailbox->sockets_assigned), socket_out, FALSE );
   outgoing->socket_src = socket_out;

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
      //bdestroy( top_envelope->contents );
      vector_remove( &(mailbox->envelopes), 0 );
      mailbox_envelope_free( top_envelope );
   } else {
      bassigncstr( buffer, "" );
   }

cleanup:
   return length_out;
}

void mailbox_close( MAILBOX* mailbox, ssize_t socket ) {
   vector_remove_scalar( &(mailbox->sockets_assigned), socket );
}

BOOL mailbox_is_alive( MAILBOX* mailbox, ssize_t socket ) {
   if( 0 > vector_get_scalar( &(mailbox->sockets_assigned), socket ) ) {
      return FALSE;
   } else {
      return TRUE;
   }
}
