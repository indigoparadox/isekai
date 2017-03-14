
#ifndef SYNCBUFF_H
#define SYNCBUFF_H

#include <stddef.h>
#include <stdlib.h>

#include "../scaffold.h"
#include "../bstrlib/bstrlib.h"

/* TODO: Minimal ifdefs for scaffold. */

typedef enum _SYNCBUFF_DEST {
   SYNCBUFF_DEST_SERVER,
   SYNCBUFF_DEST_CLIENT
} SYNCBUFF_DEST;

SCAFFOLD_SIZE syncbuff_get_count( SYNCBUFF_DEST dest );
SCAFFOLD_SIZE syncbuff_get_allocated( SYNCBUFF_DEST dest );
uint8_t syncbuff_listen();
uint8_t syncbuff_connect();
uint8_t syncbuff_accept();
SCAFFOLD_SIZE_SIGNED syncbuff_write( const bstring line, SYNCBUFF_DEST dest );
SCAFFOLD_SIZE_SIGNED syncbuff_read( bstring buffer, SYNCBUFF_DEST dest );

#endif /* SYNCBUFF_H */
