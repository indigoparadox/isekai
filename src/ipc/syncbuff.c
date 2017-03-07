
#include "syncbuff.h"

#ifdef DEBUG_SYNCBUFF
#include <assert.h>
#include <stdio.h>

#define syncbuff_trace( ... ) \
   fprintf( stderr, __VA_ARGS__ )

#else

#define syncbuff_trace( ... )
#define assert( expression )

#endif /* DEBUG_SYNCBUFF */

#define SYNCBUFF_LINE_DEFAULT 255
#define SYNCBUFF_LINES_INITIAL 2

static size_t syncbuff_size[2];
static bstring* syncbuff_lines[2];
static size_t syncbuff_count[2];

size_t syncbuff_get_count( SYNCBUFF_DEST dest ) {
   return syncbuff_count[dest];
}

size_t syncbuff_get_allocated( SYNCBUFF_DEST dest ) {
   return syncbuff_size[dest];
}

static void syncbuff_alloc( SYNCBUFF_DEST dest ) {
   size_t i;

   syncbuff_size[dest] = SYNCBUFF_LINES_INITIAL;
   syncbuff_count[dest] = 0;
   syncbuff_lines[dest] =
      (bstring*)calloc( syncbuff_size[dest], sizeof( bstring ) );

   for( i = 0 ; syncbuff_size[dest] > i ; i++ ) {
      syncbuff_lines[dest][i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }
}

static void syncbuff_realloc( SYNCBUFF_DEST dest, size_t new_alloc ) {
   size_t i;

   syncbuff_trace(
      "Realloc: %d: %d to %d\n", dest, syncbuff_size[dest], new_alloc
   );

   syncbuff_size[dest] = new_alloc;
   syncbuff_lines[dest] = (bstring*)realloc(
      syncbuff_lines[dest], syncbuff_size[dest] * sizeof( bstring )
   );
   assert( NULL != syncbuff_lines[dest] );

   for( i = syncbuff_count[dest] ; syncbuff_size[dest] > i ; i++ ) {
      syncbuff_lines[dest][i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }
}

uint8_t syncbuff_listen() {
   static uint8_t listening = 0;

#ifdef SYNCBUFF_STRICT
   assert( 0 == listening );
#else
   if( 0 != listening ) {
      return 0;
   }
#endif /* SYNCBUFF_STRICT */

   syncbuff_alloc( SYNCBUFF_DEST_CLIENT );
   syncbuff_alloc( SYNCBUFF_DEST_SERVER );

   if( !listening ) {
      listening = 1;
      return listening;
   } else {
      return 0;
   }
}

uint8_t syncbuff_connect() {
   static uint8_t connected = 0;
   if( !connected ) {
      connected = 1;
      return connected;
   } else {
      return 0;
   }
}

uint8_t syncbuff_accept() {
   static uint8_t accepted = 0;
   if( !accepted ) {
      accepted = 1;
      return accepted;
   } else {
      return 0;
   }
}

/* Push a line down onto the top. */
ssize_t syncbuff_write( const bstring line, SYNCBUFF_DEST dest ) {
   int bstr_result;
   ssize_t i;
   ssize_t size_out = -1;

   assert( NULL != line );

   syncbuff_trace( "Write: %s\n", bdata( line ) );

   if( syncbuff_count[dest] + 2 >= syncbuff_size[dest] ) {
      syncbuff_realloc( dest, syncbuff_size[dest] * 2 );
   }

   syncbuff_trace( "Count is %d\n", syncbuff_count[dest] );

   for( i = syncbuff_count[dest] ; 0 <= i ; i-- ) {
      syncbuff_trace(
         "Move: %d: %d was: %s\n", dest, (i + 1), bdata( syncbuff_lines[dest][i + 1] )
      );
      bstr_result =
         bassignformat( syncbuff_lines[dest][i + 1], "%s", bdata( syncbuff_lines[dest][i] ) );
      assert( NULL != syncbuff_lines[dest][i + 1] );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
      syncbuff_trace(
         "Move: %d: %d now: %s\n", dest, (i + 1), bdata( syncbuff_lines[dest][i + 1] )
      );
   }

   syncbuff_trace(
      "Write: %d: %d was: %s\n", dest, 0, bdata( syncbuff_lines[dest][0] )
   );
   bstr_result =
      bassignformat( syncbuff_lines[dest][0], "%s", bdata( line ) );
   assert( NULL != syncbuff_lines[dest][0] );
   assert( blength( line ) == blength( syncbuff_lines[dest][0] ) );
   if( 0 != bstr_result ) {
      goto cleanup;
   }
   syncbuff_trace(
      "Write: %d: %d now: %s\n", dest, 0, bdata( syncbuff_lines[dest][0] )
   );

   (syncbuff_count[dest])++;

   assert( bdata( line ) != bdata( syncbuff_lines[dest][0] ) );

   size_out = blength( syncbuff_lines[dest][0] );

cleanup:
   return size_out;
}

/* Pull a line off the bottom. */
ssize_t syncbuff_read( bstring buffer, SYNCBUFF_DEST dest ) {
   int  bstr_result;
   ssize_t size_out = -1;

   assert( NULL != buffer );
   btrunc( buffer, 0 );
   assert( 0 == blength( buffer ) );

   if( 0 < syncbuff_count[dest] ) {
      (syncbuff_count[dest])--;
      assert( NULL != syncbuff_lines[dest][syncbuff_count[dest]] );
      bstr_result =
         bassignformat( buffer, "%s", bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );
      assert( NULL != buffer );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
      bstr_result = btrunc( syncbuff_lines[dest][syncbuff_count[dest]], 0 );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
   }

   assert( bdata( buffer ) != bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );

   size_out = blength( buffer );

cleanup:
   return size_out;
}
