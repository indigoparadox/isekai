
#include "syncbuff.h"

#include <assert.h>

#define SYNCBUFF_LINE_DEFAULT 255
#define SYNCBUFF_LINES_INITIAL 2

static size_t syncbuff_size[2];
static bstring* syncbuff_lines[2];
static size_t syncbuff_count[2];

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

   syncbuff_size[dest] = new_alloc;
   syncbuff_lines[dest] = (bstring*)realloc(
      syncbuff_lines[dest], syncbuff_size[dest] * sizeof( bstring )
   );

   for( i = syncbuff_count[dest] ; syncbuff_size[dest] > i ; i++ ) {
      syncbuff_lines[dest][i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }

   (syncbuff_count[dest])++;
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
   size_t i;
   const char* c;

   assert( NULL != line );

   if( syncbuff_count[dest] + 2 >= syncbuff_size[dest] ) {
      syncbuff_realloc( dest, syncbuff_size[dest] * 2 );
   }

   for( i = 0 ; syncbuff_count[dest] > i ; i++ ) {
      bstr_result =
         bassignformat( syncbuff_lines[dest][i + 1], "%s", syncbuff_lines[dest][i] );
      assert( NULL != syncbuff_lines[dest][i + 1] );
      assert( 0 == bstr_result );
   }

   bstr_result =
      bassignformat( syncbuff_lines[dest][0], "%s", bdata( line ) );
   assert( NULL != syncbuff_lines[dest][0] );
   assert( 0 == bstr_result );

   (syncbuff_count[dest])++;

   assert( bdata( line ) != bdata( syncbuff_lines[dest][0] ) );

   c = bdata( line );

   c = bdata( syncbuff_lines[dest][0] );

   return blength( syncbuff_lines[dest][0] );
}

/* Pull a line off the bottom. */
ssize_t syncbuff_read( bstring buffer, SYNCBUFF_DEST dest ) {
   int  bstr_result;

   assert( NULL != buffer );
   btrunc( buffer, 0 );

   if( 0 < syncbuff_count[dest] ) {
      (syncbuff_count[dest])--;
      assert( NULL != syncbuff_lines[dest][syncbuff_count[dest]] );
      bstr_result =
         bassignformat( buffer, "%s", bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );
      assert( NULL != buffer );
      assert( 0 == bstr_result );
      bstr_result = btrunc( syncbuff_lines[dest][syncbuff_count[dest]], 0 );
      assert( 0 == bstr_result );
   }

   assert( bdata( buffer ) != bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );

   return blength( buffer );
}
