
#include "syncbuff.h"

#include <assert.h>

#define SYNCBUFF_MAX 10
#define SYNCBUFF_LINE_DEFAULT 255

static bstring syncbuff_lines_to_server[SYNCBUFF_MAX];
static size_t syncbuff_count_to_server = 0;

static bstring syncbuff_lines_to_client[SYNCBUFF_MAX];
static size_t syncbuff_count_to_client = 0;

uint8_t syncbuff_listen() {
   static uint8_t listening = 0;
   size_t i;

   assert( 0 == listening );

   for( i = 0 ; SYNCBUFF_MAX > i ; i++ ) {
      syncbuff_lines_to_client[i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
      syncbuff_lines_to_server[i] = bfromcstralloc( SYNCBUFF_LINE_DEFAULT, "" );
   }

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
   size_t i;
   bstring* syncbuff_lines = NULL;
   size_t* syncbuff_count = NULL;

   assert( NULL != line );

   if( SYNCBUFF_DEST_SERVER == dest ) {
      syncbuff_lines = syncbuff_lines_to_server;
      syncbuff_count = &syncbuff_count_to_server;
   } else {
      syncbuff_lines = syncbuff_lines_to_client;
      syncbuff_count = &syncbuff_count_to_client;
   }

   if( SYNCBUFF_MAX <= *syncbuff_count ) {
      return 0;
   }

   for( i = *syncbuff_count ; 0 < i ; i-- ) {
      bassignformat( syncbuff_lines[i], "%s", bdata( syncbuff_lines[i - 1] ) );
   }
   (*syncbuff_count)++;

   bassignformat( syncbuff_lines[0], "%s", bdata( line ) );

   assert( bdata( line ) != bdata( syncbuff_lines[0] ) );

   return blength( syncbuff_lines[0] );
}

/* Pull a line off the bottom. */
ssize_t syncbuff_read( bstring buffer, SYNCBUFF_DEST dest ) {
   bstring* syncbuff_lines = NULL;
   size_t* syncbuff_count = NULL;

   assert( NULL != buffer );
   btrunc( buffer, 0 );

   if( SYNCBUFF_DEST_SERVER == dest ) {
      syncbuff_lines = syncbuff_lines_to_server;
      syncbuff_count = &syncbuff_count_to_server;
   } else {
      syncbuff_lines = syncbuff_lines_to_client;
      syncbuff_count = &syncbuff_count_to_client;
   }

   if( SYNCBUFF_MAX <= *syncbuff_count ) {
      return 0;
   }

   if( 0 < *syncbuff_count ) {
      (*syncbuff_count)--;
      assert( NULL != syncbuff_lines[*syncbuff_count] );
      bassignformat( buffer, "%s", bdata( syncbuff_lines[*syncbuff_count] ) );
      btrunc( syncbuff_lines[*syncbuff_count], 0 );
   }

   assert( bdata( buffer ) != bdata( syncbuff_lines[*syncbuff_count] ) );

   return blength( buffer );
}
