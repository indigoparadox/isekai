
#define SYNCBUFF_C
#include "syncbuff.h"

#define SYNCBUFF_LINE_DEFAULT 255
#define SYNCBUFF_LINES_INITIAL 2

static SCAFFOLD_SIZE syncbuff_size[2];
static bstring* syncbuff_lines[2];
static SCAFFOLD_SIZE syncbuff_count[2];
static uint8_t syncbuff_listening = 0;
static uint8_t syncbuff_connected = 0;

SCAFFOLD_SIZE syncbuff_get_count( SYNCBUFF_DEST dest ) {
   return syncbuff_count[dest];
}

SCAFFOLD_SIZE syncbuff_get_allocated( SYNCBUFF_DEST dest ) {
   return syncbuff_size[dest];
}

static void syncbuff_alloc( SYNCBUFF_DEST dest ) {
   SCAFFOLD_SIZE i;

   syncbuff_size[dest] = SYNCBUFF_LINES_INITIAL;
   syncbuff_count[dest] = 0;
   syncbuff_lines[dest] =
      (bstring*)calloc( syncbuff_size[dest], sizeof( bstring ) );

   for( i = 0 ; syncbuff_size[dest] > i ; i++ ) {
      syncbuff_lines[dest][i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }
}

static void syncbuff_realloc( SYNCBUFF_DEST dest, SCAFFOLD_SIZE new_alloc ) {
   SCAFFOLD_SIZE i;

   scaffold_print_debug( &module,
      "Realloc: %d: %d to %d\n", dest, syncbuff_size[dest], new_alloc
   );

   syncbuff_size[dest] = new_alloc;
   syncbuff_lines[dest] = scaffold_realloc(
      syncbuff_lines[dest], syncbuff_size[dest], bstring
   );
   scaffold_assert( NULL != syncbuff_lines[dest] );

   for( i = syncbuff_count[dest] ; syncbuff_size[dest] > i ; i++ ) {
      syncbuff_lines[dest][i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }
}

uint8_t syncbuff_listen() {
#ifdef SYNCBUFF_STRICT
   scaffold_assert( 0 == listening );
#else
   if( 0 != syncbuff_listening ) {
      return 0;
   }
#endif /* SYNCBUFF_STRICT */

   syncbuff_alloc( SYNCBUFF_DEST_CLIENT );
   syncbuff_alloc( SYNCBUFF_DEST_SERVER );

   if( !syncbuff_listening ) {
      syncbuff_listening = 1;
      return syncbuff_listening;
   } else {
      return 0;
   }
}

uint8_t syncbuff_connect() {
   assert( 1 == syncbuff_listening );
   if( !syncbuff_connected ) {
      syncbuff_connected = 1;
      return syncbuff_connected;
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
SCAFFOLD_SIZE_SIGNED syncbuff_write( const bstring line, SYNCBUFF_DEST dest ) {
   int bstr_result;
   SCAFFOLD_SIZE_SIGNED i;
   SCAFFOLD_SIZE_SIGNED size_out = -1;

   scaffold_assert( NULL != line );

   scaffold_print_debug( &module,  "Write: %s\n", bdata( line ) );

   if( syncbuff_count[dest] + 2 >= syncbuff_size[dest] ) {
      syncbuff_realloc( dest, syncbuff_size[dest] * 2 );
   }

   scaffold_print_debug( &module,  "Count is %d\n", syncbuff_count[dest] );

   for( i = syncbuff_count[dest] ; 0 <= i ; i-- ) {
      scaffold_print_debug( &module,
         "Move: %d: %d was: %s\n", dest, (i + 1), bdata( syncbuff_lines[dest][i + 1] )
      );
      bstr_result =
         bassignformat( syncbuff_lines[dest][i + 1], "%s", bdata( syncbuff_lines[dest][i] ) );
      scaffold_assert( NULL != syncbuff_lines[dest][i + 1] );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
      scaffold_print_debug( &module,
         "Move: %d: %d now: %s\n", dest, (i + 1), bdata( syncbuff_lines[dest][i + 1] )
      );
   }

   scaffold_print_debug( &module,
      "Write: %d: %d was: %s\n", dest, 0, bdata( syncbuff_lines[dest][0] )
   );
   bstr_result =
      bassignformat( syncbuff_lines[dest][0], "%s", bdata( line ) );
   scaffold_assert( NULL != syncbuff_lines[dest][0] );
   scaffold_assert( blength( line ) == blength( syncbuff_lines[dest][0] ) );
   if( 0 != bstr_result ) {
      goto cleanup;
   }
   scaffold_print_debug( &module,
      "Write: %d: %d now: %s\n", dest, 0, bdata( syncbuff_lines[dest][0] )
   );

   (syncbuff_count[dest])++;

   scaffold_assert( bdata( line ) != bdata( syncbuff_lines[dest][0] ) );

   size_out = blength( syncbuff_lines[dest][0] );

cleanup:
   return size_out;
}

/* Pull a line off the bottom. */
SCAFFOLD_SIZE_SIGNED syncbuff_read( bstring buffer, SYNCBUFF_DEST dest ) {
   int  bstr_result;
   SCAFFOLD_SIZE_SIGNED size_out = -1;

   scaffold_assert( NULL != buffer );
   bstr_result = btrunc( buffer, 0 );
   scaffold_assert( 0 == blength( buffer ) );
   if( 0 != bstr_result ) {
      scaffold_print_debug( &module,  "btrunc error on line: %d\n", __LINE__ );
      goto cleanup;
   }

   if( 0 < syncbuff_count[dest] ) {
      (syncbuff_count[dest])--;
      scaffold_assert( NULL != syncbuff_lines[dest][syncbuff_count[dest]] );
      bstr_result =
         bassignformat( buffer, "%s", bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );
      scaffold_assert( NULL != buffer );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
      bstr_result = btrunc( syncbuff_lines[dest][syncbuff_count[dest]], 0 );
      if( 0 != bstr_result ) {
         goto cleanup;
      }
   }

   scaffold_assert( bdata( buffer ) != bdata( syncbuff_lines[dest][syncbuff_count[dest]] ) );

   size_out = blength( buffer );

cleanup:
   return size_out;
}
